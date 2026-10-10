#!/usr/bin/env python3
"""Zieht eine echte Lektion von Duolingo ab -- fuer ein Wegwerfkonto.

Warum ein Wegwerfkonto: **benotet wird im Client, der Server glaubt es.** Wer
die Benotung am eigenen Konto ausprobiert, verdirbt den eigenen Kurs. Die
Weboberflaeche legt fuer jeden Besucher, der ohne Anmeldung eine Lektion
probiert, selbst so ein Konto an (`POST /2023-05-23/users`, Antwortkopf
`jwt`) -- genau das macht dieses Werkzeug, und die Marke bleibt im Prozess.

    tools/sitzung-abziehen.py [--lernsprache es] [--muttersprache en]
                             [--art LESSON] [--ziel doc/]

Schreibt `doc/sitzung-<art>.json` (die Lektion, wie sie kommt) und
`doc/sitzung-befund.md` (Auszaehlung: welche Aufgabenarten, welche Felder).
"""

import argparse
import json
import os
import sys
import time
import urllib.error
import urllib.request
import uuid

BASIS = "https://www.duolingo.com"
AGENT = ("Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 "
         "(KHTML, like Gecko) Chrome/125 Safari/537.36")

# Die Arten, die eine Qt-4.7-Oberflaeche ohne Mikrofon und ohne Zeichenbrett
# zeigen kann. Was hier nicht steht, schickt der Server auch nicht -- das ist
# der Hebel aus doc/aufgabenarten.md.
UNSERE_ARTEN = [
    "assist", "characterIntro", "characterMatch", "characterSelect",
    "completeReverseTranslation", "definition", "form", "gapFill", "judge",
    "listen", "listenComplete", "listenMatch", "listenTap", "match", "name",
    "partialListen", "partialReverseTranslate", "readComprehension",
    "reverseAssist", "select", "selectPronunciation", "selectTranscription",
    "syllableTap", "syllableListenTap", "tapCloze", "tapComplete",
    "tapDescribe", "translate", "typeCloze", "typeComplete",
]


def ruf(pfad, koerper=None, marke=None, art=None):
    url = pfad if pfad.startswith("http") else BASIS + pfad
    daten = None
    if koerper is not None:
        daten = json.dumps(koerper).encode("utf8")
    anfrage = urllib.request.Request(url, data=daten)
    anfrage.add_header("User-Agent", AGENT)
    anfrage.add_header("Accept", "application/json")
    if daten is not None:
        anfrage.add_header("Content-Type", "application/json")
    if marke:
        anfrage.add_header("Authorization", "Bearer " + marke)
    if art:
        anfrage.get_method = lambda: art
    try:
        with urllib.request.urlopen(anfrage, timeout=60) as antwort:
            rohe = antwort.read()
            kopf = dict(antwort.headers.items())
            return antwort.status, kopf, rohe
    except urllib.error.HTTPError as f:
        return f.code, dict(f.headers.items()), f.read()


def konto_anlegen(muttersprache, lernsprache):
    status, kopf, rohe = ruf(
        "/2023-05-23/users?fields=id",
        {"distinctId": str(uuid.uuid4()),
         "fromLanguage": muttersprache,
         "learningLanguage": lernsprache,
         "timezone": "Europe/Vienna"})
    if status != 200:
        sys.exit("Konto anlegen ging nicht (%d): %s" % (status, rohe[:300]))
    kennung = json.loads(rohe)["id"]
    marke = kopf.get("jwt") or kopf.get("Jwt") or kopf.get("JWT")
    if not marke:
        sys.exit("Keine Marke im Antwortkopf: %s" % sorted(kopf))
    return kennung, marke


FELDER = ("id,fromLanguage,learningLanguage,streak,totalXp,xpGoal,health,"
          "currentCourse{id,title,fromLanguage,learningLanguage,xp,"
          "pathSectioned,healthEnabled}")


def konto_lesen(kennung, marke):
    status, _, rohe = ruf("/2017-06-30/users/%d?fields=%s" % (kennung, FELDER),
                          marke=marke)
    if status != 200:
        sys.exit("Konto lesen ging nicht (%d): %s" % (status, rohe[:300]))
    return json.loads(rohe)


def erste_stufe(kurs):
    """Die erste Stufe des Pfades: ihre Kennung und ihr Beiwerk."""
    for abschnitt in kurs.get("pathSectioned") or []:
        for einheit in abschnitt.get("units") or []:
            for stufe in einheit.get("levels") or []:
                return stufe, abschnitt, einheit
    return None, None, None


