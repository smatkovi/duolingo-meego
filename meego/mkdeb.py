#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Baut ein Harmattan-taugliches .deb aus einem Staging-Verzeichnis.

    python3 mkdeb.py STAGE AUSGABE.deb
    python3 mkdeb.py --info AUSGABE.deb      # Ersatz fuer "dpkg-deb -I/-c"

STAGE enthaelt den Dateibaum, wie er auf dem Geraet landen soll, plus
STAGE/DEBIAN/control.  Alles andere (md5sums, Installed-Size, die beiden
tar.gz und die ar-Huelle) erzeugt dieses Skript.

Warum nicht dpkg-deb oder GNU ar?
  * dpkg-deb gibt es weder auf dem Sailfish-Geraet noch zwingend auf dem
    Build-Rechner.
  * GNU ar haengt an jeden Membernamen ein "/" ("debian-binary/").  Neuere
    dpkg-Fassungen tolerieren das, das Harmattan-dpkg (1.15.x) wurde damit nie
    getestet.  dpkg-deb selbst schreibt die Namen ohne Schraegstrich - genau
    das macht dieses Skript.
  * Die Reihenfolge debian-binary, control.tar.gz, data.tar.gz ist Pflicht,
    ebenso gzip (das alte dpkg kann kein xz/zstd).

