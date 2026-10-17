#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Baut das Startersymbol fuer Harmattan aus dem Symbol der Webseite.

    tools/icon-bauen.py [--quelle meego/icons/quelle.png]

Geschrieben werden `meego/icons/icon-80.png` (Starter), `icon-64.png`
(base64 im control, `XB-Maemo-Icon-26`) und `icon-320.png` -- letzteres
nicht fuers Telefon, sondern fuer die Seite des Repos: 80 Pixel sehen dort
aus wie ein Daumenabdruck.

Die Form ist die **Squircle-Silhouette der Standard-Apps** -- nicht
nachgebaut, sondern der Alphakanal eines Blanco-Icons
(`~/ps/meego-icon-tool`). Ein Kreis oder ein eigenes abgerundetes Quadrat
faellt auf dem Startbildschirm zwischen den Stock-Icons sofort auf.

Die Falle bei genau diesem Bild: das Symbol der Webseite ist fuer iOS
geschnitten und hat **durchsichtige Ecken** (Alpha 0). Wer so maskiert,
bekommt vom Squircle schwarze Zwickel in den Ecken -- der Alphakanal der
Maske macht aus Durchsichtigem Flaeche. Deshalb wird die Quelle erst zu
einem vollen Quadrat in ihrer Grundfarbe aufgefuellt; der Squircle schneidet
dann seine eigene Kontur aus einer Flaeche, in der keine fremde mehr steckt.
Die Grundfarbe wird gemessen, nicht geraten.
"""

import argparse
import os
import subprocess
import sys
import tempfile

from PIL import Image

HIER = os.path.dirname(os.path.abspath(__file__))
WURZEL = os.path.dirname(HIER)
SQUIRCLE = os.path.expanduser("~/ps/meego-icon-tool/squircle.py")


def grundfarbe(bild):
    """Die haeufigste deckende Farbe am Rand -- dort liegt die Flaeche."""
    breite, hoehe = bild.size
    px = bild.load()
    zaehler = {}
    for y in range(hoehe):
        for x in list(range(breite // 8)) + list(range(breite * 7 // 8, breite)):
            r, g, b, a = px[x, y]
            if a > 240:
                zaehler[(r, g, b)] = zaehler.get((r, g, b), 0) + 1
    if not zaehler:
        return (88, 204, 2)  # das Gruen der Marke, falls nichts zu messen ist
    return max(zaehler.items(), key=lambda e: e[1])[0]


def quadrat_fuellen(quelle, ziel):
    """Zuschnitt auf das deckende Material, dann volles Quadrat im Grund."""
    bild = Image.open(quelle).convert("RGBA")
    deckend = bild.split()[3].point(lambda v: 255 if v > 200 else 0)
    kasten = deckend.getbbox()
    if not kasten:
        raise SystemExit("Das Quellsymbol ist leer.")
    bild = bild.crop(kasten)
    grund = grundfarbe(bild)
    breite, hoehe = bild.size
    kante = max(breite, hoehe)
    voll = Image.new("RGBA", (kante, kante), grund + (255,))
    voll.paste(bild, ((kante - breite) // 2, (kante - hoehe) // 2), bild)
    voll.save(ziel)
    return ziel, grund


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--quelle",
                   default=os.path.join(WURZEL, "meego", "icons", "quelle.png"))
    args = p.parse_args()

    arbeit = tempfile.mkdtemp(prefix="duo-icon-")
    sauber, grund = quadrat_fuellen(args.quelle,
                                    os.path.join(arbeit, "sauber.png"))
    print("Grundfarbe %02x%02x%02x" % grund)

    meego = os.path.join(WURZEL, "meego", "icons")
    os.makedirs(meego, exist_ok=True)
    subprocess.check_call([sys.executable, SQUIRCLE, "--out", meego,
                           "--fill", sauber, "--inset", "1.0"])

    # Dasselbe noch einmal gross. Nicht aus dem 80er hochskaliert -- die
    # Kurve franst dabei aus --, sondern direkt aus dem Werkzeug, das
    # innen ohnehin mit 320 arbeitet.
    gross = os.path.join(meego, "icon-320.png")
    subprocess.check_call([
        sys.executable, "-c",
        "import sys;sys.path.insert(0, %r);import squircle;"
        "squircle.build(None, None, None, None, False, 0.62, False, False,"
        " %r, 1.0).save(%r)"
        % (os.path.dirname(SQUIRCLE), sauber, gross)])

    for name in ("icon-80.png", "icon-64.png", "icon-320.png"):
        pfad = os.path.join(meego, name)
        print("%s: %s" % (name, Image.open(pfad).size))


if __name__ == "__main__":
    main()
