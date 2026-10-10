#include "rechner.h"

#include <QStringList>
#include <QtGlobal>

#include <cmath>

namespace {

// Die Zeichen, die der Formelbaum benennt. Im Text stehen sie als
// HTML-Schnipsel ("<span>+</span>"), darum werden sie hier aus dem
// Eintrag `symbol` genommen und nicht aus der Beschriftung.
QString zeichenFuer(const QString &symbol)
{
    if (symbol == QLatin1String("ADDITION"))
        return QLatin1String("+");
    if (symbol == QLatin1String("SUBTRACTION"))
        return QString::fromUtf8("\xe2\x88\x92");   // Minuszeichen
    if (symbol == QLatin1String("MULTIPLICATION"))
        return QString::fromUtf8("\xc3\x97");       // Malzeichen
    if (symbol == QLatin1String("DIVISION"))
        return QString::fromUtf8("\xc3\xb7");
    if (symbol == QLatin1String("EQUALITY"))
        return QLatin1String("=");
    if (symbol == QLatin1String("GREATER_THAN"))
        return QLatin1String(">");
    if (symbol == QLatin1String("LESS_THAN"))
        return QLatin1String("<");
    return QString();
}

// Aus einem Element die Beschriftung holen: entweder das Zeichen oder der
// Text ohne seine HTML-Huelle.
QString beschriftung(const QVariantMap &element)
{
    const QVariantMap wesen = element.value(QLatin1String("underlyingEntity"))
                                  .toMap().value(QLatin1String("entity")).toMap();
    const QString zeichen = zeichenFuer(wesen.value(QLatin1String("symbol")).toString());
    if (!zeichen.isEmpty())
        return zeichen;
    QString text = element.value(QLatin1String("taggedTextElement")).toMap()
                       .value(QLatin1String("text")).toString();
    text.remove(QLatin1String("<span>"));
    text.remove(QLatin1String("</span>"));
    return text;
}

// Der Zahlenwert hinter einem Element, soweit es einen hat.
bool wertVon(const QVariantMap &element, double *wert)
{
    const QVariantMap wesen = element.value(QLatin1String("underlyingEntity"))
                                  .toMap().value(QLatin1String("entity")).toMap();
    if (wesen.contains(QLatin1String("integer"))) {
        *wert = wesen.value(QLatin1String("integer")).toMap()
                    .value(QLatin1String("integer")).toDouble();
        return true;
    }
    if (wesen.contains(QLatin1String("decimal"))) {
        *wert = wesen.value(QLatin1String("decimal")).toMap()
                    .value(QLatin1String("decimal")).toDouble();
        return true;
    }
    return false;
}

QVariantMap eingabeVon(const QVariantMap &aufgabe)
{
    return aufgabe.value(QLatin1String("promptInputChallenge")).toMap()
        .value(QLatin1String("input")).toMap();
}

// Die richtige Zahl, wie sie in der Eingabe steht.
bool richtigeZahl(const QVariantMap &aufgabe, double *wert)
{
    const QVariantMap eingabe = eingabeVon(aufgabe);
    const char *wege[] = { "typeFillInput", "expressionBuildInput", 0 };
    for (int i = 0; wege[i]; ++i) {
        const QVariantMap teil = eingabe.value(QLatin1String(wege[i])).toMap();
        if (teil.isEmpty())
            continue;
        const QVariantMap antwort = teil.value(QLatin1String("correctAnswer")).toMap();
        if (antwort.contains(QLatin1String("integer"))) {
            *wert = antwort.value(QLatin1String("integer")).toMap()
                        .value(QLatin1String("integer")).toDouble();
            return true;
        }
        if (antwort.contains(QLatin1String("decimal"))) {
            *wert = antwort.value(QLatin1String("decimal")).toMap()
                        .value(QLatin1String("decimal")).toDouble();
            return true;
        }
    }
    return false;
}

}  // namespace

bool Rechner::istRechnen(const QVariantMap &aufgabe)
{
    return aufgabe.contains(QLatin1String("promptInputChallenge"));
}

