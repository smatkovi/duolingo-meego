#!/usr/bin/env python3
"""Gibt eine Lektion ab -- am Wegwerfkonto, nie am eigenen.

Das ist die Probe, an der alles haengt: beim `PUT /sessions/{id}` traegt
jede Aufgabe ihr `correct` selbst, und der Server uebernimmt es. Wenn das
geht, kann die App Fortschritt schreiben; wenn nicht, bleibt sie ein
Lesegeraet.

    tools/sitzung-abgeben.py [--art GLOBAL_PRACTICE] [--falsch 1]
                            [--trocken]

`--falsch n` beantwortet die ersten n Aufgaben absichtlich falsch (so wird
geprueft, dass Herzen und XP sich wie erwartet verhalten). `--trocken`
schickt nichts, sondern schreibt nur, was es schicken wuerde.
"""

import argparse
import json
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import duo  # noqa: E402

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))


def musterloesung(aufgabe):
    verfolg = aufgabe.get("challengeResponseTrackingProperties") or {}
    if verfolg.get("best_solution"):
        return verfolg["best_solution"]
    if aufgabe.get("correctSolutions"):
        return aufgabe["correctSolutions"][0]
    wahlen = aufgabe.get("choices") or []
    reihe = aufgabe.get("correctIndices")
    if wahlen and reihe is not None:
        def text(w):
            return w["text"] if isinstance(w, dict) else w
        return " ".join(text(wahlen[i]) for i in reihe)
    if aufgabe.get("correctIndex") is not None and wahlen:
        w = wahlen[aufgabe["correctIndex"]]
        return w["text"] if isinstance(w, dict) else w
    if aufgabe.get("pairs"):
        return ""
    return aufgabe.get("prompt", "")


def beantworten(sitzung, falsch=0):
    """Schreibt Antwort und Benotung in jede Aufgabe -- so wie die
    Weboberflaeche es vor dem Abgeben tut (`gradingResult`, `correct`,
    `guess`, `timeTaken`)."""
    richtig = 0
    strecke = 0
    beste_strecke = 0
    for nummer, aufgabe in enumerate(sitzung.get("challenges") or []):
        loesung = musterloesung(aufgabe)
        absichtlich_falsch = nummer < falsch
        antwort = loesung if not absichtlich_falsch else (loesung + " xyzzy")
        graph = aufgabe.get("grader")
        if graph:
            korrekt = duo.benoten(graph, antwort)
        else:
            korrekt = not absichtlich_falsch
        aufgabe["correct"] = korrekt
        aufgabe["guess"] = antwort
        aufgabe["skipped"] = False
        aufgabe["timeTaken"] = 4000 + nummer * 137
        aufgabe["gradingResult"] = {"correct": korrekt, "guess": antwort}
        if korrekt:
            richtig += 1
            strecke += 1
            beste_strecke = max(beste_strecke, strecke)
        else:
            strecke = 0
    return richtig, beste_strecke


