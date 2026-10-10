import QtQuick 1.1
import com.nokia.meego 1.0

// Zwei Spalten zusammenbringen: match, listenMatch, characterMatch.
//
// Links das Wort in der gelernten Sprache (bei listenMatch mit Ton), rechts
// die Uebersetzung. Ein Paar, das zusammengehoert, verschwindet; ein
// falsches blinkt kurz rot. Fertig ist die Aufgabe, wenn nichts mehr
// dasteht -- erst dann wird der Knopf unten scharf.
Item {
    id: flaeche

    property variant aufgabe
    property int linksGewaehlt: -1
    property int rechtsGewaehlt: -1
    property int offen: 0
    property int fehler: 0
    property variant erledigt: []
    property bool bereit: offen === 0 && aufgabe.links !== undefined

    // Die Weboberflaeche benotet diese Art als richtig, sobald eine Antwort
    // dasteht; die Darstellung laesst nur richtige Paare zu.
    function reihe() { return [] }
    function text() { return bereit ? "fertig" : "" }

    onAufgabeChanged: {
        erledigt = []
        linksGewaehlt = -1
        rechtsGewaehlt = -1
        fehler = 0
        offen = aufgabe.links !== undefined ? aufgabe.links.length : 0
    }

    function istWeg(paar) {
        for (var i = 0; i < erledigt.length; ++i)
            if (erledigt[i] === paar)
                return true
        return false
    }

    function pruefen() {
        if (linksGewaehlt < 0 || rechtsGewaehlt < 0)
            return
        if (linksGewaehlt === rechtsGewaehlt) {
            var neu = erledigt
            neu.push(linksGewaehlt)
            erledigt = neu
            offen = offen - 1
        } else {
            fehler = fehler + 1
        }
        linksGewaehlt = -1
        rechtsGewaehlt = -1
    }

    Flickable {
        anchors.fill: parent
        contentHeight: reihen.height + 32
        clip: true

        Row {
            id: reihen
            anchors { left: parent.left; leftMargin: 16; right: parent.right
                      rightMargin: 16; top: parent.top; topMargin: 8 }
            spacing: 12

            Column {
                id: linkeSpalte
                width: (reihen.width - 12) / 2
                spacing: 10

                Repeater {
                    model: flaeche.aufgabe.links

                    Rectangle {
                        width: linkeSpalte.width
                        height: 64
                        radius: 8
                        visible: !flaeche.istWeg(modelData.paar)
                        color: flaeche.linksGewaehlt === modelData.paar
                               ? "#1e5a7a" : "#262626"
                        border.color: flaeche.linksGewaehlt === modelData.paar
                                      ? "#4fc3f7" : "#3a3a3a"
                        border.width: 2

                        Text {
                            anchors { fill: parent; margins: 8 }
                            text: modelData.text ? modelData.text : "▶"
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            wrapMode: Text.WordWrap
                            font.pixelSize: 22
                            color: "#ffffff"
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                flaeche.linksGewaehlt = modelData.paar
                                if (modelData.ton)
                                    tonspur.quelle = modelData.ton
                                flaeche.pruefen()
                            }
                        }
                    }
                }
            }

            Column {
                id: rechteSpalte
                width: (reihen.width - 12) / 2
                spacing: 10

                Repeater {
                    model: flaeche.aufgabe.rechts

                    Rectangle {
                        width: rechteSpalte.width
                        height: 64
                        radius: 8
                        visible: !flaeche.istWeg(modelData.paar)
                        color: flaeche.rechtsGewaehlt === modelData.paar
                               ? "#1e5a7a" : "#262626"
                        border.color: flaeche.rechtsGewaehlt === modelData.paar
                                      ? "#4fc3f7" : "#3a3a3a"
                        border.width: 2

                        Text {
                            anchors { fill: parent; margins: 8 }
                            text: modelData.text
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            wrapMode: Text.WordWrap
                            font.pixelSize: 22
                            color: "#ffffff"
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                flaeche.rechtsGewaehlt = modelData.paar
                                flaeche.pruefen()
                            }
                        }
                    }
                }
            }
        }
    }

    Ton { id: tonspur; vonSelbst: true; visible: false }
}
