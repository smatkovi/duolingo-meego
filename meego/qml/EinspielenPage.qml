import QtQuick 1.1
import com.nokia.meego 1.0

// Anmelden ohne Passwort -- der Weg fuer Konten, die mit Google angelegt
// wurden.
//
// Zwei Moeglichkeiten, beide ohne Google-Anmeldung auf diesem Geraet (die
// wuerde hier ohnehin nicht laufen: Google laesst eingebettete Fenster
// nicht mehr zu):
//
//   1. Einmal ein Passwort setzen lassen. Die Mail dafuer kann die App
//      selbst anfordern; die Google-Anmeldung bleibt daneben bestehen.
//   2. Die Marke aus dem Browser hereinholen: dort anmelden wie gewohnt,
//      den Keks `jwt_token` kopieren und hier einsetzen.
Page {
    id: seite
    tools: werkzeuge

    property string meldung: ""

    Connections {
        target: Konto
        onPostUnterwegs: seite.meldung =
            qsTr("Die Mail ist unterwegs. Darin ein Passwort setzen, dann "
                 + "hier normal anmelden.")
        onFehlgeschlagen: seite.meldung = grund
        onAngemeldetWorden: {
            while (pageStack.depth > 1)
                pageStack.pop(undefined, true)
        }
    }

    Header { id: kopf; titel: qsTr("Ohne Passwort") }

    Flickable {
        anchors { top: kopf.bottom; left: parent.left; right: parent.right
                  bottom: parent.bottom }
        contentHeight: spalte.height + 32
        clip: true

        Column {
            id: spalte
            anchors { left: parent.left; leftMargin: 16; right: parent.right
                      rightMargin: 16; top: parent.top; topMargin: 16 }
            spacing: 14

            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                font.pixelSize: 22
                color: "#ffffff"
                text: qsTr("1. Ein Passwort setzen lassen")
            }
            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                font.pixelSize: 19
                color: "#9e9e9e"
                text: qsTr("Duolingo schickt eine Mail, in der sich ein "
                           + "Passwort setzen laesst. Die Anmeldung ueber "
                           + "Google bleibt daneben bestehen.")
            }
            TextField {
                id: adressfeld
                width: parent.width
                placeholderText: qsTr("E-Mail-Adresse des Kontos")
                inputMethodHints: Qt.ImhEmailCharactersOnly | Qt.ImhNoAutoUppercase
            }
            Button {
                text: qsTr("Mail anfordern")
                enabled: !Konto.laeuft && adressfeld.text.indexOf("@") > 0
                onClicked: {
                    seite.meldung = ""
                    Konto.passwortAnfordern(adressfeld.text)
                }
            }

            Rectangle {
                width: parent.width; height: 1; color: "#3a3a3a"
            }

            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                font.pixelSize: 22
                color: "#ffffff"
                text: qsTr("2. Marke aus dem Browser")
            }
            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                font.pixelSize: 19
                color: "#9e9e9e"
                text: qsTr("Am Rechner bei duolingo.com anmelden, in den "
                           + "Entwicklerwerkzeugen den Keks `jwt_token` "
                           + "kopieren und hier einsetzen. Er gilt ein Jahr.")
            }
            TextField {
                id: markenfeld
                width: parent.width
                placeholderText: qsTr("jwt_token")
                inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
            }
            Button {
                text: qsTr("Marke uebernehmen")
                enabled: markenfeld.text.length > 20
                onClicked: {
                    if (!Konto.markeEinspielen(markenfeld.text))
                        seite.meldung = qsTr("Das sieht nicht nach einer "
                                             + "Marke aus.")
                }
            }

            BusyIndicator {
                running: Konto.laeuft
                visible: Konto.laeuft
                platformStyle: BusyIndicatorStyle { size: "medium" }
            }

            Text {
                width: parent.width
                visible: seite.meldung !== ""
                text: seite.meldung
                wrapMode: Text.WordWrap
                font.pixelSize: 20
                color: "#ffc800"
            }
        }
    }

    ToolBarLayout {
        id: werkzeuge
        ToolIcon { iconId: "toolbar-back"; onClicked: pageStack.pop() }
    }
}
