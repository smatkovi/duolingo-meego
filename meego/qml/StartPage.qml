import QtQuick 1.1
import com.nokia.meego 1.0

// Was beim Aufmachen zu sehen ist: Straehne, XP, der laufende Kurs -- und
// zwei Knoepfe. "Weiter" nimmt die naechste offene Stufe des Pfades,
// "Ueben" eine Wiederholung ueber den ganzen Kurs.
Page {
    id: seite
    tools: werkzeuge

    Header {
        id: kopf
        titel: Konto.kursName !== "" ? Konto.kursName : qsTr("Duolingo")
        unterzeile: Konto.angemeldet && Konto.name !== ""
                    ? Konto.name : ""
    }

    Flickable {
        anchors { top: kopf.bottom; left: parent.left; right: parent.right
                  bottom: parent.bottom }
        contentHeight: spalte.height + 32
        clip: true

        Column {
            id: spalte
            anchors { left: parent.left; leftMargin: 16; right: parent.right
                      rightMargin: 16; top: parent.top; topMargin: 16 }
            spacing: 18

            Row {
                spacing: 32
                visible: Konto.angemeldet

                Column {
                    Text {
                        text: Konto.straehne
                        font.pixelSize: 40
                        color: "#ffc800"
                    }
                    Text {
                        text: qsTr("Tage in Folge")
                        font.pixelSize: 16
                        color: "#9e9e9e"
                    }
                }
                Column {
                    Text {
                        text: Konto.xp
                        font.pixelSize: 40
                        color: "#8ee000"
                    }
                    Text {
                        text: qsTr("XP")
                        font.pixelSize: 16
                        color: "#9e9e9e"
                    }
                }
                Column {
                    visible: Konto.herzenZaehlen
                    Text {
                        text: Konto.herzen
                        font.pixelSize: 40
                        color: "#ff5252"
                    }
                    Text {
                        text: qsTr("Herzen")
                        font.pixelSize: 16
                        color: "#9e9e9e"
                    }
                }
            }

            BusyIndicator {
                running: Konto.laeuft || Lektion.laeuft
                visible: Konto.laeuft || Lektion.laeuft
                platformStyle: BusyIndicatorStyle { size: "medium" }
            }

            Button {
                width: parent.width
                visible: Konto.angemeldet
                enabled: !Lektion.laeuft && Konto.naechsteStufe() >= 0
                text: qsTr("Weiter im Kurs")
                onClicked: {
                    var nummer = Konto.naechsteStufe()
                    if (nummer < 0)
                        return
                    var zeile = Konto.pfad[nummer]
                    if (zeile.fach === "schach") {
                        pageStack.push(Qt.resolvedUrl("SchachPage.qml"))
                        if (zeile.art === "chess_match")
                            Schach.partieStarten(zeile.abschnitt)
                        else
                            Schach.lektionStarten(nummer)
                    } else {
                        pageStack.push(Qt.resolvedUrl("LektionPage.qml"))
                        Lektion.stufeStarten(nummer)
                    }
                }
            }

            Button {
                width: parent.width
                visible: Konto.angemeldet
                enabled: !Lektion.laeuft
                text: qsTr("Ueben")
                onClicked: {
                    pageStack.push(Qt.resolvedUrl("LektionPage.qml"))
                    Lektion.uebenStarten()
                }
            }

            Button {
                width: parent.width
                visible: Konto.angemeldet
                text: qsTr("Der ganze Pfad")
                onClicked: pageStack.push(Qt.resolvedUrl("PfadPage.qml"))
            }

            Button {
                width: parent.width
                visible: Konto.angemeldet
                text: qsTr("Schach")
                onClicked: pageStack.push(Qt.resolvedUrl("SchachPage.qml"))
            }

            Button {
                width: parent.width
                visible: Konto.angemeldet
                text: Konto.kurse.length > 1
                      ? qsTr("Kurs wechseln") : qsTr("Kurse")
                onClicked: pageStack.push(Qt.resolvedUrl("KursePage.qml"))
            }

            Button {
                width: parent.width
                visible: !Konto.angemeldet
                text: qsTr("Anmelden")
                onClicked: pageStack.push(Qt.resolvedUrl("AnmeldenPage.qml"))
            }
        }
    }

    ToolBarLayout {
        id: werkzeuge
        ToolIcon {
            iconId: "toolbar-refresh"
            enabled: Konto.angemeldet && !Konto.laeuft
            onClicked: Konto.auffrischen()
        }
        ToolIcon {
            iconId: "toolbar-view-menu"
            onClicked: menue.open()
        }
    }

    Menu {
        id: menue
        MenuLayout {
            MenuItem {
                text: qsTr("Kurs wechseln")
                visible: Konto.angemeldet
                onClicked: pageStack.push(Qt.resolvedUrl("KursePage.qml"))
            }
            MenuItem {
                text: qsTr("Ueber")
                onClicked: pageStack.push(Qt.resolvedUrl("UeberPage.qml"))
            }
            MenuItem {
                text: qsTr("Abmelden")
                visible: Konto.angemeldet
                onClicked: Konto.abmelden()
            }
        }
    }
}
