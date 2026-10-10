#!/bin/sh
# Packt den ARM-Bau als Harmattan-.deb. Laeuft auf dem Baurechner nach
# "meego/build.sh arm" und "meego/build-rust.sh":
#
#   meego/build-deb.sh            -> build/meego/duolingo_<fassung>_armel.deb
#   VERSION=0.1.0-meego2 meego/build-deb.sh
#
# Auf dem Geraet:
#   /opt/duolingo/bin/duolingo
#   /opt/duolingo/bin/duo-holer           (HTTPS, siehe meego/fetch)
#   /opt/duolingo/qml
#   /usr/share/applications/duolingo.desktop
#   /usr/share/icons/hicolor/80x80/apps/duolingo.png
#
# mkdeb.py schreibt das .deb selbst: Harmattans dpkg ist 1.15 und will
# gzip-Glieder und keinen Schraegstrich an den ar-Namen -- GNU ar macht
# beides anders.
set -e

HERE=$(cd "$(dirname "$0")/.." && pwd)
PKG=$HERE/meego
OUT=$HERE/build/meego
BIN=$OUT/arm/duolingo
XGCC=${XGCC:-/tmp/xgcc-harmattan}
VERSION=${VERSION:-$(sh "$PKG/version.sh")}

[ -x "$BIN" ] || { echo "ARM-Programm fehlt: $BIN (erst meego/build.sh arm)" >&2; exit 1; }
FETCH=$OUT/fetch/duo-holer
[ -x "$FETCH" ] || { echo "duo-holer fehlt: $FETCH (erst meego/build-rust.sh)" >&2; exit 1; }

STAGE=$OUT/stage
rm -rf "$STAGE"
mkdir -p "$STAGE/DEBIAN" "$STAGE/opt/duolingo/bin" \
         "$STAGE/usr/share/applications" \
         "$STAGE/usr/share/icons/hicolor/80x80/apps" \
         "$STAGE/usr/share/themes/base/meegotouch/icons" \
         "$STAGE/usr/share/doc/duolingo"

cp "$BIN" "$STAGE/opt/duolingo/bin/duolingo"
"$XGCC/bin/arm-none-linux-gnueabi-strip" "$STAGE/opt/duolingo/bin/duolingo"
cp "$FETCH" "$STAGE/opt/duolingo/bin/duo-holer"
chmod 755 "$STAGE/opt/duolingo/bin/duolingo" "$STAGE/opt/duolingo/bin/duo-holer"

cp -a "$PKG/qml" "$STAGE/opt/duolingo/qml"
# Die Schachfiguren (cburnett, BSD-3-Clause -- siehe figuren/HERKUNFT.md).
mkdir -p "$STAGE/opt/duolingo/figuren"
cp "$HERE/figuren/"*.svg "$STAGE/opt/duolingo/figuren/"
if [ -d "$OUT/arm/translations" ]; then
    cp -a "$OUT/arm/translations" "$STAGE/opt/duolingo/translations"
fi

# Symbole: 80x80 fuer den Starter, 64x64 base64 im control fuer die
# Programmverwaltung. tools/icon-bauen.py schneidet beide auf die exakte
# Silhouette der Standard-Apps.
cp "$PKG/icons/icon-80.png" "$STAGE/usr/share/icons/hicolor/80x80/apps/duolingo.png"
cp "$PKG/icons/icon-80.png" "$STAGE/usr/share/themes/base/meegotouch/icons/duolingo-80.png"
cp "$PKG/duolingo.desktop" "$STAGE/usr/share/applications/duolingo.desktop"
cp "$HERE/LICENSE" "$STAGE/usr/share/doc/duolingo/copyright"
find "$STAGE" -type f ! -path "*/bin/*" -exec chmod 644 {} +
find "$STAGE" -type d -exec chmod 755 {} +

ICON=$(base64 -w 76 "$PKG/icons/icon-64.png" | sed 's/^/ /')
awk -v version="$VERSION" -v icon="$ICON" '
    { gsub(/@VERSION@/, version) }
    /^@ICON@$/ { print icon; next }
    { print }
' "$PKG/control.in" > "$STAGE/DEBIAN/control"

DEB=$OUT/duolingo_${VERSION}_armel.deb
python3 "$PKG/mkdeb.py" "$STAGE" "$DEB"
python3 "$PKG/mkdeb.py" --info "$DEB" | head -20
