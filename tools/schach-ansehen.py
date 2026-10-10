#!/usr/bin/env python3
"""Sieht sich den Schachkurs an -- am Wegwerfkonto.

Duolingo hat neben den Sprachen drei andere Faecher; ihre Kurskennungen
stehen im Web-Buendel fest verdrahtet: `CHESS_CH`, `MATH_BT`, `MUSIC_MT`
(ohne Muttersprache). Dieses Werkzeug schaltet ein Wegwerfkonto auf Schach
um, liest den Pfad und holt die erste Lektion -- damit die Frage "koennen
wir Schach auch?" an Rohdaten entschieden wird und nicht am Gefuehl.

    tools/schach-ansehen.py [--kurs CHESS_CH]
"""

import argparse
import json
import os
import sys
import uuid

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import duo  # noqa: E402

# Die Agentenzeile der App -- ohne sie laesst der Dienst niemanden an.
duo.AGENT = ("Duodroid/6.0.0 Dalvik/2.1.0 (Linux; U; MeeGo 1.2 Harmattan; "
             "Nokia N9)")


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--kurs", default="CHESS_CH")
    p.add_argument("--ziel", default=None)
    args = p.parse_args()
    hier = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    ziel = args.ziel or os.path.join(hier, "doc")

    kennung, marke = duo.wegwerfkonto()
    status, _, rohe = duo.ruf(
        "%s/users/%s?fields=currentCourse%%7Bid%%7D" % (duo.NEU, kennung),
        {"currentCourseId": args.kurs, "fromLanguage": "en",
         "distinctId": str(uuid.uuid4())}, marke=marke, methode="PATCH")
    print("auf %s umschalten -> %d %s"
          % (args.kurs, status, rohe[:120].decode("utf8", "replace")))
    if status != 200:
        return 1

    nutzer = duo.konto_lesen(kennung, marke)
    kurs = nutzer.get("currentCourse") or {}
    print("Kurs %s, Abschnitte %d"
          % (kurs.get("id"), len(kurs.get("pathSectioned") or [])))
    stufe, _ = duo.erste_offene_stufe(kurs)
    if not stufe:
        print("Keine offene Stufe im Pfad.")
        return 1
    print("erste Stufe: %s"
          % json.dumps({k: v for k, v in stufe.items()
                        if not isinstance(v, (list, dict))},
                       ensure_ascii=False)[:300])
    beiwerk = stufe.get("pathLevelClientData") or {}
    print("   pathLevelClientData: %s"
          % json.dumps(beiwerk, ensure_ascii=False)[:400])

    # Schach laeuft als eigene Sitzungsart mit `metadata` aus der Stufe.
    sitzungsbeiwerk = (beiwerk.get("sessionMetadatas") or [None])[0]
    koerper = {
        "challengeTypes": ["chessMatch", "chessMiniMatch", "chessPvpMatch"],
        "fromLanguage": "en",
        "isV2": True,
        "juicy": True,
        "type": "CHESS_SESSION",
        "metadata": sitzungsbeiwerk,
        "levelId": stufe.get("id"),
        "levelSessionIndex": stufe.get("finishedSessions", 0),
        "pathLevelSessionMetadata": stufe.get("pathLevelSessionMetadata"),
    }
    status, _, rohe = duo.ruf(duo.NEU + "/sessions", koerper, marke=marke)
    print("Schachlektion holen -> %d, %d Byte" % (status, len(rohe)))
    if status != 200:
        print("   %s" % rohe[:300].decode("utf8", "replace"))
        return 1

    sitzung = json.loads(rohe)
    os.makedirs(ziel, exist_ok=True)
    name = os.path.join(ziel, "sitzung-schach.json")
    open(name, "w", encoding="utf8").write(
        json.dumps(sitzung, indent=1, ensure_ascii=False))
    aufgaben = sitzung.get("challenges") or []
    print("   %d Aufgaben, Arten: %s"
          % (len(aufgaben),
             ", ".join(sorted(set(a.get("type", "?") for a in aufgaben)))))
    if aufgaben:
        erste = aufgaben[0]
        print("   Felder der ersten: %s" % ", ".join(sorted(erste.keys())))
        for feld, wert in erste.items():
            kurz = json.dumps(wert, ensure_ascii=False)
            print("      %-26s %s" % (feld, kurz[:150]))
    print("geschrieben: %s" % name)
    return 0


if __name__ == "__main__":
    sys.exit(main())
