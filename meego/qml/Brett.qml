import QtQuick 1.1

// Das Schachbrett: 64 Felder ueber einem GridView, die Figuren als SVG.
//
// Nach dem Vorbild von `harbour-schachlehrer` (meego/qml/Board.qml, GPL-3,
// selber Verfasser) -- hier ohne dessen Einstellungen und Farbtafeln, und
// mit `Schach` als Gegenueber statt dem Lehrer.
//
// Feldnummern wie in der Schachbibliothek: 0 = a1, 7 = h1, 63 = h8.
// Gezogen wird mit zwei Tippern (erst die Figur, dann das Ziel); gezogene
// Figuren verdecken unter dem Finger das Feld, auf das sie sollen.
Item {
    id: brett

    property variant felder: Schach.felder
    property variant ziele: Schach.ziele
    property int gewaehlt: Schach.gewaehlt
    property bool gedreht: !Schach.untenWeiss
    property bool bedienbar: Schach.ichAmZug

    property real seite: Math.min(width, height)
    property real feldseite: seite / 8

    function feldVonZelle(zelle) {
        var reihe = Math.floor(zelle / 8)      // 0 ist oben
        var linie = zelle % 8
        if (gedreht) {
            reihe = 7 - reihe
            linie = 7 - linie
        }
        return (7 - reihe) * 8 + linie
    }

    function istZiel(feld) {
        if (!ziele)
            return false
        for (var i = 0; i < ziele.length; ++i)
            if (ziele[i] === feld)
                return true
        return false
    }

    function figurAuf(feld) {
        if (!felder || felder.length !== 64 || feld < 0 || feld > 63)
            return ""
        return felder[feld]
    }

    // Der letzte Zug kommt als "e2e4"; beide Felder werden markiert.
    function feldAusNamen(name) {
        if (!name || name.length < 2)
            return -1
        var l = "abcdefgh".indexOf(name.charAt(0))
        var r = parseInt(name.charAt(1), 10) - 1
        if (l < 0 || isNaN(r) || r < 0 || r > 7)
            return -1
        return r * 8 + l
    }
    property int vonFeld: feldAusNamen(("" + Schach.letzterZug).substring(0, 2))
    property int nachFeld: feldAusNamen(("" + Schach.letzterZug).substring(2, 4))

    ListModel {
        id: zellen
        Component.onCompleted: {
            for (var i = 0; i < 64; ++i)
                zellen.append({ "zelle": i })
        }
    }

    Rectangle {
        id: rahmen
        width: brett.seite
        height: brett.seite
        anchors.centerIn: parent
        color: "#3a2a1a"
        border.color: "#3a2a1a"
        border.width: Math.max(1, Math.round(brett.feldseite * 0.02))

        GridView {
            anchors { fill: parent; margins: rahmen.border.width }
            cellWidth: Math.floor(width / 8)
            cellHeight: cellWidth
            interactive: false
            clip: true
            model: zellen

            delegate: Item {
                id: zelle
                width: GridView.view.cellWidth
                height: GridView.view.cellHeight

                property int feld: brett.feldVonZelle(model.zelle)
                property bool dunkel: ((feld % 8) + Math.floor(feld / 8)) % 2 === 0
                property string figur: brett.figurAuf(feld)
                property bool ausgewaehlt: brett.gewaehlt === feld
                property bool zielfeld: brett.gewaehlt >= 0 && brett.istZiel(feld)
                property bool letzter: feld === brett.vonFeld
                                       || feld === brett.nachFeld

                Rectangle {
                    anchors.fill: parent
                    color: zelle.dunkel ? "#8b6b45" : "#e8d7b8"
                }

                Rectangle {
                    anchors.fill: parent
                    visible: zelle.letzter && !zelle.ausgewaehlt
                    color: "#ffc800"
                    opacity: 0.3
                }

                Rectangle {
                    anchors.fill: parent
                    visible: zelle.ausgewaehlt
                    color: "#4fc3f7"
                    opacity: 0.45
                }

                // Ein Punkt fuer ein moegliches Ziel, ein Ring, wenn dort
                // etwas zu schlagen ist.
                Rectangle {
                    anchors.centerIn: parent
                    visible: zelle.zielfeld && zelle.figur === ""
                    width: parent.width / 4
                    height: width
                    radius: width / 2
                    color: "#2e7d32"
                    opacity: 0.7
                }
                Rectangle {
                    anchors.fill: parent
                    visible: zelle.zielfeld && zelle.figur !== ""
                    color: "transparent"
                    border.color: "#2e7d32"
                    border.width: Math.max(2, parent.width / 12)
                    opacity: 0.8
                }

                Image {
                    anchors { fill: parent; margins: parent.width * 0.06 }
                    visible: zelle.figur !== ""
                    smooth: true
                    fillMode: Image.PreserveAspectFit
                    sourceSize.width: parent.width
                    sourceSize.height: parent.height
                    source: zelle.figur === ""
                            ? ""
                            : "/opt/duolingo/figuren/Chess_"
                              + zelle.figur.charAt(1)
                              + (zelle.figur.charAt(0) === "w" ? "l" : "d")
                              + "t45.svg"
                }

                MouseArea {
                    anchors.fill: parent
                    enabled: brett.bedienbar
                    onClicked: Schach.feldAntippen(zelle.feld)
                }
            }
        }
    }
}
