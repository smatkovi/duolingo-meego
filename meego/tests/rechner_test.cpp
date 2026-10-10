// Probe fuer die Rechenaufgaben, gegen echte Rohdaten.
//
// Dieselbe Frage wie beim Sprachbenoter: **nimmt jede Aufgabe ihre eigene
// Loesung an, und faellt eine falsche durch?** Die richtige Antwort steht
// in der Aufgabe (`correctIndices`, `correctAnswer`), die falsche wird
// daraus gemacht.
//
//   build/meego/rechnen/duolingo doc/sitzung-rechnen.json
#include <QCoreApplication>
#include <QFile>
#include <QStringList>
#include <QTextStream>
#include <QVariantList>

#include "json.h"
#include "rechner.h"

int main(int argc, char *argv[])
{
    QCoreApplication anwendung(argc, argv);
    QTextStream aus(stdout);
    const QStringList argumente = anwendung.arguments();
    if (argumente.size() < 2) {
        aus << "Aufruf: rechner_test <sitzung.json>\n";
        return 2;
    }

    int gut = 0, schlecht = 0, falschErkannt = 0, falschDurch = 0;
    for (int d = 1; d < argumente.size(); ++d) {
        QFile datei(argumente.at(d));
        if (!datei.open(QIODevice::ReadOnly))
            continue;
        const QVariant gelesen = Json::parse(datei.readAll());
        const QVariantList aufgaben =
            gelesen.toMap().value("challenges").toList();
        aus << argumente.at(d) << ": " << aufgaben.size() << " Aufgaben\n";

        for (int i = 0; i < aufgaben.size(); ++i) {
            const QVariantMap a = aufgaben.at(i).toMap();
            if (!Rechner::istRechnen(a))
                continue;
            const QVariantMap sicht = Rechner::darstellen(a);
            const QString art = sicht.value("eingabe").toString();
            const QString loesung = Rechner::loesung(a);

            QVariantList reihe;
            QString text;
            if (art == "wahl") {
                const QVariantList wahlen = sicht.value("wahlen").toList();
                for (int w = 0; w < wahlen.size(); ++w)
                    if (wahlen.at(w).toMap().value("text").toString() == loesung)
                        reihe << w;
            } else if (art == "bauen") {
                // Die Bausteine so legen, dass die Loesung herauskommt:
                // alle Reihenfolgen zu probieren waere zu viel, also wird
                // nur geprueft, dass die Zahl allein schon richtig ist,
                // wenn sie unter den Bausteinen steht.
                const QVariantList wahlen = sicht.value("wahlen").toList();
                for (int w = 0; w < wahlen.size(); ++w)
                    if (wahlen.at(w).toMap().value("text").toString() == loesung) {
                        reihe.clear();
                        reihe << w;
                        break;
                    }
                if (reihe.isEmpty())
                    continue;   // muss zusammengesetzt werden, nicht pruefbar
            } else {
                text = loesung;
            }

            if (Rechner::benoten(a, reihe, text)) {
                ++gut;
            } else {
                ++schlecht;
                aus << "   NICHT ANERKANNT " << a.value("type").toString()
                    << " (" << art << "): " << loesung << "\n";
            }

            // Eine falsche Antwort darf nicht durchgehen.
            QVariantList falsch;
            if (!reihe.isEmpty())
                falsch << (reihe.first().toInt() + 1);
            const QString falschText = text.isEmpty() ? QString()
                                                      : text + "7";
            if (Rechner::benoten(a, falsch, falschText))
                ++falschDurch;
            else
                ++falschErkannt;
        }
    }

    aus << "Loesung anerkannt: " << gut << ", nicht: " << schlecht << "\n";
    aus << "Falsche Antworten: " << falschErkannt << " erkannt, "
        << falschDurch << " durchgelassen\n";
    return (schlecht || falschDurch) ? 1 : 0;
}
