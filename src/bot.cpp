#include "bot.h"

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <vector>

namespace {

using schach::core::Position;

const int MATT = 30000;

// Wie gern eine Figur wo steht, in Hundertstel Bauern. Nur grob -- es soll
// verhindern, dass der Gegner im Eroeffnungsnebel die Raender entlangkriecht,
// nicht Meisterschach spielen. Die Tafel ist aus Sicht von Weiss; fuer
// Schwarz wird sie gespiegelt.
const int BAUER[64] = {
      0,  0,  0,  0,  0,  0,  0,  0,
      5, 10, 10,-20,-20, 10, 10,  5,
      5, -5,-10,  0,  0,-10, -5,  5,
      0,  0,  0, 20, 20,  0,  0,  0,
      5,  5, 10, 25, 25, 10,  5,  5,
     10, 10, 20, 30, 30, 20, 10, 10,
     50, 50, 50, 50, 50, 50, 50, 50,
      0,  0,  0,  0,  0,  0,  0,  0
};

const int SPRINGER[64] = {
    -50,-40,-30,-30,-30,-30,-40,-50,
    -40,-20,  0,  5,  5,  0,-20,-40,
    -30,  5, 10, 15, 15, 10,  5,-30,
    -30,  0, 15, 20, 20, 15,  0,-30,
    -30,  5, 15, 20, 20, 15,  5,-30,
    -30,  0, 10, 15, 15, 10,  0,-30,
    -40,-20,  0,  0,  0,  0,-20,-40,
    -50,-40,-30,-30,-30,-30,-40,-50
};

const int LAEUFER[64] = {
    -20,-10,-10,-10,-10,-10,-10,-20,
    -10,  5,  0,  0,  0,  0,  5,-10,
    -10, 10, 10, 10, 10, 10, 10,-10,
    -10,  0, 10, 10, 10, 10,  0,-10,
    -10,  5,  5, 10, 10,  5,  5,-10,
    -10,  0,  5, 10, 10,  5,  0,-10,
    -10,  0,  0,  0,  0,  0,  0,-10,
    -20,-10,-10,-10,-10,-10,-10,-20
};

int tafelwert(char art, int feld, bool weiss)
{
    const int i = weiss ? feld : (56 - 8 * (feld / 8) + feld % 8);
    switch (art) {
    case 'p': return BAUER[i];
    case 'n': return SPRINGER[i];
    case 'b': return LAEUFER[i];
    default:  return 0;
    }
}

int materialwert(char art)
{
    switch (art) {
    case 'p': return 100;
    case 'n': return 320;
    case 'b': return 330;
    case 'r': return 500;
    case 'q': return 900;
    default:  return 0;
    }
}

// Die Stellung Feld fuer Feld durchgehen. Position gibt uns dafuer die
// FEN -- das ist langsamer als in die Bibliothek zu greifen, aber es haelt
// diese Datei von chess.hpp unabhaengig und reicht bei der Suchtiefe.
int zaehlen(const Position &stellung)
{
    const std::string fen = stellung.fen();
    int punkte = 0;
    int feld = 56;  // a8
    for (std::string::size_type i = 0; i < fen.size(); ++i) {
        const char c = fen[i];
        if (c == ' ')
            break;
        if (c == '/') {
            feld -= 16;
            continue;
        }
        if (c >= '1' && c <= '8') {
            feld += c - '0';
            continue;
        }
        const bool weiss = (c >= 'A' && c <= 'Z');
        const char art = weiss ? (char)(c - 'A' + 'a') : c;
        const int wert = materialwert(art) + tafelwert(art, feld, weiss);
        punkte += weiss ? wert : -wert;
        ++feld;
    }
    return punkte;
}

int suchen(Position &stellung, int tiefe, int alpha, int beta)
{
    const schach::core::Outcome ausgang = stellung.outcome();
    if (ausgang != schach::core::Outcome::Ongoing) {
        if (ausgang == schach::core::Outcome::Draw)
            return 0;
        const bool weissGewinnt = (ausgang == schach::core::Outcome::WhiteWins);
        const int wert = weissGewinnt ? MATT : -MATT;
        // Ein Matt in zwei Zuegen ist besser als eines in vier.
        return (stellung.whiteToMove() ? 1 : -1) * wert
               * (tiefe + 1) / (tiefe + 1);
    }
    if (tiefe <= 0)
        return stellung.whiteToMove() ? zaehlen(stellung) : -zaehlen(stellung);

    const std::vector<std::string> zuege = stellung.legalMoves();
    int beste = -MATT * 2;
    for (std::vector<std::string>::const_iterator z = zuege.begin();
         z != zuege.end(); ++z) {
        Position naechste = stellung;
        if (!naechste.play(*z))
            continue;
        const int wert = -suchen(naechste, tiefe - 1, -beta, -alpha);
        if (wert > beste)
            beste = wert;
        if (beste > alpha)
            alpha = beste;
        if (alpha >= beta)
            break;
    }
    return beste;
}

}  // namespace

int Bot::bewerten(const Position &stellung)
{
    return zaehlen(stellung);
}

std::string Bot::zug(const Position &stellung, int staerke)
{
    const std::vector<std::string> zuege = stellung.legalMoves();
    if (zuege.empty())
        return std::string();

    // Tiefe nach Staerke. Mehr als vier Halbzuege rechnet ein OMAP3 nicht
    // in der Zeit, die jemand vor dem Brett zu warten bereit ist.
    int tiefe = 1;
    if (staerke >= 4)
        tiefe = 4;
    else if (staerke == 3)
        tiefe = 3;
    else if (staerke == 2)
        tiefe = 2;

    std::string bester;
    int besterWert = -MATT * 2;
    for (std::vector<std::string>::const_iterator z = zuege.begin();
         z != zuege.end(); ++z) {
        Position naechste = stellung;
        if (!naechste.play(*z))
            continue;
        int wert = -suchen(naechste, tiefe - 1, -MATT * 2, MATT * 2);
        // Auf den unteren Stufen spielt der Gegner nicht das Beste, sondern
        // etwas Annehmbares -- sonst macht eine Partie gegen ihn keinen
        // Spass und keine Elo-Zahl stimmt mehr.
        if (staerke <= 1)
            wert += (std::rand() % 60) - 30;
        if (wert > besterWert) {
            besterWert = wert;
            bester = *z;
        }
    }
    return bester;
}
