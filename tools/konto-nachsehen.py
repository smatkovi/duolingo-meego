#!/usr/bin/env python3
"""Sieht nach, ob es zu einer Adresse ueberhaupt ein Konto gibt -- und wie
sein Benutzername lautet.

Hintergrund: Duolingo antwortet auf jede misslungene Anmeldung mit demselben
401 und demselben Satz, egal ob die Kennung falsch ist, das Passwort nicht
stimmt oder gar keines gesetzt ist. Diese Nachschau trennt wenigstens den
ersten Fall ab. Sie liest nur (`GET /2017-06-30/users?email=…`), aendert
nichts und probiert kein Passwort.

    tools/konto-nachsehen.py <adresse-oder-benutzername>
"""

import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import duo  # noqa: E402

FELDER = "id,username,email,hasPlus,hasGoogleId,hasFacebookId,name,creationDate"


def nachsehen(was):
    schluessel = "email" if "@" in was else "username"
    status, _, rohe = duo.ruf(
        "%s/users?%s=%s&fields=users%%7B%s%%7D"
        % (duo.ALT, schluessel, was.replace("@", "%40"), FELDER))
    if status != 200:
        print("Nachschau ging nicht (%d): %s"
              % (status, rohe[:200].decode("utf8", "replace")))
        return 1
    gefunden = json.loads(rohe).get("users") or []
    if not gefunden:
        print("Zu %r gibt es kein Konto." % was)
        return 0
    for u in gefunden:
        print("Konto gefunden:")
        for feld in ("id", "username", "email", "name", "hasGoogleId",
                     "hasFacebookId", "hasPlus"):
            if feld in u:
                print("   %-14s %s" % (feld, u[feld]))
    return 0


if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit("Aufruf: konto-nachsehen.py <adresse-oder-benutzername>")
    sys.exit(nachsehen(sys.argv[1]))
