#!/usr/bin/env python3
"""Sucht das Buendelstueck, in dem ein Wort vorkommt.

Die Weboberflaeche laedt ihre Seiten erst beim Betreten nach; was Schach
macht, steht deshalb in keinem der Stuecke, die die Startseite nennt. Die
Liste aller Stuecke steht im `manifest`, und dieses Werkzeug holt sie der
Reihe nach (groesste zuerst) und sucht darin.

    tools/buendelstueck-suchen.py chess
    tools/buendelstueck-suchen.py --woerter moveName,opponentMove --hoechstens 60

Geladenes bleibt unter --arbeit liegen; ein zweiter Lauf holt nichts neu.
"""

import argparse
import os
import re
import subprocess
import sys

CDN = "https://d35aaqx5ub95lt.cloudfront.net/js/"
AGENT = ("Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 "
         "(KHTML, like Gecko) Chrome/125 Safari/537.36")


def manifest(arbeit):
    pfad = os.path.join(arbeit, "manifest.js")
    if not os.path.exists(pfad):
        sys.exit("manifest.js fehlt in %s -- erst tools/bundle-lesen.py" % arbeit)
    text = open(pfad, encoding="utf8", errors="replace").read()
    # Im manifest steht die Karte Nummer -> Hash, in der Form 123:"a1b2c3d4".
    paare = re.findall(r'(\d{3,5}):"([a-f0-9]{8})"', text)
    return sorted(set(paare))


def groesse(url):
    try:
        aus = subprocess.check_output(
            ["curl", "-sI", "-A", AGENT, url], timeout=30).decode("utf8", "replace")
    except Exception:
        return 0
    m = re.search(r"content-length: *(\d+)", aus, re.I)
    return int(m.group(1)) if m else 0


def holen(url, ziel):
    if os.path.exists(ziel) and os.path.getsize(ziel) > 0:
        return
    subprocess.check_call(["curl", "-s", "--compressed", "-A", AGENT,
                           url, "-o", ziel])


def main():
    p = argparse.ArgumentParser()
    p.add_argument("wort", nargs="?", default="chess")
    p.add_argument("--woerter", default=None,
                   help="mehrere, durch Komma getrennt")
    p.add_argument("--arbeit", default="/tmp/duo-bundle")
    p.add_argument("--hoechstens", type=int, default=40,
                   help="so viele Stuecke holen (die groessten zuerst)")
    args = p.parse_args()
    gesucht = [w.strip() for w in (args.woerter or args.wort).split(",")]

    stuecke = manifest(args.arbeit)
    print("%d Stuecke im manifest, Groessen werden abgefragt …" % len(stuecke))
    mit = []
    for nummer, hash_ in stuecke:
        url = "%s%s-%s.js" % (CDN, nummer, hash_)
        mit.append((groesse(url), nummer, url))
    mit.sort(reverse=True)

    for g, nummer, url in mit[:args.hoechstens]:
        ziel = os.path.join(args.arbeit, "stueck-%s.js" % nummer)
        holen(url, ziel)
        text = open(ziel, encoding="utf8", errors="replace").read()
        treffer = [w for w in gesucht if w in text]
        if treffer:
            print("%-8s %7d Byte  <- %s" % (nummer, g, ", ".join(treffer)))
    print("geladen nach %s" % args.arbeit)
    return 0


if __name__ == "__main__":
    sys.exit(main())
