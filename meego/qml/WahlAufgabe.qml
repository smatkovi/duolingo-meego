import QtQuick 1.1
import com.nokia.meego 1.0

// Eine aus mehreren Antworten: assist, judge, select, form, gapFill,
// characterSelect, definition, readComprehension …
Item {
    id: flaeche

    property variant aufgabe
    property int gewaehlt: -1
    property bool bereit: gewaehlt >= 0

    function reihe() { return gewaehlt >= 0 ? [gewaehlt] : [] }
    function text() { return "" }

    // Neue Aufgabe, nichts angetippt.
    onAufgabeChanged: gewaehlt = -1

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
                    groesse: 48
                }
                Text {
                    width: spalte.width - (flaeche.aufgabe.ton ? 60 : 0)
                    text: flaeche.aufgabe.frage ? flaeche.aufgabe.frage : ""
                    visible: text !== ""
                    wrapMode: Text.WordWrap
                    font.pixelSize: 28
                    color: "#ffffff"
                }
            }

            Text {
                width: parent.width
                visible: text !== ""
                text: flaeche.aufgabe.text ? flaeche.aufgabe.text : ""
                wrapMode: Text.WordWrap
                font.pixelSize: 20
                color: "#c8c8c8"
            }

            Text {
                width: parent.width
                visible: text !== ""
                text: flaeche.aufgabe.frage2 ? flaeche.aufgabe.frage2 : ""
                wrapMode: Text.WordWrap
                font.pixelSize: 24
                color: "#ffffff"
            }

            Text {
                width: parent.width
                visible: text !== ""
                text: flaeche.aufgabe.satz ? flaeche.aufgabe.satz : ""
                wrapMode: Text.WordWrap
                font.pixelSize: 26
                color: "#ffffff"
            }

            Repeater {
                model: flaeche.aufgabe.wahlen

                Rectangle {
                    width: spalte.width
                    height: Math.max(modelData.bild ? 96 : 72, zeile.height + 24)
                    radius: 8
                    color: flaeche.gewaehlt === index ? "#1e5a7a" : "#262626"
                    border.color: flaeche.gewaehlt === index
                                  ? "#4fc3f7" : "#3a3a3a"
                    border.width: 2

                    Row {
                        id: zeile
                        anchors { left: parent.left; leftMargin: 14
                                  right: parent.right; rightMargin: 14
                                  verticalCenter: parent.verticalCenter }
                        spacing: 12

                        // `select` bringt zum Wort ein Bild mit -- und
                        // zwar als SVG; gezeigt wird die PNG-Fassung
                        // derselben Kennung (siehe src/lektion.cpp).
                        Image {
                            source: modelData.bild ? modelData.bild : ""
                            visible: source != ""
                            width: visible ? 72 : 0
                            height: 72
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                            asynchronous: true
                        }

                        Text {
                            width: zeile.width - (modelData.bild ? 84 : 0)
                            text: modelData.text
                            wrapMode: Text.WordWrap
                            verticalAlignment: Text.AlignVCenter
                            height: parent.height
                            font.pixelSize: 26
                            color: "#ffffff"
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            flaeche.gewaehlt = index
                            if (modelData.ton)
                                tonspur.quelle = modelData.ton
                        }
                    }
                }
            }
        }
    }

    // Ein Spieler fuer die angetippte Wahl -- die Woerter haben eigene
    // Tonspuren, aber nebeneinander stehende Lautsprecher braucht niemand.
    Ton { id: tonspur; vonSelbst: true; visible: false }
}
