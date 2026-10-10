#ifndef RECHNER_H
#define RECHNER_H

// Die Rechenaufgaben.
//
// Duolingos Rechenkurs schickt keine Saetze, sondern einen kleinen
// Formelbaum (doc/WAS-NOCH-FEHLT.md):
//
//   promptInputChallenge
//     prompt.instruction.text              "Type the answer"
//     prompt.body.mathSequenceElement.elements[]
//         taggedTextElement {text:"3"}     ein Stueck der Zeile
//         blankElement                     die Luecke
//     input.productSelectInput             eine Wahl aus mehreren
//     input.typeFillInput                  eine Zahl eintippen
//     input.expressionBuildInput           die Zeile aus Bausteinen legen
//
// Hier wird daraus etwas, das QML zeichnen kann -- und die Benotung, die
// zur jeweiligen Eingabeart gehoert. Benotet wird (wie bei den Sprachen)
// im Client: die richtige Antwort steht in der Aufgabe selbst.

#include <QString>
#include <QVariantList>
#include <QVariantMap>

namespace Rechner
{

// Ist das ueberhaupt eine Rechenaufgabe?
bool istRechnen(const QVariantMap &aufgabe);

// Die Aufgabe flach, so wie die Oberflaeche sie braucht:
//   anweisung  "Type the answer"
//   zeile      [{text, luecke}]  -- die Formel, Stueck fuer Stueck
//   wahlen     [{text, nummer}]  -- bei der Wahl und beim Bauen
//   eingabe    "wahl" | "tippen" | "bauen"
//   plaetze    wie viele Bausteine die Zeile aufnimmt (beim Bauen)
QVariantMap darstellen(const QVariantMap &aufgabe);

// Benoten. `reihe` sind die angetippten Felder (Wahl oder Bausteine in
// ihrer Reihenfolge), `text` das Eingetippte.
bool benoten(const QVariantMap &aufgabe, const QVariantList &reihe,
             const QString &text);

// Die Musterloesung, fuer das Band nach einer falschen Antwort.
QString loesung(const QVariantMap &aufgabe);

// Eine Kette aus Zahlen und Zeichen ausrechnen ("3 + 4" -> 7).
// Punkt vor Strich, keine Klammern -- mehr kommt in diesen Aufgaben nicht
// vor. `ging` sagt, ob die Kette ueberhaupt eine Rechnung war.
double ausrechnen(const QStringList &stuecke, bool *ging = 0);

}

#endif // RECHNER_H
