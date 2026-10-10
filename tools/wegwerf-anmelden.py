#!/usr/bin/env python3
"""Probiert die Anmeldung an einem Konto aus, dessen Passwort wir kennen.

Warum nicht am eigenen: bei falscher Kennung antwortet Duolingo immer
dasselbe (401 mit demselben Satz), egal ob das Passwort nicht stimmt oder
der Aufruf falsch gebaut ist. Ohne ein Konto zum Gegenhalten sucht man
endlos an der falschen Stelle -- und probiert dabei Passwoerter durch, was
niemand tun soll.

Also: das Wegwerfkonto bekommt eine Adresse (`@example.com`, die ist fuer
genau solche Zwecke reserviert und hat keinen Briefkasten) und ein
Passwort, und dann wird die Anmeldung damit durchgespielt -- einmal je Weg.

    tools/wegwerf-anmelden.py
"""

import json
import os
import sys
import uuid

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import duo  # noqa: E402


def main():
    kennung, marke = duo.wegwerfkonto()
    nutzer = duo.konto_lesen(kennung, marke, "id,username,email,name")
    print("Wegwerfkonto %s, Benutzername %r"
          % (kennung, nutzer.get("username")))

    adresse = "duo-meego-%s@example.com" % uuid.uuid4().hex[:10]
    passwort = "Pr0be-" + uuid.uuid4().hex[:12]

    status, _, rohe = duo.ruf(
        "%s/users/%s?fields=id,username,email" % (duo.NEU, kennung),
        {"email": adresse, "password": passwort,
         "name": "MeeGo Probe", "distinctId": str(uuid.uuid4())},
        marke=marke, methode="PATCH")
    print("Konto eintragen -> %d: %s" % (status, rohe[:200].decode("utf8", "replace")))
    if status != 200:
        print("Geht nicht -- dann sagt diese Probe nichts ueber die Anmeldung.")
        return 1
    angelegt = json.loads(rohe)
    benutzername = angelegt.get("username") or nutzer.get("username")

    for titel, pfad, feld, wert in (
            ("alt, `identifier` = Adresse", duo.ALT, "identifier", adresse),
            ("alt, `login` = Adresse", duo.ALT, "login", adresse),
            ("neu, `identifier` = Adresse", duo.NEU, "identifier", adresse),
            ("neu, `identifier` = Benutzername", duo.NEU, "identifier", benutzername),
    ):
        koerper = {"distinctId": str(uuid.uuid4()), "password": passwort}
        koerper[feld] = wert
        status, kopf, rohe = duo.ruf(pfad + "/login?fields=jwt", koerper)
        hat = bool(kopf.get("jwt") or kopf.get("Jwt") or kopf.get("JWT"))
        print("%-34s %s/login -> %d%s" % (titel, pfad, status,
                                          "  MARKE DA" if hat else ""))
        if status != 200:
            print("      %s" % rohe[:160].decode("utf8", "replace"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
