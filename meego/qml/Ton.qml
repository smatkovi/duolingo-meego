import QtQuick 1.1
import com.nokia.meego 1.0
import QtMultimediaKit 1.1

// Ein Lautsprecherknopf. Harmattans Multimedia heisst QtMultimediaKit 1.1
// (QtMobility), nicht QtMultimedia.
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

    Audio {
        id: spieler
        source: knopf.quelle
    }

    function abspielen() {
        if (knopf.quelle == "")
            return
        spieler.stop()
        spieler.play()
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
