// Duolingo fuer MeeGo Harmattan (Nokia N9/N950).
//
// Der Kern liegt in src/ und ist von Qt 5 unabhaengig geschrieben, damit
// eine spaetere Sailfish-Fassung nichts kostet. Hier wird er mit der
// com.nokia.meego-Oberflaeche zusammengesteckt:
//   * QApplication und QDeclarativeView statt QGuiApplication/QQuickView
//   * das QML liegt in /opt/duolingo/qml, nicht in einer Ressource
//   * das Netz macht der Rust-Holer daneben (src/netz.cpp)
#include <QApplication>
#include <QDeclarativeContext>
#include <QDeclarativeView>
#include <QDir>
#include <QFileInfo>
#include <QLocale>
#include <QTranslator>

#include "konto.h"
#include "lektion.h"
#include "netz.h"

#ifndef APP_VERSION
#define APP_VERSION "0.0.0"
#endif

namespace {

QString qmlDatei()
{
    // Aus dem Baubaum heraus zu starten ist fuer den QML-Pruefer und einen
    // schnellen Versuch ueber ssh noetig; der installierte Pfad gewinnt,
    // wenn es ihn gibt.
    const QString installiert = QLatin1String("/opt/duolingo/qml/main.qml");
    if (QFileInfo(installiert).exists())
        return installiert;
    return QDir(QCoreApplication::applicationDirPath()).filePath(
        QLatin1String("qml/main.qml"));
}

}  // namespace

int main(int argc, char *argv[])
{
    QApplication anwendung(argc, argv);
    anwendung.setOrganizationName(QLatin1String("duolingo"));
    anwendung.setApplicationName(QLatin1String("duolingo"));

    QTranslator uebersetzer;
    if (uebersetzer.load(QLatin1String("duolingo-") + QLocale::system().name(),
                         QLatin1String("/opt/duolingo/translations")))
        anwendung.installTranslator(&uebersetzer);

    Netz netz;
    Konto konto(&netz);
    Lektion lektion(&netz, &konto);

    QDeclarativeView sicht;
    QDeclarativeContext *zusammenhang = sicht.rootContext();
    zusammenhang->setContextProperty(QLatin1String("Netz"), &netz);
    zusammenhang->setContextProperty(QLatin1String("Konto"), &konto);
    zusammenhang->setContextProperty(QLatin1String("Lektion"), &lektion);
    zusammenhang->setContextProperty(QLatin1String("Fassung"),
                                     QString(QLatin1String(APP_VERSION)));

    sicht.setResizeMode(QDeclarativeView::SizeRootObjectToView);
    sicht.setSource(QUrl::fromLocalFile(qmlDatei()));
    sicht.showFullScreen();

    return anwendung.exec();
}
