import QtQuick 1.1
import com.nokia.meego 1.0

// Die Oberflaeche fuer MeeGo Harmattan. QtQuick 1.1 heisst: kein Canvas,
// keine Eigenschaften aus spaeteren Fassungen, PageStackWindow statt
// ApplicationWindow.
PageStackWindow {
    id: fenster

    initialPage: startseite
    showStatusBar: true
    showToolBar: true

    // Dunkel wie die meisten Harmattan-Apps. `theme.inverted` ist global und
    // setzbar -- das faerbt auch die Plattformhuelle mit.
    Component.onCompleted: {
        theme.inverted = true
        if (Konto.angemeldet)
            Konto.auffrischen()
        else
            pageStack.push(Qt.resolvedUrl("AnmeldenPage.qml"))
    }

    StartPage { id: startseite }

    Connections {
        target: Netz
        onMarkeAbgelaufen: {
            while (pageStack.depth > 1)
                pageStack.pop(undefined, true)
            pageStack.push(Qt.resolvedUrl("AnmeldenPage.qml"))
        }
    }

    Connections {
        target: Konto
        onFehlgeschlagen: fenster.melden(grund)
    }

    QueryDialog {
        id: meldungsdialog
        titleText: qsTr("Schiefgegangen")
        acceptButtonText: qsTr("Gut")
    }

    function melden(text) {
        meldungsdialog.message = text
        meldungsdialog.open()
    }
}
