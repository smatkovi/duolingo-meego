#!/bin/sh
# Liest das MeeGo-QML mit dem QDeclarative von Qt 4 auf dem Baurechner ein.
#
#   meego/tests/check-qml.sh
#
# Faengt, was sonst erst am Geraet auffaellt: eine Eigenschaft, die es unter
# QtQuick 1.1 nicht gibt, eine vertippte Bindung, eine Datei, die gar nicht
# liest. Was aus com.nokia.meego und QtMultimediaKit kommt, wird
# ausgeblendet -- die gibt es auf dem Baurechner nicht, auf dem Geraet schon.
set -e
HERE=$(cd "$(dirname "$0")/../.." && pwd)
sh "$HERE/meego/build.sh" check
"$HERE/build/meego/check/duolingo" "$HERE/meego/qml"
