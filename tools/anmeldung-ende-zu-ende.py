#!/usr/bin/env python3
"""Spielt die Anmeldung an einem Konto durch, dessen Passwort wir kennen.

Das ist die Probe, die lange gefehlt hat: ohne ein Konto zum Gegenhalten
sieht jede misslungene Anmeldung gleich aus, und man sucht den Fehler
abwechselnd beim Passwort und beim eigenen Aufruf.

Der Weg dahin, nachgemessen am 10.10.2026: ein frisches Wegwerfkonto
(`POST /2023-05-23/users`) nimmt **in einem Zug** Adresse, Passwort und
Namen an (`PATCH /2023-05-23/users/{id}`) -- ohne reCAPTCHA, solange es
vorher keines hatte. Nur das Passwort allein zu aendern geht nicht, dafuer
will der Dienst das alte (`IDENTITY_INVALID / PASSWORD_INCORRECT`).

    tools/anmeldung-ende-zu-ende.py

Die Adresse liegt auf `@example.com` -- die ist fuer solche Zwecke
reserviert und hat keinen Briefkasten.
"""

import json
import os
import sys
import uuid

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import duo  # noqa: E402


def main():
    # Immer ein frisches Konto: ein schon eingerichtetes nimmt kein neues
    # Passwort mehr an, ohne das alte zu kennen.
    kennung, marke = duo.wegwerfkonto(frisch=True)
    adresse = "duo-meego-%s@example.com" % uuid.uuid4().hex[:10]
    passwort = "Pr0be-" + uuid.uuid4().hex[:12]
    benutzername = "meegoprobe" + uuid.uuid4().hex[:6]

    status, _, rohe = duo.ruf(
        "%s/users/%s?fields=id,username,email" % (duo.NEU, kennung),
        {"email": adresse, "password": passwort, "name": "MeeGo Probe",
         "username": benutzername, "distinctId": str(uuid.uuid4())},
        marke=marke, methode="PATCH")
    print("Konto einrichten -> %d: %s"
          % (status, rohe[:160].decode("utf8", "replace")))
    if status != 200:
        return 1
    print("   Adresse %s, Benutzername %s" % (adresse, benutzername))

    # Jetzt die Anmeldung, genau so, wie die App sie schickt -- und zum
    # Vergleich die anderen Kombinationen.
    proben = (
        ("neu  /login, identifier=Adresse", duo.NEU, "identifier", adresse),
        ("neu  /login, identifier=Benutzername", duo.NEU, "identifier", benutzername),
        ("alt  /login, identifier=Adresse", duo.ALT, "identifier", adresse),
        ("alt  /login, login=Adresse", duo.ALT, "login", adresse),
    )
    for titel, basis, feld, wert in proben:
        koerper = {"distinctId": str(uuid.uuid4()), "password": passwort,
                   feld: wert}
        status, kopf, rohe = duo.ruf(basis + "/login?fields=jwt", koerper)
        hat = bool(kopf.get("jwt") or kopf.get("Jwt") or kopf.get("JWT"))
        print("%-38s -> %d %s" % (titel, status,
                                  "MARKE DA" if hat else
                                  rohe[:90].decode("utf8", "replace")))

    # Und die Gegenprobe: dasselbe mit falschem Passwort.
    koerper = {"distinctId": str(uuid.uuid4()), "identifier": adresse,
               "password": passwort + "x"}
    status, kopf, rohe = duo.ruf(duo.NEU + "/login?fields=jwt", koerper)
    print("%-38s -> %d %s" % ("neu  /login, falsches Passwort", status,
                              "MARKE DA" if kopf.get("jwt") else "keine Marke"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
