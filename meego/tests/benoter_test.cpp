// Prueft den C++-Benoter gegen echte Rohdaten -- dieselbe Probe, die
// tools/benoter-pruefen.py in Python macht: **jede Aufgabe muss ihre eigene
// Musterloesung annehmen**, und eine halbe Antwort darf nicht durchgehen.
//
// Warum doppelt: die Python-Fassung prueft den Gedanken, diese hier den
// Code, der am Telefon laeuft. Weichen sie voneinander ab, faellt es hier
// auf und nicht im echten Kurs.
//
//   build/meego/check/benoter_test doc/sitzung-global_practice.json
#include <QCoreApplication>
#include <QFile>
#include <QStringList>
#include <QTextStream>
#include <QVariantList>

#include "benoter.h"
#include "json.h"

int main(int argc, char *argv[])
{
    QCoreApplication anwendung(argc, argv);
    QTextStream aus(stdout);
    const QStringList argumente = anwendung.arguments();
    if (argumente.size() < 2) {
        aus << "Aufruf: benoter_test <sitzung.json> …\n";
        return 2;
    }

    int gut = 0, schlecht = 0, ohne = 0, gegenprobe = 0, durchgelassen = 0;
    int ganzGut = 0, ganzSchlecht = 0, falschErkannt = 0, falschDurch = 0;
    for (int d = 1; d < argumente.size(); ++d) {
        QFile datei(argumente.at(d));
        if (!datei.open(QIODevice::ReadOnly)) {
            aus << "nicht zu lesen: " << argumente.at(d) << "\n";
            return 2;
        }
        QString grund;
        const QVariant gelesen = Json::parse(datei.readAll(), &grund);
        if (!gelesen.isValid()) {
            aus << "kein JSON: " << grund << "\n";
            return 2;
        }
        QVariantList aufgaben =
            gelesen.toMap().value("challenges").toList();
        aufgaben += gelesen.toMap().value("adaptiveChallenges").toList();
        aus << argumente.at(d) << ": " << aufgaben.size() << " Aufgaben\n";

        for (int i = 0; i < aufgaben.size(); ++i) {
            const QVariantMap aufgabe = aufgaben.at(i).toMap();
            const QString art = aufgabe.value("type").toString();
            // Schach und Rechnen werden nicht vom Sprachbenoter benotet:
            // sie haben eigene Regeln (Zugfolge, Formelbaum). Ihre
            // Rohdaten liegen im selben Ordner, gehoeren hier aber nicht
            // her.
            if (art == "star" || art.startsWith("math")
                || art.startsWith("chess"))
                continue;

            // Die ganze Benotung, so wie die Oberflaeche sie aufruft: eine
            // vollkommene Antwort muss als richtig durchgehen, eine
            // verdrehte nicht. Das deckt auch die Arten ohne Graphen ab
            // (Wahl, angetippte Woerter, Paare).
            {
                QList<int> reihe;
                QString text;
                if (aufgabe.contains("correctIndex")) {
                    reihe << aufgabe.value("correctIndex").toInt();
                } else if (!aufgabe.value("correctIndices").toList().isEmpty()) {
                    const QVariantList r = aufgabe.value("correctIndices").toList();
                    const QVariantList wahlen = aufgabe.value("choices").toList();
                    QStringList teile;
                    for (int k = 0; k < r.size(); ++k) {
                        const int n = r.at(k).toInt();
                        reihe << n;
                        if (n >= 0 && n < wahlen.size())
                            teile << (wahlen.at(n).type() == QVariant::Map
                                          ? wahlen.at(n).toMap().value("text").toString()
                                          : wahlen.at(n).toString());
                    }
                    text = teile.join(" ");
                } else if (aufgabe.contains("pairs")) {
                    text = "fertig";
                } else {
                    text = Benoter::musterloesung(aufgabe);
                }

                if (Benoter::benoten(aufgabe, text, reihe)) {
                    ++ganzGut;
                } else {
                    ++ganzSchlecht;
                    aus << "   NICHT ANERKANNT " << aufgabe.value("type").toString()
                        << ": " << text << "\n";
                }

                // Eine klar falsche Antwort darf nicht durchgehen. Bei den
                // Paaren gibt es nichts zu verdrehen -- die Darstellung
                // laesst nur richtige zu.
                if (!aufgabe.contains("pairs")) {
                    QList<int> verdreht;
                    for (int k = reihe.size() - 1; k >= 0; --k)
                        verdreht << reihe.at(k);
                    const QString quatsch = "zzz " + text;
                    if (reihe.size() > 1 || !text.isEmpty()) {
                        if (Benoter::benoten(aufgabe, quatsch,
                                             reihe.size() > 1 ? verdreht
                                                              : QList<int>()))
                            ++falschDurch;
                        else
                            ++falschErkannt;
                    }
                }
            }

            const QVariantMap graph = aufgabe.value("grader").toMap();
            if (graph.isEmpty()) {
                ++ohne;
                continue;
            }
            const QString loesung = Benoter::musterloesung(aufgabe);
            if (Benoter::graphNimmtAn(graph, loesung)) {
                ++gut;
            } else {
                ++schlecht;
                aus << "   ABGELEHNT " << aufgabe.value("type").toString()
                    << ": " << loesung << "\n";
            }
            const QStringList teile = loesung.split(' ');
            if (teile.size() > 1) {
                QStringList kurz = teile;
                kurz.removeLast();
                if (Benoter::graphNimmtAn(graph, kurz.join(" "))) {
                    ++durchgelassen;
                    aus << "   HALB ANGENOMMEN "
                        << aufgabe.value("type").toString() << "\n";
                } else {
                    ++gegenprobe;
                }
            }
        }
    }

    aus << "Musterloesung angenommen: " << gut << ", abgelehnt: " << schlecht
        << ", ohne Graphen: " << ohne << "\n";
    aus << "Gegenprobe: " << gegenprobe << " abgelehnt, " << durchgelassen
        << " durchgelassen\n";
    aus << "Volle Benotung: " << ganzGut << " anerkannt, " << ganzSchlecht
        << " nicht; falsche Antworten: " << falschErkannt << " erkannt, "
        << falschDurch << " durchgelassen\n";
    return (schlecht || durchgelassen || ganzSchlecht || falschDurch) ? 1 : 0;
}