Das Ergebnis ist reproduzierbar: feste mtime (SOURCE_DATE_EPOCH oder 0),
uid/gid 0, Besitzer root:root, sortierte Eintraege, gzip ohne Zeitstempel.
Zweimal hintereinander aufgerufen entsteht dieselbe Datei Byte fuer Byte.
"""

import gzip
import hashlib
import io
import os
import stat
import sys
import tarfile
import time

MTIME = int(os.environ.get("SOURCE_DATE_EPOCH", "0"))


def collect(root):
    """(Verzeichnisse, Dateien) relativ zu root, ohne DEBIAN, sortiert."""
    dirs, files = [], []
    for base, subdirs, names in os.walk(root):
        subdirs.sort()
        rel = os.path.relpath(base, root)
        if rel == ".":
            rel = ""
        if rel.split(os.sep)[0] == "DEBIAN":
            subdirs[:] = []
            continue
        if rel:
            dirs.append(rel)
        for n in sorted(names):
            files.append(os.path.join(rel, n) if rel else n)
    return sorted(dirs), sorted(files)


def tar_gz(entries):
    """entries: Liste (name, is_dir, mode, bytes|None) -> gzip-Bytes."""
    raw = io.BytesIO()
    tf = tarfile.open(fileobj=raw, mode="w", format=tarfile.GNU_FORMAT)
    for name, is_dir, mode, data in entries:
        ti = tarfile.TarInfo("./" + name if not name.startswith("./") else name)
        ti.mtime = MTIME
        ti.uid = ti.gid = 0
        ti.uname = ti.gname = "root"
        if is_dir:
            ti.type = tarfile.DIRTYPE
            ti.mode = mode or 0o755
            ti.size = 0
            tf.addfile(ti)
        else:
            ti.type = tarfile.REGTYPE
            ti.mode = mode
            ti.size = len(data)
            tf.addfile(ti, io.BytesIO(data))
    tf.close()
    out = io.BytesIO()
    # mtime=0: sonst steckt die Bauzeit im gzip-Kopf und das Ergebnis waere
    # nicht reproduzierbar.
    with gzip.GzipFile(fileobj=out, mode="wb", compresslevel=9, mtime=0) as gz:
        gz.write(raw.getvalue())
    return out.getvalue()


def ar_member(name, data, mtime=None):
    if mtime is None:
        mtime = MTIME
    hdr = "%-16s%-12d%-6d%-6d%-8o%-10d`\n" % (name, mtime, 0, 0, 0o100644, len(data))
    hdr = hdr.encode("ascii")
    assert len(hdr) == 60, (name, len(hdr))
    # ar-Mitglieder werden auf gerade Laenge aufgefuellt.
    return hdr + data + (b"\n" if len(data) % 2 else b"")


def read_ar(path):
    """[(name, offset, size)] eines ar-Archivs - fuer --info."""
    out = []
    with open(path, "rb") as f:
        if f.read(8) != b"!<arch>\n":
            raise ValueError("%s ist kein ar-Archiv" % path)
        while True:
            hdr = f.read(60)
            if len(hdr) < 60 or hdr[58:60] != b"`\n":
                break
            name = hdr[0:16].decode("ascii", "replace").rstrip().rstrip("/")
            size = int(hdr[48:58].decode("ascii").strip())
            off = f.tell()
            out.append((name, off, size))
            f.seek(off + size + (size & 1))
    return out


def info(path):
    """Zeigt Aufbau, control und Dateiliste - was sonst dpkg-deb -I/-c taete."""
    members = read_ar(path)
    print("== ar-Mitglieder von %s (%d B)" % (path, os.path.getsize(path)))
    for name, off, size in members:
        print("   %-16s %9d B  @%d" % (name, size, off))
    names = [m[0] for m in members]
    if names[:1] != ["debian-binary"]:
        print("   !! erstes Mitglied muss debian-binary sein")
    if len(names) < 3 or not names[1].startswith("control.tar") \
            or not names[2].startswith("data.tar"):
        print("   !! Reihenfolge muss debian-binary, control.tar.gz, data.tar.gz sein")
    with open(path, "rb") as f:
        for name, off, size in members:
            f.seek(off)
            blob = f.read(size)
            if name == "debian-binary":
                print("== debian-binary: %s" % blob.decode().strip())
            elif name.startswith("control.tar"):
                tf = tarfile.open(fileobj=io.BytesIO(blob), mode="r:*")
                print("== control.tar.gz: %s" % ", ".join(tf.getnames()))
                ctl = tf.extractfile("./control")
                print("== control")
                for line in ctl.read().decode("utf-8").splitlines():
                    print("   " + (line[:100] + " ..." if len(line) > 100 else line))
            elif name.startswith("data.tar"):
                tf = tarfile.open(fileobj=io.BytesIO(blob), mode="r:*")
                print("== data.tar.gz")
                for ti in tf:
                    print("   %s %-10s %9d %s" % (
                        "d" if ti.isdir() else "-", oct(ti.mode)[2:].rjust(4, "0"),
                        ti.size, ti.name))
    return 0


def main(argv):
    if len(argv) == 3 and argv[1] == "--info":
        return info(argv[2])
    if len(argv) != 3:
        sys.stderr.write(__doc__)
        return 2
    root, out = argv[1], argv[2]
    ctl_path = os.path.join(root, "DEBIAN", "control")
    if not os.path.isfile(ctl_path):
        sys.stderr.write("Fehler: %s fehlt\n" % ctl_path)
        return 1

    dirs, files = collect(root)
    if not files:
        sys.stderr.write("Fehler: %s enthaelt ausser DEBIAN nichts\n" % root)
        return 1

    # --- data.tar.gz --------------------------------------------------------
    data_entries = []
    for d in dirs:
        data_entries.append((d, True, 0o755, None))
    md5lines = []
    total = 0
    for f in files:
        p = os.path.join(root, f)
        with open(p, "rb") as fh:
            blob = fh.read()
        mode = 0o755 if os.stat(p).st_mode & stat.S_IXUSR else 0o644
        data_entries.append((f, False, mode, blob))
        md5lines.append("%s  %s\n" % (hashlib.md5(blob).hexdigest(), f))
        total += len(blob)
    # Verzeichnisse zuerst, dann Dateien - so schreibt es auch dpkg-deb.
    data_entries.sort(key=lambda e: (not e[1], e[0]))
    data_tgz = tar_gz(data_entries)

    # --- control.tar.gz -----------------------------------------------------
    with open(ctl_path, "rb") as fh:
        ctl = fh.read().decode("utf-8")
    installed_kib = (total + 1023) // 1024
    lines = []
    seen = False
    for line in ctl.splitlines():
        if line.startswith("Installed-Size:"):
            lines.append("Installed-Size: %d" % installed_kib)
            seen = True
        else:
            lines.append(line)
    if not seen:
        # hinter Architecture einfuegen, sonst ans Ende der Kopffelder
        ins = len(lines)
        for i, line in enumerate(lines):
            if line.startswith("Architecture:"):
                ins = i + 1
                break
        lines.insert(ins, "Installed-Size: %d" % installed_kib)
    ctl = "\n".join(lines).rstrip("\n") + "\n"

    ctl_entries = [
        ("control", False, 0o644, ctl.encode("utf-8")),
        ("md5sums", False, 0o644, "".join(md5lines).encode("utf-8")),
    ]

    # digsigsums: ohne diese Liste sagt aegis beim Installieren
    # "<for> refers to '...', not installed by that package -- ignored"
    # und vergibt die angeforderte Kennung nicht. Sie nennt je Datei die
    # beanspruchte Herkunft, den SHA1 und den Pfad -- nachgebaut aus dem
    # echten modRana-Paket, das auf demselben Weg die Location-Kennung
    # bekommt. Nur ausfuehrbare Dateien brauchen einen Eintrag.
    herkunft = "com.nokia.maemo"
    digsig = []
    for f in files:
        voll = os.path.join(root, f)
        if not (os.stat(voll).st_mode & stat.S_IXUSR):
            continue
        with open(voll, "rb") as fh:
            summe = hashlib.sha1(fh.read()).hexdigest()
        digsig.append("S %d %s H %d %s R %d %s\n"
                      % (len(herkunft), herkunft, len(summe), summe, len(f), f))
    if digsig:
        ctl_entries.append(("digsigsums", False, 0o644, "".join(digsig).encode("utf-8")))
    for extra in ("preinst", "postinst", "prerm", "postrm"):
        p = os.path.join(root, "DEBIAN", extra)
        if os.path.isfile(p):
            with open(p, "rb") as fh:
                ctl_entries.append((extra, False, 0o755, fh.read()))
    ctl_entries.sort(key=lambda e: e[0])
    ctl_tgz = tar_gz(ctl_entries)

    aegis_path = os.path.join(root, "DEBIAN", "_aegis")
    aegis = None
    if os.path.isfile(aegis_path):
        with open(aegis_path, "rb") as fh:
            aegis = fh.read()

    # --- ar-Huelle ----------------------------------------------------------
    tmp = out + ".new"
    with open(tmp, "wb") as f:
        f.write(b"!<arch>\n")
        f.write(ar_member("debian-binary", b"2.0\n"))
        f.write(ar_member("control.tar.gz", ctl_tgz))
        f.write(ar_member("data.tar.gz", data_tgz))
        # Das aegis-Manifest ist ein gewoehnliches viertes ar-Glied hinter
        # data.tar.gz -- genau so, wie modRana es mit "ar q" anhaengt
        # (nachgesehen in modrana_0.43.4_armel.deb). Ohne dieses Glied
        # bekommt das Paket auf Harmattan keine Ortung: der Sitzungs-D-Bus
        # laesst nur Prozesse mit der Kennung "Location" an
        # com.nokia.positioningd.client heran.
        if aegis is not None:
            f.write(ar_member("_aegis", aegis))
    os.replace(tmp, out)

    print("%s: %d B  (Nutzdaten %d B in %d Dateien, Installed-Size %d KiB)"
          % (out, os.path.getsize(out), total, len(files), installed_kib))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
