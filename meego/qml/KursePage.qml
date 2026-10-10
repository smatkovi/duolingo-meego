import QtQuick 1.1
import com.nokia.meego 1.0

// Den laufenden Kurs wechseln -- oder einen neuen anfangen.
//
// Oben, was belegt ist (mit XP und einem Haken am laufenden), darunter die
// Sprachen, die es zur eigenen Muttersprache sonst noch gibt. Beides ist
// derselbe Aufruf: `currentCourseId` setzen. Ist der Kurs neu, faengt er
// damit an.
Page {
    id: seite
    tools: werkzeuge

    Component.onCompleted: Konto.angeboteHolen()

    Connections {
        target: Konto
        onKursGewechselt: pageStack.pop()
    }

    Header {
        id: kopf
        titel: qsTr("Kurse")
        unterzeile: Konto.muttersprache !== ""
                    ? qsTr("von %1 aus").arg(Konto.spracheName(Konto.muttersprache))
                    : ""
    }

    Flickable {
        anchors { top: kopf.bottom; left: parent.left; right: parent.right
                  bottom: parent.bottom }
        contentHeight: spalte.height + 32
        clip: true

        Column {
            id: spalte
            anchors { left: parent.left; right: parent.right; top: parent.top }

            Text {
                x: 16
                height: 56
                verticalAlignment: Text.AlignVCenter
                text: qsTr("Belegt")
                font.pixelSize: 20
                color: "#9e9e9e"
            }

            Repeater {
                model: Konto.kurse

                Item {
                    width: spalte.width
                    height: 76

                    Column {
                        anchors { left: parent.left; leftMargin: 16
                                  right: haken.left; rightMargin: 8
                                  verticalCenter: parent.verticalCenter }
                        spacing: 2
                        Text {
                            width: parent.width
                            text: modelData.titel
                            elide: Text.ElideRight
                            font.pixelSize: 26
                            color: "#ffffff"
                        }
                        Text {
                            text: qsTr("%1 XP").arg(modelData.xp)
                            font.pixelSize: 16
                            color: "#9e9e9e"
                        }
                    }

                    Text {
                        id: haken
                        anchors { right: parent.right; rightMargin: 16
                                  verticalCenter: parent.verticalCenter }
                        text: modelData.laufend ? "✓" : ""
                        font.pixelSize: 28
                        color: "#8ee000"
                    }

                    MouseArea {
                        anchors.fill: parent
                        enabled: !modelData.laufend && !Konto.laeuft
                        onClicked: Konto.kursWechseln(modelData.id)
                    }

                    Rectangle {
                        anchors { bottom: parent.bottom; left: parent.left
                                  right: parent.right }
                        height: 1
                        color: "#2c2c2c"
                    }
                }
            }

            Text {
                x: 16
                height: 64
                verticalAlignment: Text.AlignVCenter
                text: qsTr("Neu anfangen")
                font.pixelSize: 20
                color: "#9e9e9e"
                visible: Konto.angebote.length > 0
            }

            Repeater {
                model: Konto.angebote

                Item {
                    width: spalte.width
                    height: 64
                    visible: !seite.schonBelegt(modelData.id)

                    Text {
                        anchors { left: parent.left; leftMargin: 16
                                  right: parent.right; rightMargin: 16
                                  verticalCenter: parent.verticalCenter }
                        text: modelData.titel
                        elide: Text.ElideRight
                        font.pixelSize: 24
                        color: "#c8c8c8"
                    }

                    MouseArea {
                        anchors.fill: parent
                        enabled: !Konto.laeuft
                        onClicked: Konto.kursWechseln(modelData.id)
                    }

                    Rectangle {
                        anchors { bottom: parent.bottom; left: parent.left
                                  right: parent.right }
                        height: 1
                        color: "#2c2c2c"
                    }
                }
            }
        }
    }

    function schonBelegt(kursId) {
        for (var i = 0; i < Konto.kurse.length; ++i)
            if (Konto.kurse[i].id === kursId)
                return true
        return false
    }

    BusyIndicator {
        anchors.centerIn: parent
        running: Konto.laeuft
        visible: Konto.laeuft
        platformStyle: BusyIndicatorStyle { size: "large" }
    }

    ToolBarLayout {
        id: werkzeuge
        ToolIcon { iconId: "toolbar-back"; onClicked: pageStack.pop() }
    }
}
