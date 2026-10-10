import QtQuick 1.1
import com.nokia.meego 1.0

// Woerter in die richtige Reihenfolge tippen: translate, listenTap,
// tapComplete, syllableTap …
//
// Oben die Zeile, die entsteht, unten der Vorrat. Ein Wort wandert mit
// einem Tipper hin und mit einem zurueck. Kein Ziehen: auf dem
// kapazitiven Schirm des N9 ist Tippen treffsicherer, und die App macht
// es genauso.
Item {
    id: flaeche

    property variant aufgabe
    property variant gewaehlt: []
    property bool bereit: gewaehlt.length > 0

    function reihe() { return gewaehlt }
    function text() { return "" }

    function nehmen(nummer) {
        var neu = gewaehlt
        neu.push(nummer)
        gewaehlt = neu
    }

    function zuruecklegen(stelle) {
        var neu = []
        for (var i = 0; i < gewaehlt.length; ++i)
            if (i !== stelle)
                neu.push(gewaehlt[i])
        gewaehlt = neu
    }

    function schonGewaehlt(nummer) {
        for (var i = 0; i < gewaehlt.length; ++i)
            if (gewaehlt[i] === nummer)
                return true
        return false
    }

    function wortText(nummer) {
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
            spacing: 14

            Row {
                spacing: 12
                Ton {
                    quelle: flaeche.aufgabe.ton ? flaeche.aufgabe.ton : ""
                    groesse: 48
                }
                Ton {
                    quelle: flaeche.aufgabe.tonLangsam
                            ? flaeche.aufgabe.tonLangsam : ""
                    groesse: 40
                }
                Text {
                    text: flaeche.aufgabe.frage ? flaeche.aufgabe.frage : ""
                    visible: text !== ""
                    width: spalte.width - 120
                    wrapMode: Text.WordWrap
                    font.pixelSize: 28
                    color: "#ffffff"
                }
            }

            // Die zusammengesetzte Antwort.
            Rectangle {
                width: parent.width
                height: Math.max(74, gelegt.height + 16)
                color: "transparent"
                border.color: "#3a3a3a"
                border.width: 1
                radius: 6

                Flow {
                    id: gelegt
                    anchors { left: parent.left; leftMargin: 8
                              right: parent.right; rightMargin: 8
                              verticalCenter: parent.verticalCenter }
                    spacing: 8

                    Repeater {
                        model: flaeche.gewaehlt.length

                        Rectangle {
                            height: 54
                            width: beschriftung.width + 26
                            radius: 6
                            color: "#2f2f2f"
                            border.color: "#5a5a5a"
                            border.width: 1

                            Text {
                                id: beschriftung
                                anchors.centerIn: parent
                                text: flaeche.wortText(flaeche.gewaehlt[index])
                                font.pixelSize: 24
                                color: "#ffffff"
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: flaeche.zuruecklegen(index)
                            }
                        }
                    }
                }
            }

            // Der Vorrat.
            Flow {
                width: parent.width
                spacing: 8

                Repeater {
                    model: flaeche.aufgabe.wahlen

                    Rectangle {
                        height: 54
                        width: wort.width + 26
                        radius: 6
                        visible: !flaeche.schonGewaehlt(index)
                        color: "#262626"
                        border.color: "#3a3a3a"
                        border.width: 2

                        Text {
                            id: wort
                            anchors.centerIn: parent
                            text: modelData.text
                            font.pixelSize: 24
                            color: "#ffffff"
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: flaeche.nehmen(index)
                        }
                    }
                }
            }
        }
    }
}
