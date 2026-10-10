#include "format.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QLocale>
#include <QStringList>

namespace {

QString waehrungszeichen(const QString &code)
{
    if (code == QLatin1String("EUR"))
        return QString::fromUtf8("€");
    if (code == QLatin1String("GBP"))
        return QString::fromUtf8("£");
    if (code == QLatin1String("USD"))
        return QLatin1String("$");
    if (code == QLatin1String("CHF"))
        return QLatin1String("CHF");
    if (code == QLatin1String("DKK") || code == QLatin1String("NOK")
        || code == QLatin1String("SEK"))
        return QLatin1String("kr");
    return code;
}

QDateTime ausIso(const QString &iso)
{
    if (iso.isEmpty())
        return QDateTime();
    QString s = iso;
    // Der Dienst schickt `2026-10-09T15:00:00Z`, manchmal mit Bruchteilen.
    const int punkt = s.indexOf(QLatin1Char('.'));
    if (punkt > 0) {
        int ende = punkt;
        while (ende < s.size() && (s.at(ende).isDigit() || ende == punkt))
            ++ende;
        s.remove(punkt, ende - punkt);
    }
    QDateTime t = QDateTime::fromString(s, Qt::ISODate);
    if (!t.isValid())
        return t;
    // Qt 4.7 liest das `Z` als Ortszeit, Qt 5 nicht -- beide auf UTC setzen
    // und dann in Ortszeit wandeln, damit es gleich herauskommt.
    if (s.endsWith(QLatin1Char('Z')) || s.endsWith(QLatin1String("+00:00")))
        t.setTimeSpec(Qt::UTC);
    return t.toLocalTime();
}

}  // namespace

QString Format::preisZahl(const QVariantMap &roh)
{
    if (roh.isEmpty())
        return QString();
    const qlonglong einheiten = roh.value(QLatin1String("minor_units")).toLongLong();
    int dezimalen = roh.value(QLatin1String("decimals")).toInt();
    if (dezimalen < 0 || dezimalen > 6)
        dezimalen = 2;
    qlonglong teiler = 1;
    for (int i = 0; i < dezimalen; ++i)
        teiler *= 10;
    const QLocale ort;
    if (dezimalen == 0)
        return ort.toString(einheiten);
    const qlonglong ganz = einheiten / teiler;
    const qlonglong rest = einheiten % teiler;
    return ort.toString(ganz) + ort.decimalPoint()
           + QString::number(rest).rightJustified(dezimalen, QLatin1Char('0'));
}

QString Format::preis(const QVariantMap &roh)
{
    if (roh.isEmpty())
        return QString();
    const QString zahl = preisZahl(roh);
    if (zahl.isEmpty())
        return QString();
    return zahl + QLatin1Char(' ')
           + waehrungszeichen(roh.value(QLatin1String("code")).toString());
}

QString Format::zeitpunkt(const QString &iso)
{
    const QDateTime t = ausIso(iso);
    if (!t.isValid())
        return QString();
    return QLocale().toString(t, QLatin1String("d. MMM, HH:mm"));
}

QString Format::abholfenster(const QString &von, const QString &bis)
{
    const QDateTime a = ausIso(von);
    const QDateTime b = ausIso(bis);
    if (!a.isValid())
        return QString();

    const QDate heute = QDate::currentDate();
    QString tag;
    if (a.date() == heute)
        tag = QCoreApplication::translate("Format", "heute");
    else if (a.date() == heute.addDays(1))
        tag = QCoreApplication::translate("Format", "morgen");
    else
        tag = QLocale().toString(a.date(), QLatin1String("ddd d. MMM"));

    const QString uhr = QLocale().toString(a.time(), QLatin1String("HH:mm"));
    if (!b.isValid())
        return tag + QLatin1Char(' ') + uhr;

    const QString bisUhr = QLocale().toString(b.time(), QLatin1String("HH:mm"));
    // Reicht das Fenster ueber Mitternacht, dann den zweiten Tag dazu.
    if (b.date() != a.date())
        return tag + QLatin1Char(' ') + uhr + QString::fromUtf8("–")
               + QLocale().toString(b.date(), QLatin1String("ddd")) + QLatin1Char(' ')
               + bisUhr;
    return tag + QLatin1Char(' ') + uhr + QString::fromUtf8("–") + bisUhr;
}

QString Format::entfernung(double km)
{
    if (km < 0)
        return QString();
    if (km < 1.0)
        return QCoreApplication::translate("Format", "%1 m")
            .arg(qRound(km * 1000.0 / 10.0) * 10);
    if (km < 10.0)
        return QCoreApplication::translate("Format", "%1 km")
            .arg(QLocale().toString(km, 'f', 1));
    return QCoreApplication::translate("Format", "%1 km").arg(qRound(km));
}
