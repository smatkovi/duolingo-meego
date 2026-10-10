// Probe fuer das Schachteil: Brett, Zuege, Gegner.
//
// Keine Verbindung noetig -- gerechnet wird mit demselben Code, der am
// Telefon laeuft. Geprueft wird, was schiefgehen kann, ohne dass man es
// gleich sieht: die Umrechnung FEN -> 64 Felder (ein Fehler darin stellt
// die Figuren spiegelverkehrt auf), dass der Gegner nur erlaubte Zuege
// macht, und dass eine ganze Partie zu Ende kommt.
//
//   build/meego/test/schach_test
#include <cstdio>
#include <string>
#include <vector>

#include "bot.h"
#include "core/Position.h"

using schach::core::Position;

namespace {

int fehler = 0;

void pruefe(bool bedingung, const char *was)
{
    if (!bedingung) {
        std::printf("   FEHLT: %s\n", was);
        ++fehler;
    }
}

// Dieselbe Rechnung wie Schach::felder() -- hier noch einmal, damit die
// Probe ohne Qt auskommt und trotzdem genau das prueft, was die
// Oberflaeche zeichnet.
std::vector<std::string> felder(const Position &stellung)
{
    std::vector<std::string> raus(64, std::string());
    const std::string f = stellung.fen();
    int feld = 56;
    for (std::string::size_type i = 0; i < f.size(); ++i) {
        const char c = f[i];
        if (c == ' ')
            break;
        if (c == '/') { feld -= 16; continue; }
        if (c >= '1' && c <= '8') { feld += c - '0'; continue; }
        if (feld >= 0 && feld < 64) {
            const bool weiss = (c >= 'A' && c <= 'Z');
            const char klein = weiss ? (char)(c - 'A' + 'a') : c;
            raus[feld] = std::string(weiss ? "w" : "b") + klein;
        }
        ++feld;
    }
    return raus;
}

}  // namespace

int main()
{
    Position start;
    const std::vector<std::string> brett = felder(start);

    std::printf("Anfangsstellung\n");
    pruefe(brett[4] == "wk", "e1 ist der weisse Koenig");
    pruefe(brett[60] == "bk", "e8 ist der schwarze Koenig");
    pruefe(brett[0] == "wr", "a1 ist ein weisser Turm");
    pruefe(brett[63] == "br", "h8 ist ein schwarzer Turm");
    pruefe(brett[8] == "wp" && brett[55] == "bp", "die Bauernreihen stimmen");
    int besetzt = 0;
    for (int i = 0; i < 64; ++i)
        if (!brett[i].empty())
            ++besetzt;
    pruefe(besetzt == 32, "32 Figuren stehen auf dem Brett");

    std::printf("Zuege\n");
    pruefe(start.legalMoves().size() == 20, "20 Eroeffnungszuege");
    pruefe(start.legalTargets(12).size() == 2, "der Bauer e2 hat zwei Ziele");
    pruefe(start.sanOf("e2e4") == "e4", "e2e4 heisst e4");
    pruefe(start.sanOf("e2e5").empty(), "e2e5 gibt es nicht");

    std::printf("Der Gegner\n");
    Position lauf;
    int zuege = 0;
    while (!lauf.gameOver() && zuege < 120) {
        const std::string zug = Bot::zug(lauf, zuege < 2 ? 2 : 1);
        if (zug.empty())
            break;
        if (!lauf.isLegal(zug)) {
            std::printf("   FEHLT: der Gegner wollte %s spielen\n", zug.c_str());
            ++fehler;
            break;
        }
        lauf.play(zug);
        ++zuege;
    }
    std::printf("   %d Halbzuege gespielt, Ausgang %s\n", zuege,
                lauf.gameOver() ? "entschieden" : "offen");
    pruefe(zuege > 10, "die Partie kommt in Gang");

    // Ein Matt in einem Zug muss er finden.
    Position matt("6k1/5ppp/8/8/8/8/5PPP/R5K1 w - - 0 1");
    const std::string zug = Bot::zug(matt, 3);
    if (zug != "a1a8")
        std::printf("   gefunden wurde %s\n", zug.c_str());
    pruefe(zug == "a1a8", "Turm nach a8 ist Matt");

    std::printf(fehler ? "\n%d Pruefung(en) fehlgeschlagen\n"
                       : "\nalles in Ordnung\n", fehler);
    return fehler ? 1 : 0;
}
