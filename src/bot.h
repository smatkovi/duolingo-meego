#ifndef BOT_H
#define BOT_H

// Der Gegner.
//
// Duolingos Dienst fuehrt nur Buch -- den Gegenzug muss der Client selbst
// finden (doc/SCHACH.md). Hier steht dafuer eine kleine Suche: Alpha-Beta
// ueber ein paar Halbzuege, bewertet nach Material, Beweglichkeit und ein
// paar Feldern. Das reicht fuer die Staerke, um die es geht -- Duolingos
// Bots fangen bei 200 Elo an --, und es passt in ein Paket, das auf einem
// N9 laufen soll: kein Stockfish, keine 40 MB Netz.
//
// Gerechnet wird mit `schach::core::Position`, also mit demselben Brett,
// das auch die Oberflaeche sieht.

#include <string>

#ifndef Q_MOC_RUN
#include "core/Position.h"
#endif

namespace Bot
{

// Der beste Zug in dieser Stellung, als UCI. Leer, wenn es keinen gibt.
// `staerke` 0-4: wie tief gesucht wird (0 = nur schlagen und nicht
// einstellen, 4 = ein paar Halbzuege voraus).
std::string zug(const schach::core::Position &stellung, int staerke);

// Die Bewertung einer Stellung aus Sicht des Anziehenden, in Hundertstel
// Bauern. Offen, weil die Probe sie braucht.
int bewerten(const schach::core::Position &stellung);

}

#endif // BOT_H
