#!/bin/sh
# Schiebt den Stand ins Repo.
#
#   tools/repo-schieben.sh
#
# `gh` und der Schluessel liegen auf dem Baurechner, nicht hier -- also
# wandert der Baum dorthin und wird von dort geschoben.
#
# **`.git/config` bleibt beim Abgleich aussen vor**: dort drueben steht
# die Fernverbindung drin, hier nicht, und ein Abgleich wuerde sie jedes
# Mal wieder wegputzen.
set -e
HERE=$(cd "$(dirname "$0")/.." && pwd)
NAME=${NAME:-duolingo-meego}
FERN=/tmp/$NAME-repo

if [ -n "$BUILD_HOST" ]; then
    HOST=$BUILD_HOST
elif ssh -o BatchMode=yes -o ConnectTimeout=4 sebastian@192.168.1.21 true 2>/dev/null; then
    HOST=sebastian@192.168.1.21
else
    HOST=arch
fi
echo "Baurechner: $HOST"

rsync -a --partial --delete --exclude /build --exclude .wegwerf.json \
      --exclude .git/config "$HERE/" "$HOST:$FERN/"
ssh "$HOST" "cd $FERN && git remote get-url origin >/dev/null 2>&1 || \
    git remote add origin git@github.com:smatkovi/$NAME.git; \
    git push -q origin HEAD:master && git log --oneline | head -1"
