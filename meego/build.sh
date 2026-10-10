#!/bin/sh
# Baut die MeeGo-Harmattan-Fassung (Nokia N9/N950) auf dem Baurechner, im
# dorthin gespiegelten Quellbaum:
#
#   meego/build.sh arm     das Programm fuers Geraet -> build/meego/arm/duolingo
#   meego/build.sh check   den QML-Pruefer gegen das Desktop-Qt-4 des SDK
#   meego/build.sh test    die Probe fuer den Benoter (laeuft ohne Geraet)
#   meego/build.sh schach  die Probe fuer Brett, Zuege und Gegner
#
# Der ARM-Bau braucht die GCC-Cross-Toolchain (XGCC) und das MADDE-Sysroot
# (SYSROOT); moc und lrelease kommen aus dem Qt 4.7.4 des Simulators, also
# aus der Fassung, die auf dem Geraet laeuft. libstdc++ wird statisch
# gebunden und bleibt privat, damit Qt auf dem Geraet bei seinem eigenen
# GCC 4.4 bleibt.
set -e

HERE=$(cd "$(dirname "$0")/.." && pwd)
MODE=${1:-arm}
XGCC=${XGCC:-/tmp/xgcc-harmattan}
SYSROOT=${SYSROOT:-$HOME/QtSDK/Madde/sysroots/harmattan_sysroot_10.2011.34-1_slim}
SIMQT=${SIMQT:-$HOME/QtSDK/Simulator/Qt/gcc}
JOBS=${JOBS:-8}
VERSION=${VERSION:-$(sh "$HERE/meego/version.sh")}
OUT=$HERE/build/meego/$MODE
mkdir -p "$OUT"

APP_SRC="src/benoter.cpp \
 src/bot.cpp \
 src/core/Position.cpp \
 src/format.cpp \
 src/json.cpp \
 src/konto.cpp \
 src/lektion.cpp \
 src/netz.cpp \
 src/schach.cpp \
 meego/main.cpp"

# Kopfdateien mit einem Q_OBJECT darin.
MOC_HEADERS="src/konto.h src/lektion.h src/netz.h src/schach.h"

INCLUDES="-I$HERE -I$HERE/src -I$HERE/src/core -I$HERE/meego \
 -I$HERE/third_party/chess-library"
DEFINES="-DAPP_VERSION='\"$VERSION\"' -DQT_NO_DEBUG"

COMMON_FLAGS="-O2 -Wall -Wno-deprecated-declarations -Wno-unused-parameter \
 $DEFINES $INCLUDES"
# C++17, weil die Schachbibliothek (third_party/chess-library) es braucht.
# Qt 4.7 laesst sich damit uebersetzen, wenn man ihm die alten Schluesselwoerter
# nachsieht -- dieselben Schalter wie in harbour-schachlehrer.
CXX_ONLY="-std=gnu++17 -Wno-register -Wno-nonnull"

QT4_MODULES="QtCore QtGui QtNetwork QtDeclarative"

case "$MODE" in
arm)
    CXX=$XGCC/bin/arm-none-linux-gnueabi-g++
    [ -x "$CXX" ] || { echo "Cross-Compiler fehlt: $CXX" >&2; exit 1; }
    MOC=$SIMQT/bin/moc
    LRELEASE=$SIMQT/bin/lrelease
    QTINC=$SYSROOT/usr/include/qt4
    CXXFLAGS="--sysroot=$SYSROOT $COMMON_FLAGS $CXX_ONLY -I$QTINC"
    for m in $QT4_MODULES; do CXXFLAGS="$CXXFLAGS -I$QTINC/$m"; done
    # Harmattan ist hard-float, hat aber den alten Ladernamen behalten; das
    # statische libstdc++ des neueren GCC bleibt privat im Programm.
    LDFLAGS="--sysroot=$SYSROOT -static-libstdc++ -static-libgcc -Wl,-O1 -Wl,--as-needed \
 -Wl,--exclude-libs,ALL -Wl,--dynamic-linker=/lib/ld-linux.so.3"
    LIBS="-lQtDeclarative -lQtGui -lQtNetwork -lQtCore -lpthread"
    ;;
