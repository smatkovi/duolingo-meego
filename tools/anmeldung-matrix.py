#!/usr/bin/env python3
"""Was verlangt die Anmeldung ausser Kennung und Passwort?

Ein Konto, dessen Passwort wir kennen (tools/anmeldung-ende-zu-ende.py legt
es an), wird mit verschiedenen Beigaben angemeldet -- je Lauf eine
Aenderung. Was durchgeht, sagt uns, was die App mitschicken muss.

Hintergrund: im Web-Buendel haengt an der Anmeldung ein Feld `signal`, und
das ist ein **reCAPTCHA-Enterprise-Token** (`const s = await c.e("LOGIN")`,
dann `login(e, t, s)` -> `{distinctId, signal: s, ...}`). Es kann also sein,
dass die Anmeldung ohne dieses Token gar nicht geht -- dann braucht der
eigene Client einen anderen Weg.

    tools/anmeldung-matrix.py [--konto /tmp/probe-konto.json]
"""

import argparse
import json
import os
import sys
import urllib.request
import uuid

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import duo  # noqa: E402

CHROME = ("Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 "
          "(KHTML, like Gecko) Chrome/125 Safari/537.36")
DUODROID = "Duodroid/6.0.0 Dalvik/2.1.0 (Linux; U; Android 11; Pixel 4 Build/RQ3A)"
MEEGO = "Duolingo-MeeGo/0.1.0 Qt/4.7.4 (Linux; MeeGo 1.2 Harmattan)"


def anmelden(titel, adresse, passwort, agent=CHROME, kopfzeilen=None,
             zusatz=None, basis=None, felder="jwt"):
    koerper = {"distinctId": str(uuid.uuid4()),
               "identifier": adresse, "password": passwort}
    if zusatz:
        koerper.update(zusatz)
    url = "%s%s/login?fields=%s" % (duo.BASIS, basis or duo.NEU, felder)
    daten = json.dumps(koerper).encode("utf8")
    anfrage = urllib.request.Request(url, data=daten)
    anfrage.add_header("User-Agent", agent)
    anfrage.add_header("Content-Type", "application/json")
    anfrage.add_header("Accept", "application/json")
    for name, wert in (kopfzeilen or {}).items():
        anfrage.add_header(name, wert)
    try:
        with urllib.request.urlopen(anfrage, timeout=60) as antwort:
            kopf = dict(antwort.headers.items())
            marke = kopf.get("jwt") or kopf.get("Jwt") or kopf.get("JWT")
            print("%-44s -> %d  %s" % (titel, antwort.status,
                                       "MARKE DA" if marke else "ohne Marke"))
            return bool(marke)
    except urllib.error.HTTPError as f:
        rohe = f.read()
        print("%-44s -> %d  %s" % (titel, f.code,
                                   rohe[:90].decode("utf8", "replace")))
        return False


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--konto", default="/tmp/probe-konto.json")
    args = p.parse_args()
    d = json.load(open(args.konto, encoding="utf8"))
    adresse, passwort = d["adresse"], d["passwort"]
    print("Probekonto %s\n" % adresse)

    anmelden("schlicht, Chrome-Agent", adresse, passwort)
    anmelden("mit Origin und Referer", adresse, passwort,
             kopfzeilen={"Origin": "https://www.duolingo.com",
                         "Referer": "https://www.duolingo.com/"})
    anmelden("Agent der Android-App", adresse, passwort, agent=DUODROID)
    anmelden("Agent der MeeGo-Fassung", adresse, passwort, agent=MEEGO)
    anmelden("mit signal: null", adresse, passwort, zusatz={"signal": None})
    anmelden("alter Weg /2017-06-30", adresse, passwort, basis=duo.ALT)
    anmelden("ohne fields", adresse, passwort, felder="")
    anmelden("falsches Passwort (Gegenprobe)", adresse, passwort + "x")
    return 0


if __name__ == "__main__":
    sys.exit(main())
