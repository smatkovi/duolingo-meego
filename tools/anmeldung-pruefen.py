#!/usr/bin/env python3
"""Vergleicht die Anmeldewege, ohne ein echtes Konto anzufassen.

Es gibt zwei: den alten unter `/2017-06-30/login` (Feld `login`) und den,
den die Weboberflaeche heute nimmt, `/2023-05-23/login` (Feld `identifier`).
Wer das falsche Feld an den falschen Weg schickt, bekommt **dieselbe
401-Antwort wie bei falschem Passwort** -- und sucht dann den Fehler beim
Konto statt beim Aufruf.

Dieses Werkzeug schickt absichtlich eine Kennung, die es nicht gibt, und
zeigt, wie jeder Weg darauf antwortet. Kein Durchprobieren von Passwoertern
-- ein Versuch je Weg.

    tools/anmeldung-pruefen.py [--kennung <name>]
"""

import argparse
import json
import os
import sys
import uuid

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import duo  # noqa: E402


def versuch(titel, pfad, koerper):
    status, kopf, rohe = duo.ruf(pfad, koerper)
    marke = kopf.get("jwt") or kopf.get("Jwt") or kopf.get("JWT")
    print("== %s" % titel)
    print("   %s -> %d%s" % (pfad, status, "  (Marke im Kopf!)" if marke else ""))
    text = rohe[:300].decode("utf8", "replace")
    try:
        text = json.dumps(json.loads(rohe), ensure_ascii=False)[:300]
    except ValueError:
        pass
    print("   %s" % text)
    print()
    return status, marke


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--kennung", default="kein-konto-" + uuid.uuid4().hex[:10])
    p.add_argument("--passwort", default="x" + uuid.uuid4().hex[:12])
    args = p.parse_args()
    kennung, passwort = args.kennung, args.passwort
    print("Probe mit einer Kennung, die es nicht gibt: %s\n" % kennung)

    gemeinsam = {"distinctId": str(uuid.uuid4())}

    versuch("alt, Feld `identifier` (das machte die App bisher)",
            duo.ALT + "/login?fields=jwt",
            dict(gemeinsam, identifier=kennung, password=passwort))
    versuch("alt, Feld `login`",
            duo.ALT + "/login?fields=jwt",
            dict(gemeinsam, login=kennung, password=passwort))
    versuch("neu, Feld `identifier` (das macht die Weboberflaeche)",
            duo.NEU + "/login?fields=jwt",
            dict(gemeinsam, identifier=kennung, password=passwort))
    versuch("neu, Feld `login`",
            duo.NEU + "/login?fields=jwt",
            dict(gemeinsam, login=kennung, password=passwort))
    return 0


if __name__ == "__main__":
    sys.exit(main())
