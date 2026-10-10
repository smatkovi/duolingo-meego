import QtQuick 1.1
import com.nokia.meego 1.0

// Ein Lautsprecherknopf.
//
// Gespielt wird nicht mit dem QML-Bauteil `Audio` aus QtMultimediaKit --
// das liess sich druecken und blieb stumm --, sondern mit gstreamer
// selbst, ueber `Lautsprecher` (src/lautsprecher.h). Der Weg ist am Geraet
// nachgemessen.
//
// Die Adresse kommt schon als `http` an -- umgeschrieben in src/lektion.cpp,
// weil gstreamer auf diesem Geraet kein TLS kann.
Item {
    id: knopf

    property url quelle
    property int groesse: 56
    property bool vonSelbst: false

    width: groesse
    height: groesse
    visible: quelle != ""

    function abspielen() {
        if (knopf.quelle == "")
            return
        Lautsprecher.spielen("" + knopf.quelle)
    }

    onQuelleChanged: if (vonSelbst) abspielen()

    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: zeiger.pressed ? "#2a5c93" : "#1e88e5"
    }

    Text {
        anchors.centerIn: parent
        text: "▶"
        font.pixelSize: parent.height / 2.4
        color: "#ffffff"
    }

    MouseArea {
        id: zeiger
        anchors.fill: parent
        onClicked: knopf.abspielen()
    }
}