def sitzung_holen(art, kurs, marke, stufe=None):
    koerper = {
        "challengeTypes": UNSERE_ARTEN,
        "fromLanguage": kurs["fromLanguage"],
        "learningLanguage": kurs["learningLanguage"],
        "isFinalLevel": False,
        "isV2": True,
        "juicy": True,
        "smartTipsVersion": 2,
        "type": art,
    }
    if stufe is not None:
        koerper["pathLevelSpecifics"] = {"levelIndex": 0}
        koerper["levelIndex"] = 0
        koerper["levelSessionIndex"] = 0
        koerper["pathLevelId"] = stufe.get("id")
        beiwerk = stufe.get("pathLevelMetadata") or stufe.get("pathLevelClientData")
        if beiwerk:
            koerper["metadata"] = beiwerk
        if stufe.get("skillIds"):
            koerper["skillIds"] = stufe["skillIds"]
    status, _, rohe = ruf("/2023-05-23/sessions", koerper, marke=marke)
    return status, koerper, rohe


def auszaehlen(sitzung):
    """Welche Aufgabenarten kamen, und welche Felder hat jede?"""
    zaehler = {}
    for a in sitzung.get("challenges") or []:
        art = a.get("type", "?")
        eintrag = zaehler.setdefault(art, {"anzahl": 0, "felder": {}})
        eintrag["anzahl"] += 1
        for feld, wert in a.items():
            kurz = type(wert).__name__
            eintrag["felder"].setdefault(feld, kurz)
    return zaehler


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--muttersprache", default="en")
    p.add_argument("--lernsprache", default="es")
    p.add_argument("--art", default=None,
                   help="Sitzungsart; ohne Angabe wird LESSON und "
                        "GLOBAL_PRACTICE probiert")
    p.add_argument("--ziel", default=None)
    args = p.parse_args()

    hier = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    ziel = args.ziel or os.path.join(hier, "doc")
    os.makedirs(ziel, exist_ok=True)

    kennung, marke = konto_anlegen(args.muttersprache, args.lernsprache)
    print("Wegwerfkonto %d angelegt" % kennung)

    nutzer = konto_lesen(kennung, marke)
    kurs = nutzer.get("currentCourse") or {}
    open(os.path.join(ziel, "konto-rohdaten.json"), "w", encoding="utf8").write(
        json.dumps(nutzer, indent=1, ensure_ascii=False))
    print("Kurs: %s (%s->%s), Abschnitte: %d" % (
        kurs.get("title"), kurs.get("fromLanguage"), kurs.get("learningLanguage"),
        len(kurs.get("pathSectioned") or [])))

    stufe, _, _ = erste_stufe(kurs)
    if stufe:
        print("erste Stufe: %s" % json.dumps(
            {k: v for k, v in stufe.items() if not isinstance(v, (list, dict))},
            ensure_ascii=False))

    arten = [args.art] if args.art else ["LESSON", "GLOBAL_PRACTICE"]
    befunde = []
    for art in arten:
        status, koerper, rohe = sitzung_holen(
            art, kurs, marke, stufe if art == "LESSON" else None)
        print("%s -> %d, %d Byte" % (art, status, len(rohe)))
        if status != 200:
            befunde.append((art, status, rohe[:400].decode("utf8", "replace"), None))
            continue
        sitzung = json.loads(rohe)
        name = os.path.join(ziel, "sitzung-%s.json" % art.lower())
        open(name, "w", encoding="utf8").write(
            json.dumps(sitzung, indent=1, ensure_ascii=False))
        zaehler = auszaehlen(sitzung)
        print("  %d Aufgaben, Arten: %s" % (
            len(sitzung.get("challenges") or []),
            ", ".join("%s x%d" % (k, v["anzahl"]) for k, v in sorted(zaehler.items()))))
        befunde.append((art, status, None, (sitzung, zaehler, name)))
        time.sleep(1)

    zeilen = ["# Was eine echte Lektion enthaelt\n",
              "Erzeugt von `tools/sitzung-abziehen.py` (Wegwerfkonto, "
              "nie das eigene) am %s.\n" % time.strftime("%d.%m.%Y")]
    for art, status, fehler, treffer in befunde:
        zeilen.append("\n## %s -> %d\n" % (art, status))
        if treffer is None:
            zeilen.append("```\n%s\n```" % (fehler or "(leerer Koerper)"))
            continue
        sitzung, zaehler, name = treffer
        oben = {k: v for k, v in sitzung.items()
                if not isinstance(v, (list, dict))}
        zeilen.append("Rohdaten: `%s`\n" % os.path.relpath(name, hier))
        zeilen.append("Felder der Sitzung selbst: `%s`\n"
                      % json.dumps(oben, ensure_ascii=False)[:800])
        zeilen.append("\n| Aufgabenart | Anzahl | Felder |")
        zeilen.append("|---|---|---|")
        for art2, e in sorted(zaehler.items()):
            zeilen.append("| `%s` | %d | %s |" % (
                art2, e["anzahl"],
                ", ".join("`%s`:%s" % (f, t) for f, t in sorted(e["felder"].items()))))
    open(os.path.join(ziel, "sitzung-befund.md"), "w", encoding="utf8").write(
        "\n".join(zeilen) + "\n")
    print("geschrieben:", os.path.join(ziel, "sitzung-befund.md"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
