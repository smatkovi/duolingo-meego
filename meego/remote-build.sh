#!/bin/sh
# Baut die MeeGo-Fassung auf dem Arch-Rechner und holt das .deb her.
#
#   meego/remote-build.sh
#   BUILD_HOST=sebastian@192.168.1.21 meego/remote-build.sh
#
# Alles Schwere passiert dort: Cross-Toolchain, MADDE-Sysroot und das
# musl-Ziel von Rust liegen auf dem Rechner, und ein langer Lauf darf nicht
# an der ssh-Sitzung des Telefons haengen.
set -e
HERE=$(cd "$(dirname "$0")/.." && pwd)
REMOTE=/tmp/duolingo/src
OUT=${OUT:-$HOME/ps/rpms/duolingo}

if [ -n "$BUILD_HOST" ]; then
    HOST=$BUILD_HOST
elif ssh -o BatchMode=yes -o ConnectTimeout=4 sebastian@192.168.1.21 true 2>/dev/null; then
    HOST=sebastian@192.168.1.21
else
    HOST=arch
fi
echo "Baurechner: $HOST"

ssh "$HOST" "mkdir -p $REMOTE"
# --exclude /build, nicht --exclude build*: letzteres frisst auch
# meego/build-*.sh.
rsync -a --partial --delete --exclude .git --exclude /build \
      --exclude .wegwerf.json "$HERE/" "$HOST:$REMOTE/"

ssh "$HOST" "tmux kill-session -t duolingo 2>/dev/null; \
    tmux new-session -d -s duolingo 'cd $REMOTE && \
      { sh meego/build-rust.sh && sh meego/build.sh arm && sh meego/build-deb.sh; } \
      > /tmp/duolingo-meego.log 2>&1; echo FERTIG_RC=\$? >> /tmp/duolingo-meego.log'"
echo "tmux-Sitzung duolingo laeuft, Log /tmp/duolingo-meego.log"

while ssh "$HOST" "tmux has-session -t duolingo 2>/dev/null"; do
    sleep 20
done
ssh "$HOST" "tail -20 /tmp/duolingo-meego.log"
ssh "$HOST" "grep -q 'FERTIG_RC=0' /tmp/duolingo-meego.log" || { echo "Bau fehlgeschlagen"; exit 1; }

mkdir -p "$OUT"
rsync -a --partial "$HOST:$REMOTE/build/meego/*.deb" "$OUT/"
ls -l "$OUT"/*.deb
