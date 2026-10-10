#!/usr/bin/env python3
"""Liest die Schnittstelle aus dem Web-Bundle von Duolingo.

Nicht geraten, sondern nachgeschlagen: die Weboberflaeche ist ein
Webpack-Buendel, in dem die Routen, die Sitzungsarten und die Liste der
Aufgabenarten im Klartext stehen. Dieses Werkzeug holt die Teile und schreibt
`doc/bundle-befund.md`.

    tools/bundle-lesen.py [--arbeit <verzeichnis>]

Ohne --arbeit wird in ein Verzeichnis unter /tmp geladen. Wer schon geladen
hat, gibt dasselbe Verzeichnis wieder an -- dann wird nichts neu geholt.
"""

import argparse
import json
import os
import re
import subprocess
import sys

WURZEL = "https://www.duolingo.com/"
AGENT = ("Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 "
         "(KHTML, like Gecko) Chrome/125 Safari/537.36")


def holen(url, ziel):
    if os.path.exists(ziel) and os.path.getsize(ziel) > 0:
        return
    subprocess.check_call(["curl", "-s", "--compressed", "-A", AGENT,
                           url, "-o", ziel])


def buendel_laden(arbeit):
    os.makedirs(arbeit, exist_ok=True)
    start = os.path.join(arbeit, "root.html")
    holen(WURZEL, start)
    text = open(start, encoding="utf8", errors="replace").read()
    stuecke = {}
    for url in sorted(set(re.findall(r'https://[^"\']*\.js', text))):
        if "cloudfront" not in url:
            continue
        name = os.path.basename(url).split("-")[0] + ".js"
        ziel = os.path.join(arbeit, name)
        holen(url, ziel)
        stuecke[name] = open(ziel, encoding="utf8", errors="replace").read()
    return text, stuecke


def feld_liste(text, anker, name):
    """Holt `name=[ "a","b",... ]` ab der Stelle `anker`."""
    i = text.find(anker)
    if i < 0:
        return []
    seg = text[i:i + 80000]
    m = re.search(r'(?<![A-Za-z0-9_$])' + re.escape(name) + r'=\[', seg)
    if not m:
        return []
    tiefe = 0
    start = m.end() - 1
    for j in range(start, min(len(seg), start + 6000)):
        if seg[j] == "[":
            tiefe += 1
        elif seg[j] == "]":
            tiefe -= 1
            if tiefe == 0:
                roh = seg[start:j + 1]
                return re.findall(r'[A-Za-z0-9_$.]+\.([A-Za-z]+)|"([a-zA-Z]+)"', roh)
    return []


def aufzaehlung(text, probe):
    """Alle `e.Name="WERT"` aus dem Block, in dem `probe` vorkommt."""
    i = text.find(probe)
    if i < 0:
        return {}
    blk = text[max(0, i - 4000):i + 4000]
    return dict(re.findall(r'e\.([A-Za-z0-9_]+)="([A-Za-z0-9_]+)"', blk))


def routen(stuecke):
    gefunden = set()
    muster = re.compile(r'\.(get|post|put|patch|delete)\(`?([/"][^`"\')]{2,80})')
    for text in stuecke.values():
        for art, pfad in muster.findall(text):
            pfad = pfad.strip('"')
            if pfad.startswith("/"):
                gefunden.add((art.upper(), pfad))
    return sorted(gefunden)


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--arbeit", default="/tmp/duo-bundle")
    p.add_argument("--ziel", default=None)
    args = p.parse_args()

    start, stuecke = buendel_laden(args.arbeit)
    gross = max(stuecke.items(), key=lambda kv: len(kv[1]))[1]

    sitzungsarten = aufzaehlung(gross, 'GlobalPractice="GLOBAL_PRACTICE"')
    aufgabenarten = {k: v for k, v in
                     aufzaehlung(gross, 'Translate="translate"').items()
                     if v[0].islower()}

    # Die Liste, die die Weboberflaeche selbst bei POST /sessions erklaert
    # (Modul 70732, Ausfuhr KT fuer Lektionen und fM fuer DuoRadio).
    rohe = feld_liste(gross, "{KT:()=>Q,fM:()=>Y", "Q")
    erklaert = [a or b for a, b in rohe]

    icon = re.search(r'rel="apple-touch-icon"[^>]*href="([^"]+)"', start)

    zeilen = []
    zeilen.append("# Duolingo: was im Web-Bundle steht\n")
    zeilen.append("Erzeugt von `tools/bundle-lesen.py` -- nicht von Hand "
                  "gepflegt. Gelesene Teile: "
                  + ", ".join(sorted(stuecke)) + "\n")
    if icon:
        zeilen.append("Startersymbol: " + icon.group(1) + "\n")

    zeilen.append("\n## Sitzungsarten (POST /sessions, Feld `type`)\n")
    for name, wert in sorted(sitzungsarten.items(), key=lambda kv: kv[1]):
        zeilen.append("- `%s` (%s)" % (wert, name))

    zeilen.append("\n## Aufgabenarten (Feld `type` je Aufgabe)\n")
    zeilen.append(", ".join("`%s`" % v for v in sorted(set(aufgabenarten.values()))))

    if erklaert:
        zeilen.append("\n\n## Was die Weboberflaeche selbst erklaert "
                      "(`challengeTypes`)\n")
        zeilen.append(", ".join("`%s`" % v for v in erklaert))

    zeilen.append("\n\n## Routen, die im Buendel stehen\n")
    for art, pfad in routen(stuecke):
        if "/sessions" in pfad or "/users" in pfad or "/courses" in pfad \
           or "/login" in pfad or "/rewards" in pfad:
            zeilen.append("- `%s %s`" % (art, pfad))

    ausgabe = "\n".join(zeilen) + "\n"
    ziel = args.ziel
    if ziel is None:
        hier = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        ziel = os.path.join(hier, "doc", "bundle-befund.md")
    os.makedirs(os.path.dirname(ziel), exist_ok=True)
    open(ziel, "w", encoding="utf8").write(ausgabe)
    print("geschrieben:", ziel)
    print(json.dumps({"sitzungsarten": len(sitzungsarten),
                      "aufgabenarten": len(set(aufgabenarten.values())),
                      "erklaert": len(erklaert)}, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    sys.exit(main())