check)
    # Der QML-Pruefer baut gegen das schlichte Desktop-Qt des SDK (4.8.1):
    # das Qt des Simulators bricht ohne Anzeige ab, auch fuer ein Programm
    # ohne Fenster, und das Geraete-Qt laeuft hier nicht.
    CXX=${CXX:-g++}
    PROBEQT=${PROBEQT:-$HOME/QtSDK/Desktop/Qt/4.8.1/gcc}
    [ -x "$PROBEQT/bin/moc" ] || { echo "Qt 4.8 des SDK fehlt: $PROBEQT" >&2; exit 1; }
    MOC=$PROBEQT/bin/moc
    LRELEASE=$PROBEQT/bin/lrelease
    QTINC=$PROBEQT/include
    CXXFLAGS="$COMMON_FLAGS $CXX_ONLY -I$QTINC"
    for m in $QT4_MODULES; do CXXFLAGS="$CXXFLAGS -I$QTINC/$m"; done
    LDFLAGS="-L$PROBEQT/lib -Wl,-rpath,$PROBEQT/lib"
    LIBS="-lQtDeclarative -lQtGui -lQtNetwork -lQtCore -lpthread"
    APP_SRC="meego/tests/qml_check.cpp"
    MOC_HEADERS="meego/tests/stubs.h"
    ;;
test)
    # Die Probe fuer den Benoter: nur QtCore, kein Fenster, keine
    # Attrappen -- sie rechnet mit dem echten Code gegen echte Rohdaten.
    CXX=${CXX:-g++}
    PROBEQT=${PROBEQT:-$HOME/QtSDK/Desktop/Qt/4.8.1/gcc}
    [ -x "$PROBEQT/bin/moc" ] || { echo "Qt 4.8 des SDK fehlt: $PROBEQT" >&2; exit 1; }
    MOC=$PROBEQT/bin/moc
    LRELEASE=$PROBEQT/bin/lrelease
    QTINC=$PROBEQT/include
    CXXFLAGS="$COMMON_FLAGS $CXX_ONLY -I$QTINC -I$QTINC/QtCore"
    LDFLAGS="-L$PROBEQT/lib -Wl,-rpath,$PROBEQT/lib"
    LIBS="-lQtCore"
    APP_SRC="src/benoter.cpp src/json.cpp meego/tests/benoter_test.cpp"
    MOC_HEADERS=""
    ;;
schach)
    # Die Schachprobe: Brett, Zuege, Gegner -- ganz ohne Qt.
    CXX=${CXX:-g++}
    CXXFLAGS="-O2 -Wall $DEFINES $INCLUDES $CXX_ONLY"
    LDFLAGS=""
    LIBS=""
    MOC=true
    APP_SRC="src/bot.cpp src/core/Position.cpp meego/tests/schach_test.cpp"
    MOC_HEADERS=""
    ;;
*)
    echo "Aufruf: $0 arm|check|test|schach" >&2; exit 2 ;;
esac

MK=$OUT/Makefile
{
    echo "CXX=$CXX"; echo "MOC=$MOC"
    echo "CXXFLAGS=$CXXFLAGS"
    echo "LDFLAGS=$LDFLAGS"; echo "LIBS=$LIBS"; echo "SRC=$HERE"; echo
    objs=
    for s in $APP_SRC; do
        o=$(basename "$s" .cpp).o; objs="$objs $o"
        echo "$o: \$(SRC)/$s"; printf '\t$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@\n'
    done
    for h in $MOC_HEADERS; do
        base=$(basename "$h" .h)
        o=moc_$base.o; objs="$objs $o"
        echo "moc_$base.cpp: \$(SRC)/$h"; printf '\t$(MOC) $(filter -I%%,$(CXXFLAGS)) $< -o $@\n'
        echo "$o: moc_$base.cpp"; printf '\t$(CXX) $(CXXFLAGS) -c $< -o $@\n'
    done
    echo
    echo "duolingo:$objs"; printf '\t$(CXX) $(LDFLAGS) -o $@ %s $(LIBS)\n' "$objs"
    echo
    echo "-include \$(wildcard *.d)"
} > "$MK"

make -C "$OUT" -f Makefile -j"$JOBS" duolingo

if [ "$MODE" = "schach" ]; then
    "$OUT/duolingo"
    exit $?
fi

if [ "$MODE" = "test" ]; then
    "$OUT/duolingo" "$HERE"/doc/sitzung-*.json "$HERE"/doc/abgabe-*.json
    exit $?
fi

if [ -f "$HERE/translations/duolingo-de.ts" ] && [ -x "$LRELEASE" ]; then
    mkdir -p "$OUT/translations"
    "$LRELEASE" "$HERE/translations/duolingo-de.ts" \
        -qm "$OUT/translations/duolingo-de.qm"
fi

ls -la "$OUT/duolingo"
