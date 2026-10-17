#!/bin/sh
# Haengt ein .deb an ein GitHub-Release.
#
#   tools/release-hochladen.sh 0.5.1-meego2 [datei.deb]
#
# Ohne Datei wird die passende aus ~/ps/rpms/duolingo genommen. Die Marke
# heisst `v<fassung>`; gibt es das Release schon, wird die Datei nur
# dazugelegt.
set -e
HERE=$(cd "$(dirname "$0")/.." && pwd)
NAME=${NAME:-duolingo-meego}
FASSUNG=${1:-$(sh "$HERE/meego/version.sh")}
DEB=${2:-$HOME/ps/rpms/duolingo/duolingo_${FASSUNG}_armel.deb}
[ -f "$DEB" ] || { echo "kein Paket: $DEB" >&2; exit 1; }

if [ -n "$BUILD_HOST" ]; then
    HOST=$BUILD_HOST
elif ssh -o BatchMode=yes -o ConnectTimeout=4 sebastian@192.168.1.21 true 2>/dev/null; then
    HOST=sebastian@192.168.1.21
else
    HOST=arch
fi
FERN=/tmp/$NAME-repo
echo "Baurechner: $HOST, Fassung $FASSUNG"

scp "$DEB" "$HOST:$FERN/" >/dev/null
BASIS=$(basename "$DEB")
ssh "$HOST" "cd $FERN && git fetch --quiet origin || true; \
  if gh release view v$FASSUNG >/dev/null 2>&1; then \
      gh release upload v$FASSUNG $BASIS --clobber; \
  else \
      gh release create v$FASSUNG $BASIS --title 'Duolingo fuer MeeGo $FASSUNG' \
         --notes-file RELEASE.md; \
  fi; \
  gh release view v$FASSUNG --json url -q .url"
