// Parses every QML file of the MeeGo edition and reports what QDeclarative
// complains about -- without a display and without the device.
//
// Why this exists: a mistyped property or a component that does not exist
// under QtQuick 1.1 does not fail at build time, it fails when the page is
// opened, on the device, in the rider's hand. QDeclarativeComponent finds
// it here instead, in a second.
//
// It cannot find everything: com.nokia.meego is not installed on the build
// machine, so the error list is filtered down to what does not come from
// the missing import. What is left are our own mistakes.
#include <QCoreApplication>
#include <QDeclarativeComponent>
#include <QDeclarativeContext>
#include <QDeclarativeEngine>
#include <QDir>
#include <QStringList>
#include <QTextStream>

#include "stubs.h"

int main(int argc, char *argv[])
{
    QCoreApplication application(argc, argv);
    QTextStream out(stdout);

    const QStringList arguments = application.arguments();
    if (arguments.size() < 2) {
        out << "usage: qml_check <qml directory>\n";
        return 2;
    }

    QDeclarativeEngine engine;
    StubNetz netz;
    StubKonto konto;
    StubLektion lektion;
    engine.rootContext()->setContextProperty("Netz", &netz);
    engine.rootContext()->setContextProperty("Konto", &konto);
    engine.rootContext()->setContextProperty("Lektion", &lektion);
    engine.rootContext()->setContextProperty("Fassung", QString("0.1.0"));

    const QDir directory(arguments.at(1));
    const QStringList names = directory.entryList(QStringList() << "*.qml",
                                                  QDir::Files, QDir::Name);
    int complaints = 0;
    for (int i = 0; i < names.size(); ++i) {
        const QString path = directory.filePath(names.at(i));
        QDeclarativeComponent component(&engine, QUrl::fromLocalFile(path));
        const QList<QDeclarativeError> errors = component.errors();
        for (int e = 0; e < errors.size(); ++e) {
            const QString text = errors.at(e).toString();
            // Everything that follows from com.nokia.meego being absent
            // here; the device has it. Dazu QtWebKit und QtMobility: deren
            // Plugins liegen zwar im SDK, lassen sich auf dem Baurechner
            // aber nicht laden (QtWebKit will libgst-0.10, das es dort
            // nicht mehr gibt). Auf dem N9/N950 sind beide da.
            if (text.contains("com.nokia.meego") || text.contains("com.nokia.extras")
                || text.contains("QtWebKit") || text.contains("QtMobility") || text.contains("QtMultimediaKit")
                || text.contains("is not a type") || text.contains("is not installed"))
                continue;
            out << text << "\n";
            ++complaints;
        }
    }
    out << (complaints ? QString("%1 complaint(s)\n").arg(complaints)
                       : QString("qml ok (%1 files)\n").arg(names.size()));
    return complaints ? 1 : 0;
}
