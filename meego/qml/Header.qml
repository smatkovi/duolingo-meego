import QtQuick 1.1
import com.nokia.meego 1.0

// Der Seitenkopf. Harmattan bringt keinen mit, deshalb hier einer, der auf
// allen Seiten gleich aussieht.
Item {
    id: kopf

    property string titel
    property string unterzeile

    anchors { top: parent.top; left: parent.left; right: parent.right }
    height: unterzeile !== "" ? 86 : 64

    Column {
        anchors { left: parent.left; leftMargin: 16
                  verticalCenter: parent.verticalCenter
                  right: parent.right; rightMargin: 16 }
        spacing: 2

        Text {
            width: parent.width
            text: kopf.titel
            elide: Text.ElideRight
            font.pixelSize: 32
            font.weight: Font.Light
            color: theme.inverted ? "#ffffff" : "#000000"
        }
        Text {
            width: parent.width
            visible: kopf.unterzeile !== ""
            text: kopf.unterzeile
            elide: Text.ElideRight
            font.pixelSize: 18
            color: "#9e9e9e"
        }
    }

    Rectangle {
        anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
        height: 1
        color: theme.inverted ? "#3a3a3a" : "#d0d0d0"
    }
}
