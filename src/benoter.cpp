#include "benoter.h"

#include <QSet>
#include <QStringList>
#include <QVariantList>

namespace {

QString textVon(const QVariant &wahl)
{
    if (wahl.type() == QVariant::Map)
        return wahl.toMap().value(QLatin1String("text")).toString();
    return wahl.toString();
}

}  // namespace

QString Benoter::glaetten(const QString &wort)
{
    QString raus;
    raus.reserve(wort.size());
    for (int i = 0; i < wort.size(); ++i) {
        const QChar c = wort.at(i);
        // Satzzeichen fallen weg -- `lenient` im Graphen ist schon so
        // geschrieben. Zeichen wie ¿ und ¡ sind Qt::Punctuation_Initial
        // bzw. _Final, "." ist _Other: alle Punctuation_* mitnehmen.
        if (c.category() >= QChar::Punctuation_Connector
            && c.category() <= QChar::Punctuation_Other)
            continue;
        raus.append(c.toLower());
    }
    return raus;
}

QStringList Benoter::zerlegen(const QString &antwort, bool mitAbstand)
{
    const QString geglaettet = glaetten(antwort);
    QStringList stuecke;
    if (!mitAbstand) {
        for (int i = 0; i < geglaettet.size(); ++i)
            if (!geglaettet.at(i).isSpace())
                stuecke << QString(geglaettet.at(i));
        return stuecke;
    }
    const QStringList worte = geglaettet.split(QLatin1Char(' '));
    for (int i = 0; i < worte.size(); ++i) {
        const QString wort = worte.at(i).trimmed();
        if (wort.isEmpty())
            continue;
        if (!stuecke.isEmpty())
            stuecke << QLatin1String(" ");
        stuecke << wort;
    }
    return stuecke;
}

namespace {

// Leerschritte mitnehmen: Kanten mit leerem `lenient`.
QSet<int> schluss(const QVariantList &knoten, const QSet<int> &menge)
{
    QSet<int> gesehen = menge;
    QList<int> offen = menge.toList();
    while (!offen.isEmpty()) {
        const int i = offen.takeLast();
        if (i < 0 || i >= knoten.size())
            continue;
        const QVariantList kanten = knoten.at(i).toList();
        for (int k = 0; k < kanten.size(); ++k) {
            const QVariantMap kante = kanten.at(k).toMap();
            if (!kante.value(QLatin1String("lenient")).toString().isEmpty())
                continue;
            const int nach = kante.value(QLatin1String("to")).toInt();
            if (!gesehen.contains(nach)) {
                gesehen.insert(nach);
                offen << nach;
            }
        }
    }
    return gesehen;
}

}  // namespace

bool Benoter::graphNimmtAn(const QVariantMap &graph, const QString &antwort)
{
    const QVariantList knoten = graph.value(QLatin1String("vertices")).toList();
    if (knoten.isEmpty())
        return false;
    const bool mitAbstand =
        graph.value(QLatin1String("whitespaceDelimited"), true).toBool();
    const QStringList stuecke = zerlegen(antwort, mitAbstand);

    QSet<int> start;
    start.insert(0);
    QSet<int> hier = schluss(knoten, start);

    for (int s = 0; s < stuecke.size(); ++s) {
        const QString stueck = stuecke.at(s);
        QSet<int> weiter;
        QSet<int>::const_iterator it;
        for (it = hier.constBegin(); it != hier.constEnd(); ++it) {
            if (*it < 0 || *it >= knoten.size())
                continue;
            const QVariantList kanten = knoten.at(*it).toList();
            for (int k = 0; k < kanten.size(); ++k) {
                const QVariantMap kante = kanten.at(k).toMap();
                const QString erwartet =
                    glaetten(kante.value(QLatin1String("lenient")).toString());
                if (!erwartet.isEmpty() && erwartet == stueck)
                    weiter.insert(kante.value(QLatin1String("to")).toInt());
            }
        }
        if (weiter.isEmpty())
            return false;
        hier = schluss(knoten, weiter);
    }

    // Angenommen, wenn ein Endknoten erreicht ist: der hat keine Kante mehr.
    QSet<int>::const_iterator it;
    for (it = hier.constBegin(); it != hier.constEnd(); ++it)
        if (*it >= 0 && *it < knoten.size() && knoten.at(*it).toList().isEmpty())
            return true;
    return false;
}

