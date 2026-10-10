#ifndef FORMAT_H
#define FORMAT_H

// Zahlen und Zeiten lesbar machen -- gleich auf beiden Plattformen.

#include <QString>
#include <QVariantMap>

namespace Format
{

// Ein Preis kommt als `{"code":"EUR","minor_units":499,"decimals":2}`.
// Gerechnet wird in ganzen Zahlen: 499 mit zwei Dezimalen ist 4,99 -- wer das
// durch ein double schickt, bekommt irgendwann 4,99 als 4,989999.
QString preis(const QVariantMap &roh);

// Nur die Zahl, ohne Waehrung (fuer "statt 15,00").
QString preisZahl(const QVariantMap &roh);

// Die Abholzeit steht als ISO-Zeitpunkt in UTC (`2026-10-09T15:00:00Z`).
// Gezeigt wird Ortszeit, und zwar als "heute 17:00-21:00" bzw. mit Datum,
// wenn es nicht heute oder morgen ist.
QString abholfenster(const QString &von, const QString &bis);

// Entfernung: unter einem Kilometer in Metern.
QString entfernung(double km);

// Ein ISO-Zeitpunkt als Ortszeit, kurz.
QString zeitpunkt(const QString &iso);

}

#endif // FORMAT_H
