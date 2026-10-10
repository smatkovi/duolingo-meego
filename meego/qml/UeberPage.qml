import QtQuick 1.1
import com.nokia.meego 1.0

Page {
    id: seite
    tools: werkzeuge

    Header { id: kopf; titel: qsTr("Ueber") }

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
                text: qsTr("Duolingo fuer MeeGo %1").arg(Fassung)
                font.pixelSize: 28
                color: "#ffffff"
            }

            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                font.pixelSize: 19
                color: "#c8c8c8"
                text: qsTr("Kein Teil von Duolingo und keine Portierung der "
                           + "Android-App: ein eigener Client fuer das N9 und "
                           + "das N950, der dieselbe Schnittstelle spricht wie "
                           + "die Weboberflaeche.")
            }

            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                font.pixelSize: 19
                color: "#c8c8c8"
                text: qsTr("Benotet wird auf dem Geraet, so wie im Browser "
                           + "auch -- nach dem Graphen, den jede Aufgabe "
                           + "mitbringt. Was dabei herauskommt, geht an den "
                           + "Server und zaehlt fuer den echten Fortschritt.")
            }

            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                font.pixelSize: 19
                color: "#c8c8c8"
                text: qsTr("Aufgaben mit Mikrofon, Zeichenbrett, Geschichten, "
                           + "Schach und Rechnen fehlen. Der Server schickt "
                           + "sie gar nicht erst: welche Arten kommen, sagt "
                           + "der Client beim Anfordern.")
            }

            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                font.pixelSize: 17
                color: "#7a7a7a"
                text: qsTr("HTTPS macht ein kleines Rust-Programm daneben, "
                           + "weil Qt 4.7 auf Harmattan kein TLS 1.2 kann. "
                           + "Der Ton laeuft ueber http, weil gstreamer hier "
                           + "gar kein TLS kann.")
            }
        }
    }

    ToolBarLayout {
        id: werkzeuge
        ToolIcon { iconId: "toolbar-back"; onClicked: pageStack.pop() }
    }
}