def abschliessen(sitzung, richtig, beste_strecke, herzen):
    """Die Felder, die die Weboberflaeche bei END_SESSION setzt."""
    jetzt = int(time.time() * 1000)
    anzahl = len(sitzung.get("challenges") or [])
    sitzung["startTime"] = (jetzt - 1000 * (anzahl * 5)) / 1000.0
    sitzung["endTime"] = jetzt / 1000.0
    sitzung["enableBonusPoints"] = False
    sitzung["failed"] = False
    sitzung["heartsLeft"] = herzen
    sitzung["maxInLessonStreak"] = beste_strecke
    sitzung["shouldLearnThings"] = True
    # Was die Sitzung an XP verspricht. Die Weboberflaeche rechnet es aus
    # Art, Laenge und Boni; 10 ist der Normalfall einer Lektion. Was der
    # Server daraus macht, steht in der Antwort -- darum wird es gemessen
    # und nicht geglaubt.
    sitzung["xpPromised"] = 10
    return sitzung


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--art", default="GLOBAL_PRACTICE")
    p.add_argument("--falsch", type=int, default=0)
    p.add_argument("--herzen", type=int, default=5)
    p.add_argument("--trocken", action="store_true")
    p.add_argument("--frisch", action="store_true",
                   help="ein neues Wegwerfkonto anlegen")
    args = p.parse_args()

    hier = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    kennung, marke = duo.wegwerfkonto(frisch=args.frisch)
    print("Wegwerfkonto %s" % kennung)

    vorher = duo.konto_lesen(kennung, marke,
                             "totalXp,streak,health,xpGains")
    print("vorher: XP %s, Strähne %s, Zuwaechse %d"
          % (vorher.get("totalXp"), vorher.get("streak"),
             len(vorher.get("xpGains") or [])))

    nutzer = duo.konto_lesen(kennung, marke)
    kurs = nutzer.get("currentCourse") or {}
    stufe, _ = duo.erste_offene_stufe(kurs)
    status, koerper, rohe = duo.sitzung_holen(
        args.art, kurs, marke, stufe if args.art == "LESSON" else None)
    print("holen %s -> %d (%d Byte)" % (args.art, status, len(rohe)))
    if status != 200:
        open(os.path.join(hier, "doc", "sitzung-fehler.json"), "wb").write(rohe)
        print("Koerper war: %s" % json.dumps(koerper, ensure_ascii=False)[:600])
        print(rohe[:400].decode("utf8", "replace"))
        return 1
    sitzung = json.loads(rohe)

    # Was die Anfrage ueber die Pfadstufe wusste, bleibt in der Abgabe:
    # die Antwort des Servers nennt `levelId` nicht mehr, und ohne ihn
    # koennte die Stufe nicht weiterzaehlen. (Gemessen: sie zaehlt mit
    # einem Wegwerfkonto so oder so nicht weiter -- siehe doc/BEFUND.md.)
    for feld in ("levelId", "pathLevelSessionMetadata", "levelIndex",
                 "levelSessionIndex", "skillId", "treeId"):
        if feld in koerper and feld not in sitzung:
            sitzung[feld] = koerper[feld]

    richtig, strecke = beantworten(sitzung, args.falsch)
    abschliessen(sitzung, richtig, strecke, args.herzen)
    print("beantwortet: %d von %d richtig, laengste Strecke %d"
          % (richtig, len(sitzung["challenges"]), strecke))

    ziel = os.path.join(hier, "doc", "abgabe-%s.json" % args.art.lower())
    open(ziel, "w", encoding="utf8").write(
        json.dumps(sitzung, indent=1, ensure_ascii=False))
    print("Abgabe liegt in %s (%d Byte)"
          % (os.path.relpath(ziel, hier), os.path.getsize(ziel)))
    if args.trocken:
        return 0

    for versuch in range(4):
        status, kopf, antwort = duo.ruf(
            "%s/sessions/%s" % (duo.NEU, sitzung["id"]), sitzung,
            marke=marke, methode="PUT",
            kopfzeilen={"Idempotency-Key": sitzung["id"]})
        print("abgeben -> %d (%d Byte)" % (status, len(antwort)))
        if status != 202:
            break
        # 202 heisst: angenommen, noch nicht verrechnet -- die
        # Weboberflaeche wartet und schickt wieder.
        time.sleep(0.25 * 2 ** versuch)

    if status == 200:
        d = json.loads(antwort)
        open(os.path.join(hier, "doc", "abgabe-antwort.json"), "w",
             encoding="utf8").write(json.dumps(d, indent=1, ensure_ascii=False))
        interessant = {k: d[k] for k in
                       ("xpGain", "xp", "hasXpBoost", "streakInfo",
                        "sessionXpInfo", "failed", "heartsLeft")
                       if k in d}
        print("Antwort: %s" % json.dumps(interessant, ensure_ascii=False)[:400])
    else:
        print(antwort[:500].decode("utf8", "replace"))

    time.sleep(2)
    nachher = duo.konto_lesen(kennung, marke, "totalXp,streak,health,xpGains")
    print("nachher: XP %s, Strähne %s, Zuwaechse %d"
          % (nachher.get("totalXp"), nachher.get("streak"),
             len(nachher.get("xpGains") or [])))
    if nachher.get("totalXp") != vorher.get("totalXp"):
        print("=> Fortschritt ist angekommen.")
    else:
        print("=> Nichts angekommen.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
