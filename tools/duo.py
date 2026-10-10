#!/usr/bin/env python3
"""Gemeinsames Stueck fuer die Werkzeuge: Leitung, Wegwerfkonto, Benoter.

Nur fuer die Werkzeuge auf dem Entwicklungsrechner. Was hier steht, wird
spaeter nach `src/` in C++ uebersetzt -- die Benotung (`benoten`) absichtlich
zuerst hier, weil sie gegen die echten Rohdaten geprueft werden kann.
"""

import json
import os
import unicodedata
import urllib.error
import urllib.request
import uuid

BASIS = "https://www.duolingo.com"
ALT = "/2017-06-30"      # Konto, Anmeldung
NEU = "/2023-05-23"      # Sitzungen, Konto anlegen
AGENT = ("Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 "
         "(KHTML, like Gecko) Chrome/125 Safari/537.36")

# Die Aufgabenarten, die eine Qt-4.7-Oberflaeche ohne Mikrofon und ohne
# Zeichenbrett zeigen kann. Was hier nicht steht, schickt der Server nicht.
UNSERE_ARTEN = [
    "assist", "characterIntro", "characterMatch", "characterSelect",
    "completeReverseTranslation", "definition", "form", "gapFill", "judge",
    "listen", "listenComplete", "listenMatch", "listenTap", "match", "name",
    "partialListen", "partialReverseTranslate", "readComprehension",
    "reverseAssist", "select", "selectPronunciation", "selectTranscription",
    "syllableTap", "syllableListenTap", "tapCloze", "tapComplete",
    "tapDescribe", "translate", "typeCloze", "typeComplete",
]


# ---------------------------------------------------------------- Leitung

def ruf(pfad, koerper=None, marke=None, methode=None, kopfzeilen=None):
    """Ein Aufruf. Gibt (status, kopf, rohbytes) zurueck."""
    url = pfad if pfad.startswith("http") else BASIS + pfad
    daten = json.dumps(koerper).encode("utf8") if koerper is not None else None
    anfrage = urllib.request.Request(url, data=daten)
    anfrage.add_header("User-Agent", AGENT)
    anfrage.add_header("Accept", "application/json")
    if daten is not None:
        anfrage.add_header("Content-Type", "application/json")
    if marke:
        anfrage.add_header("Authorization", "Bearer " + marke)
    for name, wert in (kopfzeilen or {}).items():
        anfrage.add_header(name, wert)
    if methode:
        anfrage.get_method = lambda: methode
    try:
        with urllib.request.urlopen(anfrage, timeout=120) as antwort:
            return antwort.status, dict(antwort.headers.items()), antwort.read()
    except urllib.error.HTTPError as f:
        return f.code, dict(f.headers.items()), f.read()


# ------------------------------------------------------------ Wegwerfkonto

def wegwerf_datei():
    hier = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    return os.path.join(hier, ".wegwerf.json")


def wegwerfkonto(muttersprache="en", lernsprache="es", frisch=False):
    """Ein Konto zum Ausprobieren -- nie das eigene.

    Begruendung in doc/BEFUND.md: benotet wird im Client, der Server glaubt
    es. Wer am eigenen Konto probiert, verdirbt den eigenen Kurs. Die
    Weboberflaeche legt fuer jeden Besucher ohne Anmeldung genau so ein
    Konto an. Kennung und Marke bleiben in `.wegwerf.json` (nicht im Repo),
    damit nicht bei jedem Lauf ein neues entsteht.
    """
    datei = wegwerf_datei()
    if not frisch and os.path.exists(datei):
        with open(datei, encoding="utf8") as f:
            alt = json.load(f)
        if alt.get("lernsprache") == lernsprache and \
           alt.get("muttersprache") == muttersprache:
            return alt["kennung"], alt["marke"]

    status, kopf, rohe = ruf(NEU + "/users?fields=id", {
        "distinctId": str(uuid.uuid4()),
        "fromLanguage": muttersprache,
        "learningLanguage": lernsprache,
        "timezone": "Europe/Vienna",
    })
    if status != 200:
        raise RuntimeError("Konto anlegen ging nicht (%d): %s"
                           % (status, rohe[:300]))
    kennung = json.loads(rohe)["id"]
    marke = kopf.get("jwt") or kopf.get("Jwt") or kopf.get("JWT")
    if not marke:
        raise RuntimeError("Keine Marke im Antwortkopf: %s" % sorted(kopf))
    with open(datei, "w", encoding="utf8") as f:
        json.dump({"kennung": kennung, "marke": marke,
                   "muttersprache": muttersprache,
                   "lernsprache": lernsprache}, f)
    os.chmod(datei, 0o600)
    return kennung, marke


FELDER = ("id,fromLanguage,learningLanguage,streak,totalXp,xpGoal,health,"
          "currentCourse{id,title,fromLanguage,learningLanguage,xp,"
          "pathSectioned,healthEnabled}")


def konto_lesen(kennung, marke, felder=FELDER):
    status, _, rohe = ruf("%s/users/%s?fields=%s" % (ALT, kennung, felder),
                          marke=marke)
    if status != 200:
        raise RuntimeError("Konto lesen ging nicht (%d): %s" % (status, rohe[:300]))
    return json.loads(rohe)


# -------------------------------------------------------------- Sitzungen

