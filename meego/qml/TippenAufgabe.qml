import QtQuick 1.1
import com.nokia.meego 1.0

// Frei eintippen: name, listen, listenComplete, typeComplete …
// Benotet wird gegen den Graphen, den die Aufgabe mitbringt (src/benoter.h).
Item {
    id: flaeche

    property variant aufgabe
    property bool bereit: feld.text.length > 0

    function reihe() { return [] }
    function text() { return feld.text }

    // Neue Aufgabe, leeres Feld.
    onAufgabeChanged: feld.text = ""

    Flickable {
        anchors.fill: parent
        contentHeight: spalte.height + 32
        clip: true

        Column {
            id: spalte
            anchors { left: parent.left; leftMargin: 16; right: parent.right
                      rightMargin: 16; top: parent.top; topMargin: 8 }
            spacing: 14

            Row {
                spacing: 12
                Ton {
                    quelle: flaeche.aufgabe.ton ? flaeche.aufgabe.ton : ""
                    groesse: 56
                    vonSelbst: true
                }
                Ton {
                    quelle: flaeche.aufgabe.tonLangsam
                            ? flaeche.aufgabe.tonLangsam : ""
                    groesse: 44
                }
            }

            Image {
                source: flaeche.aufgabe.bild ? flaeche.aufgabe.bild : ""
                visible: source != ""
                width: Math.min(sourceSize.width, 200)
                fillMode: Image.PreserveAspectFit
                smooth: true
            }

            Text {
                width: parent.width
                visible: text !== ""
                text: flaeche.aufgabe.frage ? flaeche.aufgabe.frage : ""
                wrapMode: Text.WordWrap
                font.pixelSize: 30
                color: "#ffffff"
            }

            Text {
                width: parent.width
                visible: text !== ""
                text: flaeche.aufgabe.satz ? flaeche.aufgabe.satz : ""
                wrapMode: Text.WordWrap
                font.pixelSize: 26
                color: "#c8c8c8"
            }

            TextField {
                id: feld
                width: parent.width
                placeholderText: qsTr("Antwort")
                inputMethodHints: Qt.ImhNoPredictiveText
            }
        }
    }
}