double Rechner::ausrechnen(const QStringList &stuecke, bool *ging)
{
    // Punkt vor Strich, keine Klammern: erst die Produkte zusammenziehen,
    // dann die Summen. Mehr steht in diesen Aufgaben nicht.
    if (ging)
        *ging = false;
    QList<double> zahlen;
    QStringList rechenzeichen;
    bool warteAufZahl = true;
    for (int i = 0; i < stuecke.size(); ++i) {
        const QString s = stuecke.at(i).trimmed();
        if (s.isEmpty())
            continue;
        if (warteAufZahl) {
            bool ok = false;
            const double z = s.toDouble(&ok);
            if (!ok)
                return 0;
            zahlen.append(z);
            warteAufZahl = false;
        } else {
            if (s == QLatin1String("=") )
                return 0;
            rechenzeichen.append(s);
            warteAufZahl = true;
        }
    }
    if (zahlen.isEmpty() || warteAufZahl)
        return 0;

    for (int i = 0; i < rechenzeichen.size();) {
        const QString z = rechenzeichen.at(i);
        if (z == QString::fromUtf8("\xc3\x97") || z == QLatin1String("*")
            || z == QString::fromUtf8("\xc3\xb7") || z == QLatin1String("/")) {
            const double a = zahlen.at(i);
            const double b = zahlen.at(i + 1);
            const bool mal = (z == QString::fromUtf8("\xc3\x97")
                              || z == QLatin1String("*"));
            if (!mal && qFuzzyCompare(b + 1.0, 1.0))
                return 0;
            zahlen[i] = mal ? a * b : a / b;
            zahlen.removeAt(i + 1);
            rechenzeichen.removeAt(i);
        } else {
            ++i;
        }
    }
    double summe = zahlen.isEmpty() ? 0 : zahlen.at(0);
    for (int i = 0; i < rechenzeichen.size(); ++i) {
        const QString z = rechenzeichen.at(i);
        const double b = zahlen.at(i + 1);
        if (z == QLatin1String("+"))
            summe += b;
        else if (z == QString::fromUtf8("\xe2\x88\x92") || z == QLatin1String("-"))
            summe -= b;
        else
            return 0;
    }
    if (ging)
        *ging = true;
    return summe;
}

QVariantMap Rechner::darstellen(const QVariantMap &aufgabe)
{
    QVariantMap k;
    const QVariantMap stueck =
        aufgabe.value(QLatin1String("promptInputChallenge")).toMap();
    const QVariantMap frage = stueck.value(QLatin1String("prompt")).toMap();
    k.insert(QLatin1String("anweisung"),
             frage.value(QLatin1String("instruction")).toMap()
                 .value(QLatin1String("text")));

    // Die Zeile: jedes Element entweder Text oder Luecke.
    QVariantList zeile;
    const QVariantList elemente = frage.value(QLatin1String("body")).toMap()
        .value(QLatin1String("mathSequenceElement")).toMap()
        .value(QLatin1String("elements")).toList();
    for (int i = 0; i < elemente.size(); ++i) {
        const QVariantMap e = elemente.at(i).toMap();
        QVariantMap teil;
        const bool luecke = e.contains(QLatin1String("blankElement"));
        teil.insert(QLatin1String("luecke"), luecke);
        teil.insert(QLatin1String("text"), luecke ? QString() : beschriftung(e));
        zeile.append(teil);
    }
    k.insert(QLatin1String("zeile"), zeile);

    const QVariantMap eingabe = eingabeVon(aufgabe);
    QVariantList wahlen;

    const QVariantMap wahl = eingabe.value(QLatin1String("productSelectInput")).toMap();
    if (!wahl.isEmpty()) {
        const QVariantList optionen = wahl.value(QLatin1String("answerOptions")).toList();
        for (int i = 0; i < optionen.size(); ++i) {
            QVariantMap w;
            w.insert(QLatin1String("text"),
                     beschriftung(optionen.at(i).toMap()
                                      .value(QLatin1String("choice")).toMap()));
            w.insert(QLatin1String("nummer"), i);
            wahlen.append(w);
        }
        k.insert(QLatin1String("eingabe"), QLatin1String("wahl"));
        k.insert(QLatin1String("wahlen"), wahlen);
        return k;
    }

    const QVariantMap bauen = eingabe.value(QLatin1String("expressionBuildInput")).toMap();
    if (!bauen.isEmpty()) {
        const QVariantList bausteine = bauen.value(QLatin1String("dragChoices")).toList();
        for (int i = 0; i < bausteine.size(); ++i) {
            QVariantMap w;
            w.insert(QLatin1String("text"), beschriftung(bausteine.at(i).toMap()));
            w.insert(QLatin1String("nummer"), i);
            wahlen.append(w);
        }
        k.insert(QLatin1String("eingabe"), QLatin1String("bauen"));
        k.insert(QLatin1String("wahlen"), wahlen);
        k.insert(QLatin1String("plaetze"), bauen.value(QLatin1String("slotCount"), 3));
        return k;
    }

    const QVariantMap tippen = eingabe.value(QLatin1String("typeFillInput")).toMap();
    k.insert(QLatin1String("eingabe"), QLatin1String("tippen"));
    k.insert(QLatin1String("platzhalter"),
             tippen.value(QLatin1String("placeholderText")).toMap()
                 .value(QLatin1String("text")));
    k.insert(QLatin1String("wahlen"), wahlen);
    return k;
}

