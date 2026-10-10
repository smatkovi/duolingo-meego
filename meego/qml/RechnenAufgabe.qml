import QtQuick 1.1
import com.nokia.meego 1.0

// Eine Rechenaufgabe: oben die Anweisung, darunter die Formel mit der
// Luecke, unten die Eingabe -- je nach Art eine Wahl, ein Feld zum
// Eintippen oder Bausteine, die in die Zeile wandern.
Item {
    id: flaeche

    property variant aufgabe
    property int gewaehlt: -1
    property variant gelegt: []

    property string art: aufgabe.eingabe ? aufgabe.eingabe : "tippen"
    property bool bereit: art === "wahl" ? gewaehlt >= 0
                        : art === "bauen" ? gelegt.length > 0
                        : feld.text.length > 0

    function reihe() {
        if (art === "wahl")
            return gewaehlt >= 0 ? [gewaehlt] : []
        if (art === "bauen")
            return gelegt
        return []
    }
    function text() { return art === "tippen" ? feld.text : "" }

    onAufgabeChanged: {
        gewaehlt = -1
        gelegt = []
        feld.text = ""
    }

    function legen(nummer) {
        var neu = gelegt
        neu.push(nummer)
        gelegt = neu
    }
    function wegnehmen(stelle) {
        var neu = []
        for (var i = 0; i < gelegt.length; ++i)
            if (i !== stelle)
                neu.push(gelegt[i])
        gelegt = neu
    }
    function bausteinText(nummer) {
        return flaeche.aufgabe.wahlen[nummer].text
    }

    Flickable {
        anchors.fill: parent
        contentHeight: spalte.height + 32
        clip: true

        Column {
            id: spalte
            anchors { left: parent.left; leftMargin: 16; right: parent.right
                      rightMargin: 16; top: parent.top; topMargin: 8 }
            spacing: 18

            Text {
                width: parent.width
                text: flaeche.aufgabe.anweisung ? flaeche.aufgabe.anweisung : ""
                visible: text !== ""
                wrapMode: Text.WordWrap
                font.pixelSize: 22
                color: "#9e9e9e"
            }

            // Die Formel.
            Flow {
                width: parent.width
                spacing: 8

                Repeater {
                    model: flaeche.aufgabe.zeile

                    Rectangle {
                        height: 64
                        width: Math.max(44, teil.width + 20)
                        radius: 6
                        color: modelData.luecke ? "#1e5a7a" : "transparent"
                        border.color: modelData.luecke ? "#4fc3f7" : "transparent"
                        border.width: 2

                        Text {
                            id: teil
                            anchors.centerIn: parent
                            text: modelData.luecke
                                  ? (flaeche.art === "bauen"
                                     ? "?" : (flaeche.art === "tippen"
                                              ? (feld.text !== "" ? feld.text : "?")
                                              : (flaeche.gewaehlt >= 0
                                                 ? flaeche.bausteinText(flaeche.gewaehlt)
                                                 : "?")))
                                  : modelData.text
                            font.pixelSize: 34
                            color: "#ffffff"
                        }
                    }
                }
            }

            // Was gelegt wurde (nur beim Bauen).
            Flow {
                width: parent.width
                spacing: 8
                visible: flaeche.art === "bauen"

                Repeater {
                    model: flaeche.gelegt.length

                    Rectangle {
                        height: 56
                        width: gelegtText.width + 26
                        radius: 6
                        color: "#2f2f2f"
                        border.color: "#5a5a5a"
                        border.width: 1

                        Text {
                            id: gelegtText
                            anchors.centerIn: parent
                            text: flaeche.bausteinText(flaeche.gelegt[index])
                            font.pixelSize: 28
                            color: "#ffffff"
                        }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: flaeche.wegnehmen(index)
                        }
                    }
                }
            }

            // Eintippen.
            TextField {
                id: feld
                width: parent.width
                visible: flaeche.art === "tippen"
                placeholderText: flaeche.aufgabe.platzhalter
                                 ? flaeche.aufgabe.platzhalter : qsTr("Antwort")
                inputMethodHints: Qt.ImhFormattedNumbersOnly
            }

            // Wahl oder Bausteine.
            Flow {
                width: parent.width
                spacing: 10
                visible: flaeche.art !== "tippen"

                Repeater {
                    model: flaeche.aufgabe.wahlen

                    Rectangle {
                        height: 64
                        width: Math.max(64, wahlText.width + 28)
                        radius: 8
                        visible: flaeche.art !== "bauen"
                                 || !flaeche.schonGelegt(index)
                        color: flaeche.art === "wahl" && flaeche.gewaehlt === index
                               ? "#1e5a7a" : "#262626"
                        border.color: flaeche.art === "wahl"
                                      && flaeche.gewaehlt === index
                                      ? "#4fc3f7" : "#3a3a3a"
                        border.width: 2

                        Text {
                            id: wahlText
                            anchors.centerIn: parent
                            text: modelData.text
                            font.pixelSize: 30
                            color: "#ffffff"
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                if (flaeche.art === "bauen")
                                    flaeche.legen(index)
                                else
                                    flaeche.gewaehlt = index
                            }
                        }
                    }
                }
            }
        }
    }

    function schonGelegt(nummer) {
        for (var i = 0; i < gelegt.length; ++i)
            if (gelegt[i] === nummer)
                return true
        return false
    }
}