bool Benoter::benoten(const QVariantMap &aufgabe, const QString &antwort,
                      const QList<int> &reihe)
{
    // Eine Wahl aus mehreren (`assist`, `judge`, `select`, …).
    if (aufgabe.contains(QLatin1String("correctIndex"))) {
        if (reihe.size() != 1)
            return false;
        return reihe.first() == aufgabe.value(QLatin1String("correctIndex")).toInt();
    }

    // Angetippte Woerter: zuerst die Reihe vergleichen. Die Weboberflaeche
    // macht es genauso und faellt erst danach auf den Graphen zurueck --
    // eine andere, aber gueltige Reihenfolge faengt der Graph auf.
    if (!reihe.isEmpty() && aufgabe.contains(QLatin1String("correctIndices"))) {
        const QVariantList richtige =
            aufgabe.value(QLatin1String("correctIndices")).toList();
        if (richtige.size() == reihe.size()) {
            bool gleich = true;
            for (int i = 0; i < richtige.size(); ++i)
                if (richtige.at(i).toInt() != reihe.at(i)) {
                    gleich = false;
                    break;
                }
            if (gleich)
                return true;
        }
    }

    // Zwei Spalten zusammenbringen (`match`, `listenMatch`): die
    // Weboberflaeche benotet das als richtig, sobald ueberhaupt eine
    // Antwort da ist -- die Darstellung laesst nur richtige Paare zu und
    // ist erst fertig, wenn alle liegen.
    if (aufgabe.contains(QLatin1String("pairs")))
        return !antwort.isEmpty();

    const QVariantMap graph = aufgabe.value(QLatin1String("grader")).toMap();
    if (!graph.isEmpty())
        return graphNimmtAn(graph, antwort);

    // Ohne Graphen bleibt der Textvergleich gegen die Musterloesungen.
    const QVariantList loesungen =
        aufgabe.value(QLatin1String("correctSolutions")).toList();
    const QString gegeben = glaetten(antwort).simplified();
    for (int i = 0; i < loesungen.size(); ++i)
        if (glaetten(loesungen.at(i).toString()).simplified() == gegeben)
            return true;
    return false;
}

QString Benoter::musterloesung(const QVariantMap &aufgabe)
{
    const QVariantMap verfolg =
        aufgabe.value(QLatin1String("challengeResponseTrackingProperties")).toMap();
    const QString beste = verfolg.value(QLatin1String("best_solution")).toString();
    if (!beste.isEmpty())
        return beste;

    const QVariantList loesungen =
        aufgabe.value(QLatin1String("correctSolutions")).toList();
    if (!loesungen.isEmpty())
        return loesungen.first().toString();

    const QVariantList wahlen = aufgabe.value(QLatin1String("choices")).toList();
    if (aufgabe.contains(QLatin1String("correctIndex"))) {
        const int i = aufgabe.value(QLatin1String("correctIndex")).toInt();
        if (i >= 0 && i < wahlen.size())
            return textVon(wahlen.at(i));
    }
    const QVariantList reihe =
        aufgabe.value(QLatin1String("correctIndices")).toList();
    if (!reihe.isEmpty() && !wahlen.isEmpty()) {
        QStringList teile;
        for (int i = 0; i < reihe.size(); ++i) {
            const int k = reihe.at(i).toInt();
            if (k >= 0 && k < wahlen.size())
                teile << textVon(wahlen.at(k));
        }
        return teile.join(QLatin1String(" "));
    }
    return aufgabe.value(QLatin1String("prompt")).toString();
}