bool Rechner::benoten(const QVariantMap &aufgabe, const QVariantList &reihe,
                      const QString &text)
{
    const QVariantMap eingabe = eingabeVon(aufgabe);

    const QVariantMap wahl = eingabe.value(QLatin1String("productSelectInput")).toMap();
    if (!wahl.isEmpty()) {
        if (reihe.size() != 1)
            return false;
        const QVariantList richtige = wahl.value(QLatin1String("correctIndices")).toList();
        for (int i = 0; i < richtige.size(); ++i)
            if (richtige.at(i).toInt() == reihe.first().toInt())
                return true;
        return false;
    }

    const QVariantMap bauen = eingabe.value(QLatin1String("expressionBuildInput")).toMap();
    if (!bauen.isEmpty()) {
        // Benotet wird nach **Wert**, nicht nach Gestalt: "3 + 4" und
        // "4 + 3" sind beide richtig, wenn 7 herauskommen soll
        // (gradingMethod: VALUE).
        const QVariantList bausteine = bauen.value(QLatin1String("dragChoices")).toList();
        QStringList gelegt;
        for (int i = 0; i < reihe.size(); ++i) {
            const int n = reihe.at(i).toInt();
            if (n >= 0 && n < bausteine.size())
                gelegt << beschriftung(bausteine.at(n).toMap());
        }
        bool ging = false;
        const double wert = ausrechnen(gelegt, &ging);
        double soll = 0;
        if (!ging || !richtigeZahl(aufgabe, &soll))
            return false;
        return qAbs(wert - soll) < 1e-9;
    }

    bool ok = false;
    const double gegeben = text.trimmed().toDouble(&ok);
    double soll = 0;
    if (!ok || !richtigeZahl(aufgabe, &soll))
        return false;
    return qAbs(gegeben - soll) < 1e-9;
}

QString Rechner::loesung(const QVariantMap &aufgabe)
{
    const QVariantMap eingabe = eingabeVon(aufgabe);
    const QVariantMap wahl = eingabe.value(QLatin1String("productSelectInput")).toMap();
    if (!wahl.isEmpty()) {
        const QVariantList richtige = wahl.value(QLatin1String("correctIndices")).toList();
        const QVariantList optionen = wahl.value(QLatin1String("answerOptions")).toList();
        if (!richtige.isEmpty()) {
            const int n = richtige.first().toInt();
            if (n >= 0 && n < optionen.size())
                return beschriftung(optionen.at(n).toMap()
                                        .value(QLatin1String("choice")).toMap());
        }
        return QString();
    }
    double soll = 0;
    if (richtigeZahl(aufgabe, &soll)) {
        if (qAbs(soll - qRound(soll)) < 1e-9)
            return QString::number((qlonglong) qRound(soll));
        return QString::number(soll);
    }
    return QString();
}
