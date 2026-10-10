#!/usr/bin/env python3
"""Prueft den Benoter gegen echte Rohdaten -- die beste Probe, die es gratis
gibt: **jede Aufgabe muss ihre eigene Musterloesung annehmen.**

Die Musterloesung steht in jeder Aufgabe selbst
(`challengeResponseTrackingProperties.best_solution`, dazu
`correctSolutions` und die Reihe aus `correctIndices`). Nimmt der Graph sie
nicht an, ist unsere Glaettung falsch -- nicht der Graph.

Dazu die Gegenprobe: eine verdrehte oder halbe Antwort darf *nicht*
angenommen werden.

    tools/benoter-pruefen.py [doc/sitzung-global_practice.json ...]
"""
import glob
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import duo  # noqa: E402


def musterloesung(aufgabe):
    verfolg = aufgabe.get("challengeResponseTrackingProperties") or {}
    if verfolg.get("best_solution"):
        return verfolg["best_solution"]
    if aufgabe.get("correctSolutions"):
        return aufgabe["correctSolutions"][0]
    if aufgabe.get("prompt") and aufgabe.get("correctIndices") is None:
        return aufgabe["prompt"]
    wahlen = aufgabe.get("choices") or []
    reihe = aufgabe.get("correctIndices")
    if wahlen and reihe is not None:
        def text(w):
            return w["text"] if isinstance(w, dict) else w
        return " ".join(text(wahlen[i]) for i in reihe)
    return None


def main():
    dateien = sys.argv[1:]
    if not dateien:
        hier = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        dateien = sorted(glob.glob(os.path.join(hier, "doc", "sitzung-*.json")))
    if not dateien:
        sys.exit("keine Rohdaten da -- zuerst tools/sitzung-abziehen.py")

    gut = schlecht = ohne = 0
    gegenprobe_gut = gegenprobe_schlecht = 0
    for datei in dateien:
        sitzung = json.load(open(datei, encoding="utf8"))
        aufgaben = (sitzung.get("challenges") or []) \
            + (sitzung.get("adaptiveChallenges") or [])
        print("\n== %s: %d Aufgaben" % (os.path.basename(datei), len(aufgaben)))
        for a in aufgaben:
            graph = a.get("grader")
            loesung = musterloesung(a)
            if not graph:
                ohne += 1
                continue
            if loesung is None:
                print("   %-18s keine Musterloesung im Datensatz" % a["type"])
                continue
            if duo.benoten(graph, loesung):
                gut += 1
            else:
                schlecht += 1
                print("   %-18s ABGELEHNT: %r" % (a["type"], loesung))

            # Gegenprobe: ein Wort weniger darf nicht durchgehen.
            stuecke = loesung.split()
            if len(stuecke) > 1:
                kuerzer = " ".join(stuecke[:-1])
                if duo.benoten(graph, kuerzer):
                    gegenprobe_schlecht += 1
                    print("   %-18s ANGENOMMEN, obwohl halb: %r"
                          % (a["type"], kuerzer))
                else:
                    gegenprobe_gut += 1

    print("\nMusterloesung angenommen: %d, abgelehnt: %d, ohne Graphen: %d"
          % (gut, schlecht, ohne))
    print("Gegenprobe (halbe Antwort abgelehnt): %d gut, %d durchgelassen"
          % (gegenprobe_gut, gegenprobe_schlecht))
    return 1 if (schlecht or gegenprobe_schlecht) else 0


if __name__ == "__main__":
    sys.exit(main())