def erste_offene_stufe(kurs):
    """Die erste Stufe des Pfades, die noch nicht durch ist."""
    for abschnitt in kurs.get("pathSectioned") or []:
        for einheit in abschnitt.get("units") or []:
            for stufe in einheit.get("levels") or []:
                if stufe.get("state") in ("active", "unlocked"):
                    return stufe, einheit
    return None, None


def sitzungskoerper(art, kurs, stufe=None):
    """Der Koerper fuer `POST /sessions`.

    Abgeschrieben aus dem Buendel (`tools/lektion-koerper-lesen.py`): der
    gemeinsame Teil und, bei einer Pfadstufe der Art `skill`, der Teil aus
    `pathLevelClientData`. Nicht geraten -- LESSON antwortet mit 500, wenn
    `levelId`, `levelSessionIndex` oder `pathLevelSessionMetadata` fehlen.
    """
    koerper = {
        "challengeTypes": UNSERE_ARTEN,
        "fromLanguage": kurs["fromLanguage"],
        "learningLanguage": kurs["learningLanguage"],
        "isFinalLevel": False,
        "isRedoingPassedNode": False,
        "isV2": True,
        "juicy": True,
        "smartTipsVersion": 2,
        "type": art,
    }
    if stufe is None:
        return koerper

    daten = stufe.get("pathLevelClientData") or {}
    beiwerk = stufe.get("pathLevelMetadata") or {}
    if beiwerk.get("treeId"):
        koerper["treeId"] = beiwerk["treeId"]
    koerper["levelId"] = stufe.get("id")
    koerper["levelSessionIndex"] = stufe.get("finishedSessions", 0)
    koerper["pathLevelSessionMetadata"] = stufe.get("pathLevelSessionMetadata")
    if stufe.get("type") == "skill":
        koerper["skillId"] = daten.get("skillId")
        koerper["levelIndex"] = daten.get("crownLevelIndex", 0)
        koerper["isCustomIntroSkill"] = stufe.get("subtype") == "custom_intro"
        koerper["isGrammarSkill"] = stufe.get("subtype") == "grammar"
        koerper["showGrammarSkillSplash"] = False
    elif stufe.get("type") == "practice":
        koerper["skillIds"] = daten.get("skillIds") or []
        koerper["lexemePracticeType"] = "practice_level"
    return koerper


def sitzung_holen(art, kurs, marke, stufe=None):
    koerper = sitzungskoerper(art, kurs, stufe)
    status, _, rohe = ruf(NEU + "/sessions", koerper, marke=marke)
    return status, koerper, rohe


# -------------------------------------------------------------- Benotung

def glaetten(wort):
    """Kleinschreibung, Satzzeichen weg -- `lenient` im Graphen ist so.

    Der Abstand bleibt: im Graphen ist **das Leerzeichen eine eigene Kante**
    (`{"to": 3, "lenient": " "}`), kein Trenner.
    """
    wort = unicodedata.normalize("NFC", wort or "").lower()
    return "".join(c for c in wort
                   if not unicodedata.category(c).startswith("P"))


def zerlegen(text, mit_abstand=True):
    """Zerlegt eine Antwort in die Stuecke, die der Graph liest.

    Bei `whitespaceDelimited` sind das Woerter **und** die Leerzeichen
    dazwischen, jedes als eigenes Stueck. Satzzeichen fallen weg; bleibt
    davon ein leeres Wort uebrig (etwa aus "¡"), faellt es ebenfalls weg,
    und zwei Leerzeichen hintereinander werden eines.
    """
    geglaettet = glaetten(text)
    if not mit_abstand:
        return [c for c in geglaettet if c.strip()]
    stuecke = []
    for teil in geglaettet.split(" "):
        teil = teil.strip()
        if not teil:
            continue
        if stuecke:
            stuecke.append(" ")
        stuecke.append(teil)
    return stuecke


def benoten(graph, antwort):
    """Laeuft den Benotungsgraphen der Aufgabe mit der Antwort ab.

    Der Graph in `challenge.grader` ist ein Automat: `vertices[i]` ist eine
    Liste von Kanten `{to, lenient, orig}`. Eine Kante mit leerem `lenient`
    ist ein Leerschritt. Angenommen ist die Antwort, wenn sie ganz gelesen
    ist und ein Zustand erreicht wurde, der keine Kante mehr hat (die
    Endknoten stehen im Graphen als leere Listen).

    Warum von Hand und nicht ueber den Server: beim `PUT /sessions/{id}`
    traegt jede Aufgabe ihr `correct` selbst, und der Server uebernimmt es.
    Falsch gerechnet heisst: echter Kursfortschritt verdorben.
    """
    if not graph or not graph.get("vertices"):
        return False
    knoten = graph["vertices"]
    stuecke = zerlegen(antwort, graph.get("whitespaceDelimited", True))

    def schluss(menge):
        """Leerschritte mitnehmen."""
        offen = list(menge)
        gesehen = set(menge)
        while offen:
            i = offen.pop()
            for kante in knoten[i] if i < len(knoten) else []:
                if not kante.get("lenient", "") and kante.get("to") not in gesehen:
                    gesehen.add(kante["to"])
                    offen.append(kante["to"])
        return gesehen

    hier = schluss({0})
    for stueck in stuecke:
        weiter = set()
        for i in hier:
            for kante in knoten[i] if i < len(knoten) else []:
                erwartet = glaetten(kante.get("lenient", ""))
                if erwartet and erwartet == stueck:
                    weiter.add(kante["to"])
        if not weiter:
            return False
        hier = schluss(weiter)
    return any(i < len(knoten) and not knoten[i] for i in hier)
