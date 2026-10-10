#!/usr/bin/env python3
"""Liest aus dem Web-Buendel, welche Aufgabenarten es gibt und wie jede
benotet wird.

Warum das wichtig ist: **benotet wird im Client, der Server glaubt es.**
Beim `PUT /sessions/{id}` traegt jede Aufgabe ihr `correct` selbst. Wer das
falsch rechnet, verdirbt den echten Kursfortschritt. Also wird die Regel
nicht erfunden, sondern aus der Weboberflaeche abgeschrieben: dort steht je
Aufgabenart eine Karte mit `grade`, `validate`, `getTts` und
`prepareChallengeState`.

    tools/aufgaben-lesen.py [--buendel /tmp/duo-bundle/app.js]

Schreibt `doc/aufgabenarten.md`.
"""

import argparse
import os
import re
import sys

# Die Handgriffe, die uns angehen. `grade` ist die Benotung, `validate` sagt,
# wann eine Antwort ueberhaupt abgeschickt werden darf.
HANDGRIFFE = ("grade", "validate", "isReadyForGuessing", "getTts",
              "prepareChallengeState")


def karte_finden(text):
    i = text.find("{KT:()=>Q,fM:()=>Y")
    if i < 0:
        return None
    seg = text[i:i + 200000]
    m = re.search(r'(?<![A-Za-z0-9_$.])K\s*=\s*\{', seg)
    if not m:
        return None
    tiefe = 0
    start = m.end() - 1
    for j in range(start, len(seg)):
        if seg[j] == "{":
            tiefe += 1
        elif seg[j] == "}":
            tiefe -= 1
            if tiefe == 0:
                return seg[start:j + 1]
    return None


def eintraege(karte):
    """Zerlegt `{[w.Sm.Assist]: new class ... , [w.Sm.Judge]: ... }`."""
    gefunden = []
    tiefe = 0
    j = 1
    letzter = None
    while j < len(karte):
        if tiefe == 0:
            m = re.match(r'\s*\[[A-Za-z0-9_$]+\.[A-Za-z0-9_$]+\.([A-Za-z0-9_$]+)\]\s*:',
                         karte[j:])
            if m:
                if letzter:
                    gefunden.append((letzter[0], karte[letzter[1]:j]))
                letzter = (m.group(1), j + m.end())
                j += m.end()
                continue
        c = karte[j]
        if c in "{[(":
            tiefe += 1
        elif c in "}])":
            tiefe -= 1
        j += 1
    if letzter:
        gefunden.append((letzter[0], karte[letzter[1]:]))
    return gefunden


def handgriff(text, name):
    """Holt `this.<name>=<ausdruck>` bis zum Komma auf gleicher Ebene."""
    m = re.search(r'this\.' + name + r'\s*=\s*', text)
    if not m:
        return None
    tiefe = 0
    start = m.end()
    for j in range(start, len(text)):
        c = text[j]
        if c in "{[(":
            tiefe += 1
        elif c in "}])":
            if tiefe == 0:
                return text[start:j].strip()
            tiefe -= 1
        elif c == "," and tiefe == 0:
            return text[start:j].strip()
    return text[start:].strip()


def namen_karte(text):
    """`Sm` ist die Aufzaehlung der Aufgabenarten: Assist -> "assist"."""
    i = text.find('Translate="translate"')
    if i < 0:
        return {}
    blk = text[max(0, i - 6000):i + 6000]
    return dict(re.findall(r'e\.([A-Za-z0-9_]+)="([a-zA-Z]+)"', blk))


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--buendel", default="/tmp/duo-bundle/app.js")
    p.add_argument("--ziel", default=None)
    args = p.parse_args()

    text = open(args.buendel, encoding="utf8", errors="replace").read()
    karte = karte_finden(text)
    if karte is None:
        sys.exit("Die Aufgabenkarte steht nicht in %s" % args.buendel)
    namen = namen_karte(text)

    zeilen = ["# Aufgabenarten und ihre Benotung\n",
              "Erzeugt von `tools/aufgaben-lesen.py` aus dem Web-Buendel "
              "-- nicht von Hand gepflegt.\n",
              "`grade` ist die Regel, nach der die **Weboberflaeche selbst** "
              "benotet; der Server uebernimmt das Ergebnis beim "
              "`PUT /sessions/{id}`. Unsere Fassung muss dasselbe rechnen.\n"]

    liste = eintraege(karte)
    zeilen.append("\nGefunden: **%d** Aufgabenarten.\n" % len(liste))
    for kennung, koerper in liste:
        art = namen.get(kennung, kennung)
        zeilen.append("\n## `%s` (%s)\n" % (art, kennung))
        for name in HANDGRIFFE:
            wert = handgriff(koerper, name)
            if wert is None:
                continue
            kurz = re.sub(r'\s+', ' ', wert)
            if len(kurz) > 700:
                kurz = kurz[:700] + " …"
            zeilen.append("- `%s`: `%s`" % (name, kurz))

    ziel = args.ziel
    if ziel is None:
        hier = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        ziel = os.path.join(hier, "doc", "aufgabenarten.md")
    os.makedirs(os.path.dirname(ziel), exist_ok=True)
    open(ziel, "w", encoding="utf8").write("\n".join(zeilen) + "\n")
    print("geschrieben: %s (%d Arten)" % (ziel, len(liste)))
    print(", ".join(namen.get(k, k) for k, _ in liste))
    return 0


if __name__ == "__main__":
    sys.exit(main())
