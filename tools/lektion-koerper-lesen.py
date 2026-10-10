#!/usr/bin/env python3
"""Zeigt, wie die Weboberflaeche den Koerper fuer `POST /sessions` baut.

LESSON antwortet mit 500, wenn die Felder der Pfadstufe falsch zusammengelegt
sind. Statt zu raten: die Stelle im Buendel ausdrucken, die den Koerper baut
(die Funktion mit `isV2:!0, juicy:!0`), und daneben alles, was beim Abgeben
auf die Aufgaben und die Sitzung geschrieben wird.

    tools/lektion-koerper-lesen.py [/tmp/duo-bundle/app.js]
"""
import re
import sys

datei = sys.argv[1] if len(sys.argv) > 1 else "/tmp/duo-bundle/app.js"
s = open(datei, encoding="utf8", errors="replace").read()


def zeig(titel, pos, vor=0, nach=2600):
    print("\n########## %s (+%d)" % (titel, pos))
    print(s[max(0, pos - vor):pos + nach])


i = s.find("isV2:!0,juicy:!0")
if i < 0:
    sys.exit("Koerperbauer nicht gefunden")
zeig("Koerper fuer POST /sessions", i, vor=2600, nach=3000)

for probe in ("timeTaken", "shouldLearnThings", "xpPromised:"):
    m = re.search(re.escape(probe), s)
    if m:
        zeig(probe, m.start(), vor=900, nach=900)
