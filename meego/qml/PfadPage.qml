import QtQuick 1.1
import com.nokia.meego 1.0

// Der Pfad als Liste. Eine Reihe von Kreisen wie in der App waere hier
// huebscher als brauchbar: auf 480 Pixel sieht man in einer Liste mehr,
// und sie laesst sich mit dem Daumen schieben.
Page {
    id: seite
    tools: werkzeuge

    Header {
        id: kopf
        titel: qsTr("Pfad")
        unterzeile: Konto.kursName
    }

    ListView {
        id: liste
        anchors { top: kopf.bottom; left: parent.left; right: parent.right
                  bottom: parent.bottom }
        clip: true
        model: Konto.pfad
        cacheBuffer: 800

        delegate: Item {
            width: liste.width
            height: 88

            Rectangle {
                anchors.fill: parent
                color: "#1c1c1c"
                visible: zeiger.pressed && modelData.machbar && modelData.offen
            }

            Rectangle {
                anchors { left: parent.left; leftMargin: 16
                          verticalCenter: parent.verticalCenter }
                width: 10
                height: 10
                radius: 5
                color: modelData.zustand === "passed" ? "#8ee000"
                     : modelData.offen ? "#ffc800" : "#4a4a4a"
            }

            Column {
                anchors { left: parent.left; leftMargin: 40
                          right: parent.right; rightMargin: 16
                          verticalCenter: parent.verticalCenter }
                spacing: 3

                Text {
                    width: parent.width
                    text: modelData.titel !== ""
                          ? modelData.titel
                          : qsTr("Einheit %1").arg(modelData.einheit)
                    elide: Text.ElideRight
                    font.pixelSize: 24
                    color: modelData.machbar ? "#ffffff" : "#6a6a6a"
                }
                Text {
                    width: parent.width
                    elide: Text.ElideRight
                    font.pixelSize: 16
                    color: "#9e9e9e"
                    text: modelData.pruefung
                          ? qsTr("Einheit %1 · Test zum Ueberspringen")
                            .arg(modelData.einheit)
                          : qsTr("Einheit %1 · %2 von %3")
                            .arg(modelData.einheit)
                            .arg(modelData.fertig)
                            .arg(modelData.gesamt)
                            + (modelData.machbar ? "" : " · " + modelData.art)
                }
            }

            MouseArea {
                id: zeiger
                anchors.fill: parent
                enabled: modelData.machbar && modelData.offen
                onClicked: {
                    if (modelData.fach === "schach") {
                        pageStack.push(Qt.resolvedUrl("SchachPage.qml"))
                        if (modelData.art === "chess_match")
                            Schach.partieStarten(modelData.abschnitt)
                        else
                            Schach.lektionStarten(modelData.nummer)
                    } else {
                        pageStack.push(Qt.resolvedUrl("LektionPage.qml"))
                        Lektion.stufeStarten(modelData.nummer)
                    }
                }
            }

            Rectangle {
                anchors { bottom: parent.bottom; left: parent.left
                          right: parent.right }
                height: 1
                color: "#2c2c2c"
            }
        }
    }

    ScrollDecorator { flickableItem: liste }

    ToolBarLayout {
        id: werkzeuge
        ToolIcon { iconId: "toolbar-back"; onClicked: pageStack.pop() }
        ToolIcon {
            iconId: "toolbar-refresh"
            enabled: !Konto.laeuft
            onClicked: Konto.auffrischen()
        }
    }
}
