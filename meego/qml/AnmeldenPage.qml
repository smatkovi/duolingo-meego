import QtQuick 1.1
import com.nokia.meego 1.0

// Anmelden mit dem eigenen Duolingo-Konto. Blankes JSON, keine
// Pruefungsaufgabe; die Marke kommt im Antwortkopf zurueck.
Page {
    id: seite
    tools: werkzeuge

    property string meldung: ""

    Connections {
        target: Konto
        onAngemeldetWorden: {
            seite.meldung = ""
            pageStack.pop()
        }
        onAnmeldungAbgelehnt: seite.meldung = grund
    }

    Header { id: kopf; titel: qsTr("Anmelden") }

    Flickable {
        anchors { top: kopf.bottom; left: parent.left; right: parent.right
                  bottom: parent.bottom }
        contentHeight: spalte.height + 32
        clip: true

        Column {
            id: spalte
            anchors { left: parent.left; leftMargin: 16; right: parent.right
                      rightMargin: 16; top: parent.top; topMargin: 16 }
            spacing: 16

            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                font.pixelSize: 20
                color: "#9e9e9e"
                text: qsTr("Dasselbe Konto wie in der App und im Browser. "
                           + "Benutzername oder E-Mail-Adresse.")
            }

            TextField {
                id: kennungsfeld
                width: parent.width
                placeholderText: qsTr("Benutzername oder E-Mail")
                inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
            }

            TextField {
                id: passwortfeld
                width: parent.width
                placeholderText: qsTr("Passwort")
                echoMode: TextInput.Password
            }

            Button {
                text: qsTr("Anmelden")
                enabled: !Konto.laeuft && kennungsfeld.text !== ""
                         && passwortfeld.text !== ""
                onClicked: {
                    seite.meldung = ""
                    Konto.anmelden(kennungsfeld.text, passwortfeld.text)
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
                color: "#ff6b6b"
            }

            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                font.pixelSize: 16
                color: "#7a7a7a"
                text: qsTr("Das Passwort bleibt auf dem Geraet nicht liegen; "
                           + "gespeichert wird nur die Marke, die der Server "
                           + "dafuer ausgibt.")
            }
        }
    }

    ToolBarLayout {
        id: werkzeuge
        ToolIcon {
            iconId: "toolbar-back"
            visible: pageStack.depth > 1
            onClicked: pageStack.pop()
        }
    }
}
