import QtQuick 1.1
import com.nokia.meego 1.0

// Ein Satz und ein Knopf. Ist nichts zu sagen, nimmt das Bauteil keine
// Hoehe ein -- die Liste darunter haengt an seiner Unterkante.
Item {
    id: kasten

    property string text
    property string schaltertext
    signal gedrueckt()

    anchors { left: parent.left; right: parent.right }
    height: kasten.text !== "" ? inhalt.height + 24 : 0
    visible: kasten.text !== ""

    Column {
        id: inhalt
        anchors { left: parent.left; leftMargin: 16; right: parent.right
                  rightMargin: 16; top: parent.top; topMargin: 12 }
        spacing: 10

        Text {
            width: parent.width
            text: kasten.text
            wrapMode: Text.WordWrap
            font.pixelSize: 20
            color: "#4ec9c4"
        }

        Button {
            visible: kasten.schaltertext !== ""
            text: kasten.schaltertext
            onClicked: kasten.gedrueckt()
        }
    }
}
