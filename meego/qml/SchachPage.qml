import QtQuick 1.1
import com.nokia.meego 1.0

// Schach gegen den Rechner, mit Duolingos Buchfuehrung dahinter: Elo und
// Bilanz kommen vom Dienst, jede Partie und jeder Zug wandern dorthin
// zurueck (doc/SCHACH.md).
Page {
    id: seite
    tools: werkzeuge

    Component.onCompleted: Schach.standHolen()
    Component.onDestruction: Schach.partieVerlassen()

    Connections {
        target: Schach
        onUmwandlungNoetig: umwandlung.open()
        onPartieZuEnde: {
            ende.message = qsTr("%1.").arg(ausgang)
            ende.open()
        }
    }

    Item {
        id: kopf
        anchors { top: parent.top; left: parent.left; right: parent.right }
        height: 64

        Text {
            anchors { left: parent.left; leftMargin: 16
                      verticalCenter: parent.verticalCenter }
            text: Schach.offen ? Schach.stand
                               : qsTr("Elo %1").arg(Schach.elo)
            font.pixelSize: 24
            color: "#ffffff"
        }
        Text {
            anchors { right: parent.right; rightMargin: 16
                      verticalCenter: parent.verticalCenter }
            text: qsTr("%1 S · %2 N · %3 R")
                  .arg(Schach.siege).arg(Schach.niederlagen).arg(Schach.remis)
            font.pixelSize: 18
            color: "#9e9e9e"
        }
    }

    Brett {
        id: brett
        anchors { top: kopf.bottom; left: parent.left; right: parent.right
                  bottom: fuss.top; margins: 8 }
        visible: Schach.offen
    }

    // Vor der ersten Partie: ein Knopf und ein Wort dazu.
    Column {
        anchors.centerIn: parent
        width: parent.width - 64
        spacing: 18
        visible: !Schach.offen && !Schach.laeuft

        Text {
            width: parent.width
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 22
            color: "#c8c8c8"
            text: qsTr("Eine Partie gegen den Rechner. Duolingo fuehrt Elo "
                       + "und Bilanz mit; gerechnet wird auf dem Telefon.")
        }
        Button {
            width: parent.width
            text: qsTr("Partie anfangen")
            onClicked: Schach.partieStarten(1)
        }
    }

    BusyIndicator {
        anchors.centerIn: parent
        running: Schach.laeuft
        visible: Schach.laeuft
        platformStyle: BusyIndicatorStyle { size: "large" }
    }

    Item {
        id: fuss
        anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
        height: Schach.offen ? 72 : 0
        visible: Schach.offen

        Text {
            anchors { left: parent.left; leftMargin: 16
                      right: parent.right; rightMargin: 16
                      verticalCenter: parent.verticalCenter }
            elide: Text.ElideLeft
            font.pixelSize: 18
            color: "#9e9e9e"
            text: Schach.zugliste.join(" ")
        }
    }

    SelectionDialog {
        id: umwandlung
        titleText: qsTr("Umwandeln in")
        model: ListModel {
            ListElement { name: QT_TR_NOOP("Dame"); code: "q" }
            ListElement { name: QT_TR_NOOP("Turm"); code: "r" }
            ListElement { name: QT_TR_NOOP("Laeufer"); code: "b" }
            ListElement { name: QT_TR_NOOP("Springer"); code: "n" }
        }
        onSelectedIndexChanged: {
            if (selectedIndex >= 0)
                Schach.umwandeln(model.get(selectedIndex).code)
        }
    }

    QueryDialog {
        id: ende
        titleText: qsTr("Partie zu Ende")
        acceptButtonText: qsTr("Gut")
        onAccepted: Schach.partieVerlassen()
    }

    ToolBarLayout {
        id: werkzeuge
        ToolIcon { iconId: "toolbar-back"; onClicked: pageStack.pop() }
        ToolIcon {
            iconId: "toolbar-refresh"
            visible: !Schach.offen
            onClicked: Schach.standHolen()
        }
        ToolIcon {
            iconId: "toolbar-delete"
            visible: Schach.offen
            onClicked: aufgeben.open()
        }
    }

    QueryDialog {
        id: aufgeben
        titleText: qsTr("Aufgeben?")
        message: qsTr("Die Partie zaehlt dann als verloren.")
        acceptButtonText: qsTr("Aufgeben")
        rejectButtonText: qsTr("Weiterspielen")
        onAccepted: Schach.aufgeben()
    }
}
