import QtQuick 1.1
import com.nokia.meego 1.0

// Die Lektion. Oben der Fortschritt, in der Mitte die Aufgabe, unten der
// Knopf und das Band mit der Rueckmeldung.
//
// Welche Aufgabe gezeichnet wird, entscheidet `Lektion.aufgabe.darstellung`
// -- die vier Faelle liegen in eigenen Dateien. Ein Loader statt eines
// grossen Wenn-Dann: so bleibt jede Darstellung fuer sich lesbar, und beim
// Weitergehen wird sie sauber neu gebaut.
Page {
    id: seite
    tools: werkzeuge

    property bool rueckmeldung: false
    property bool warRichtig: false
    property string loesung: ""

    Connections {
        target: Lektion
        onBenotet: {
            seite.warRichtig = richtig
            seite.loesung = loesung
            seite.rueckmeldung = true
        }
        onAbgegeben: {
            pageStack.pop()
            abschluss.xp = xp
            abschluss.open()
        }
        onFehlgeschlagen: {
            band.visible = false
            fehlerdialog.message = grund
            fehlerdialog.open()
        }
        onGeaendert: seite.rueckmeldung = false
    }

    Component.onDestruction: Lektion.abbrechen()

    // Kopf: Fortschrittsbalken und Herzen.
    Item {
        id: kopf
        anchors { top: parent.top; left: parent.left; right: parent.right }
        height: 56

        Rectangle {
            id: balkenGrund
            anchors { left: parent.left; leftMargin: 16
                      right: herzen.visible ? herzen.left : parent.right
                      rightMargin: 16
                      verticalCenter: parent.verticalCenter }
            height: 10
            radius: 5
            color: "#3a3a3a"

            Rectangle {
                height: parent.height
                radius: parent.radius
                color: "#8ee000"
                width: Lektion.anzahl > 0
                       ? parent.width * Lektion.nummer / Lektion.anzahl : 0
            }
        }

        Text {
            id: herzen
            anchors { right: parent.right; rightMargin: 16
                      verticalCenter: parent.verticalCenter }
            visible: Konto.herzenZaehlen
            text: "♥ " + Lektion.herzen
            font.pixelSize: 22
            color: "#ff5252"
        }
    }

    BusyIndicator {
        anchors.centerIn: parent
        running: Lektion.laeuft
        visible: Lektion.laeuft
        platformStyle: BusyIndicatorStyle { size: "large" }
    }

    // Die Aufgabe selbst.
    Loader {
        id: aufgabenflaeche
        anchors { top: kopf.bottom; left: parent.left; right: parent.right
                  bottom: fuss.top; margins: 0 }
        visible: !Lektion.laeuft && !Lektion.fertig
        source: {
            if (Lektion.laeuft || !Lektion.offen)
                return ""
            var d = Lektion.aufgabe.darstellung
            if (d === "woerter") return "WoerterAufgabe.qml"
            if (d === "paare") return "PaareAufgabe.qml"
            if (d === "tippen") return "TippenAufgabe.qml"
            if (d === "wahl") return "WahlAufgabe.qml"
            return ""
        }
        onLoaded: item.aufgabe = Lektion.aufgabe
    }

    // Am Ende: die Abrechnung und der Knopf zum Abschicken.
    Column {
        anchors { centerIn: parent }
        width: parent.width - 64
        spacing: 20
        visible: Lektion.fertig && !Lektion.laeuft

        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("Geschafft")
            font.pixelSize: 36
            color: "#8ee000"
        }
        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            font.pixelSize: 22
            color: "#ffffff"
            text: qsTr("%1 von %2 richtig")
                  .arg(Lektion.richtige).arg(Lektion.anzahl)
        }
        Button {
            width: parent.width
            text: qsTr("Abschicken")
            enabled: !Lektion.laeuft
            onClicked: Lektion.abgeben()
        }
        Button {
            width: parent.width
            text: qsTr("Verwerfen")
            onClicked: pageStack.pop()
        }
    }

    // Fuss: Rueckmeldung und der grosse Knopf.
    Item {
        id: fuss
        anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
        height: band.visible ? 150 : 86
        visible: Lektion.offen && !Lektion.fertig && !Lektion.laeuft

        Rectangle {
            id: band
            anchors { top: parent.top; left: parent.left; right: parent.right }
            height: 64
            visible: seite.rueckmeldung
            color: seite.warRichtig ? "#1d3b00" : "#3b0000"

            Text {
                anchors { left: parent.left; leftMargin: 16
                          right: parent.right; rightMargin: 16
                          verticalCenter: parent.verticalCenter }
                elide: Text.ElideRight
                font.pixelSize: 20
                color: seite.warRichtig ? "#8ee000" : "#ff8a8a"
                text: seite.warRichtig
                      ? qsTr("Richtig")
                      : qsTr("Richtig waere: %1").arg(seite.loesung)
            }
        }

        Button {
            anchors { bottom: parent.bottom; bottomMargin: 16
                      left: parent.left; leftMargin: 16
                      right: parent.right; rightMargin: 16 }
            text: seite.rueckmeldung ? qsTr("Weiter") : qsTr("Pruefen")
            enabled: seite.rueckmeldung
                     || (aufgabenflaeche.item && aufgabenflaeche.item.bereit)
            onClicked: {
                if (seite.rueckmeldung) {
                    Lektion.weiter()
                } else if (aufgabenflaeche.item) {
                    Lektion.antworten(aufgabenflaeche.item.reihe(),
                                      aufgabenflaeche.item.text())
                }
            }
        }
    }

    QueryDialog {
        id: fehlerdialog
        titleText: qsTr("Schiefgegangen")
        acceptButtonText: qsTr("Gut")
    }

    QueryDialog {
        id: abschluss
        property int xp: 0
        titleText: qsTr("Fertig")
        message: qsTr("%1 XP dazu.").arg(abschluss.xp)
        acceptButtonText: qsTr("Gut")
    }

    ToolBarLayout {
        id: werkzeuge
        ToolIcon {
            iconId: "toolbar-back"
            onClicked: pageStack.pop()
        }
        ToolIcon {
            iconId: "toolbar-next"
            visible: Lektion.offen && !Lektion.fertig && !seite.rueckmeldung
            onClicked: Lektion.auslassen()
        }
    }
}
