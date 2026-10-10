#!/bin/sh
# Die Fassung, die der Bau in Paket und Programm stempelt. Hochzaehlen bei
# jedem neuen .deb -- sonst schreibt build-deb.sh stillschweigend dasselbe
# Paket zweimal und dpkg sieht keine Neuerung.
echo "0.1.0-meego3"
