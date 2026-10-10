#ifndef BENOTER_H
#define BENOTER_H

// Die Benotung einer Antwort.
//
// **Benotet wird im Client, der Server glaubt es.** Beim `PUT /sessions/{id}`
// traegt jede Aufgabe ihr `correct` selbst. Eine falsch gerechnete Benotung
// verdirbt deshalb den echten Kursfortschritt -- nicht nur die Anzeige.
//
// Darum ist die Regel nicht erfunden, sondern aus der Weboberflaeche
// abgeschrieben (doc/aufgabenarten.md) und gegen echte Rohdaten geprueft:
// jede Aufgabe muss ihre eigene Musterloesung annehmen
// (tools/benoter-pruefen.py, dieselbe Rechnung in Python).
//
// Zwei Wege, je nach Aufgabe:
//   * **Angetippte Antworten** (`choices` + `correctIndices`): die Reihe der
//     angetippten Felder muss die richtige sein. Das ist der haeufige Fall.
//   * **Getippte Antworten**: dafuer bringt jede Aufgabe einen Graphen mit
//     (`grader`), einen Automaten ueber die Woerter der Antwort.

#include <QString>
#include <QStringList>
#include <QVariantMap>

namespace Benoter
{

// Zerlegt eine Antwort in die Stuecke, die der Graph liest: Woerter **und**
// die Leerzeichen dazwischen, jedes als eigenes Stueck -- im Graphen ist das
// Leerzeichen eine eigene Kante, kein Trenner.
QStringList zerlegen(const QString &antwort, bool mitAbstand = true);

// Kleinschreibung, Satzzeichen weg. Der Abstand bleibt.
QString glaetten(const QString &wort);

// Laeuft den Graphen der Aufgabe (`challenge.grader`) mit der Antwort ab.
bool graphNimmtAn(const QVariantMap &graph, const QString &antwort);

// Die Benotung einer ganzen Aufgabe. `antwort` ist der zusammengesetzte
// Text, `reihe` die angetippten Felder (leer, wenn getippt wurde).
bool benoten(const QVariantMap &aufgabe, const QString &antwort,
             const QList<int> &reihe);

// Die Musterloesung, die in der Aufgabe selbst steht -- fuer die Anzeige
// nach einer falschen Antwort und fuer die Probe.
QString musterloesung(const QVariantMap &aufgabe);

}

#endif // BENOTER_H
