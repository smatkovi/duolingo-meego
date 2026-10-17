#!/bin/sh
# Legt das Repo auf GitHub an und schiebt den Baum hinauf.
#
#   tools/repo-anlegen.sh [--oeffentlich]
#
# `gh` ist auf dem Baurechner angemeldet, nicht hier -- deshalb wandert
# der ganze Baum (samt .git) dorthin und die Arbeit passiert per ssh.
# Standard ist **privat**: das laesst sich mit einem Befehl umdrehen,
# umgekehrt nicht.
set -e
HERE=$(cd "$(dirname "$0")/.." && pwd)
NAME=${NAME:-duolingo-meego}
SICHT=--private
[ "$1" = "--oeffentlich" ] && SICHT=--public

if [ -n "$BUILD_HOST" ]; then
    HOST=$BUILD_HOST
elif ssh -o BatchMode=yes -o ConnectTimeout=4 sebastian@192.168.1.21 true 2>/dev/null; then
    HOST=sebastian@192.168.1.21
else
    HOST=arch
fi
FERN=/tmp/$NAME-repo
echo "Baurechner: $HOST"

ssh "$HOST" "mkdir -p $FERN"
# Diesmal **mit** .git -- es geht ja um die Vorgeschichte.
rsync -a --partial --delete --exclude /build --exclude .wegwerf.json \
      "$HERE/" "$HOST:$FERN/"

ssh "$HOST" "cd $FERN && gh repo create $NAME $SICHT --source=. --remote=origin --push"
ssh "$HOST" "cd $FERN && gh repo view --json url,visibility -q '.url + \"  (\" + .visibility + \")\"'"
