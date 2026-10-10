#!/bin/sh
# Baut duo-holer (meego/fetch) fuer das N9/N950 -- statisch gegen musl, auf
# dem Baurechner. meego/remote-build.sh ruft das dort auf.
#
#   meego/build-rust.sh     -> build/meego/fetch/duo-holer
set -e
HERE=$(cd "$(dirname "$0")/.." && pwd)
. "$HERE/meego/cross.env"
[ -x "$CARGO_HOME/bin/cargo" ] || { echo "cargo fehlt unter $CARGO_HOME" >&2; exit 1; }
cd "$HERE/meego/fetch"
CARGO_TARGET_DIR=${CARGO_TARGET_DIR:-/tmp/duo-holer-target}
export CARGO_TARGET_DIR
nice cargo build --release --target "$ZIEL"
mkdir -p "$HERE/build/meego/fetch"
cp "$CARGO_TARGET_DIR/$ZIEL/release/duo-holer" "$HERE/build/meego/fetch/duo-holer"
ls -la "$HERE/build/meego/fetch/duo-holer"
