/*
    Copyright (C) 2026 smatkovi

    This file is part of harbour-schachlehrer.

    harbour-schachlehrer is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    harbour-schachlehrer is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with harbour-schachlehrer. If not, see <https://www.gnu.org/licenses/>.

    SPDX-License-Identifier: GPL-3.0-or-later
*/
#ifndef SCHACH_CORE_POSITION_H
#define SCHACH_CORE_POSITION_H

// A thin shell around the vendored chess-library (docs/design.md §2): FEN,
// legal moves, SAN, PGN output, material balance, game phase — plus the two
// board primitives the diagnosis needs and the library does not have: a
// static exchange evaluation and "what hangs here?" (teacher.md §2.2).
//
// Qt-free and free of I/O, like the Tarock core and for the same reason: it
// can be checked against fixed inputs.

// Qt's moc cannot parse this header (it stops at a nested namespace in
// chess.hpp), and it does not need to: nothing in the Qt classes that include
// Position.h exposes a chess-library type in a signal, slot or property.
#ifndef Q_MOC_RUN
#include <chess.hpp>
#endif

#include <cstdint>
#include <string>
#include <vector>

namespace schach {
namespace core {

// teacher.md §0.4 — mechanical, so that it is reproducible.
enum class Phase : std::uint8_t { Opening = 0, Middlegame = 1, Endgame = 2 };

enum class Outcome : std::uint8_t { Ongoing = 0, WhiteWins, BlackWins, Draw };

// Why the game ended; the sentence in the UI is built from this, never from
// a bare "0-1".
enum class EndReason : std::uint8_t {
    None = 0,
    Checkmate,
    Stalemate,
    InsufficientMaterial,
    FiftyMoves,
    Repetition,
    Resignation,
    Agreement
};

// One square that the opponent can win material on (teacher.md §2.2:
// "hängt(f)" is SEE > 0, never "attacked and undefended").
struct Hanging {
    int square = -1;      // 0 = a1 … 63 = h8
    int value = 0;        // piece value in pawn units
    char piece = ' ';     // 'N', 'b', … as in FEN
    int see = 0;          // what the opponent wins there, in pawn units

    bool operator==(const Hanging& other) const { return square == other.square; }
};

// The seven-tag roster plus what we actually know (platform.md §5.2).
struct PgnTags {
    std::string event = "Übungspartie";
    std::string site = "harbour-schachlehrer";
    std::string date;    // YYYY.MM.DD, "????.??.??" when unknown
    std::string round = "-";
    std::string white = "Weiß";
    std::string black = "Schwarz";
    std::string result = "*";
    std::string eco;
    std::string opening;
    std::string timeControl;
};

class Position
{
public:
    Position();
    explicit Position(const std::string& fen);

    // --- state -------------------------------------------------------------
    bool setFen(const std::string& fen);   // also resets the move history
    std::string fen() const;
    // The first four FEN fields. platform.md §5.2: this is the key for
    // position identity, repetition and puzzle identity; the full FEN is only
    // stored, never indexed.
    std::string epd() const;
    void reset();                          // back to the start position

    bool whiteToMove() const;
    int ply() const;                       // half moves played since the start of this game
    int fullMoveNumber() const;
    bool inCheck() const;

    const chess::Board& board() const { return m_board; }

    // --- moves -------------------------------------------------------------
    std::vector<std::string> legalMoves() const;             // UCI strings
    std::vector<int> legalTargets(int fromSquare) const;     // for the board UI
    bool isLegal(const std::string& uciMove) const;
    // Promotion moves without a piece letter are rejected; the caller must
    // ask the player, which is what TeacherEngine::play() does.
    bool needsPromotion(int fromSquare, int toSquare) const;

    bool play(const std::string& uciMove);   // appends to the history
    bool playSan(const std::string& san);
    bool undo();                             // takeBack(), see teacher.md §7.6

    std::string sanOf(const std::string& uciMove) const;   // empty when illegal
    std::string uciOf(const std::string& san) const;       // empty when unparsable
    // SAN of a move played in some other position, for sentences about lines.
    static std::string sanIn(const std::string& fen, const std::string& uciMove);

    // --- history and PGN ---------------------------------------------------
    const std::vector<std::string>& history() const { return m_history; }   // UCI
    const std::vector<std::string>& sanHistory() const { return m_sanHistory; }
    const std::string& startFen() const { return m_startFen; }
    std::string lastMove() const;            // UCI, empty at the start
    std::string lastMoveSan() const;

    // ~50 lines: the library reads PGN but does not write it.
    // Evaluations are emitted in the Lichess-compatible {[%eval 1.23]} form so
    // that our analyses can travel back (platform.md §5.2).
    std::string toPgn(const PgnTags& tags,
                      const std::vector<std::string>& comments = std::vector<std::string>()) const;

    // --- verdicts ----------------------------------------------------------
    Outcome outcome() const;
    EndReason endReason() const;
    bool gameOver() const { return outcome() != Outcome::Ongoing; }

    // --- material and phase ------------------------------------------------
    static int pieceValue(chess::PieceType type);            // pawn units, king = 0
    int materialBalance() const;      // pawn units, positive = White is up
    int materialFor(bool white) const;
    int officers() const;             // non-pawns of both colours (teacher.md §0.4)
    int heavyMaterial() const;        // sum of piece values without kings and pawns
    Phase phase() const;
    int pieceCount() const;           // for the tablebase question "<= 5 men?"

    // --- the two primitives the diagnosis needs ----------------------------
    // Static exchange evaluation of a capture (or of a quiet move: then it is
    // the value the moved piece loses on its destination square), in pawn
    // units. teacher.md §2.2: the swap-off sequence with the cheapest attacker
    // and max(0, …) at every node.
    int see(const std::string& uciMove) const;
    int seeOnSquare(int square, bool attackerIsWhite) const;

    // Own pieces of `colourWhite` on which the opponent has a capture with
    // SEE > 0, sorted by piece value descending.
    std::vector<Hanging> hanging(bool colourWhite) const;

    // Squares from which `byWhite` attacks `square`.
    std::vector<int> attackersOf(int square, bool byWhite) const;
    int attackerCount(int square, bool byWhite) const;

    // Does the side to move have a capture worth at least `minSee` pawns?
    // (teacher.md C2: "a free piece was standing there".)
    std::string bestFreeCapture(int minSee, int* seeOut = nullptr) const;

    // All checks and captures of the side to move — the eleventh of all moves
    // that refutes three quarters of all blunders (teacher.md §1.2.2, §7.5).
    std::vector<std::string> checksAndCaptures() const;

    // --- helpers for the motif rules (teacher.md §2.5) ----------------------
    bool isCapture(const std::string& uciMove) const;
    bool givesCheck(const std::string& uciMove) const;
    bool isPromotion(const std::string& uciMove) const;
    // A quiet move in the Lichess sense: neither check nor capture, and it
    // does not create an immediate capture threat.
    bool isQuiet(const std::string& uciMove) const;
    char pieceAt(int square) const;     // FEN letter, ' ' when empty
    int mobilityOf(int square) const;   // legal moves of the piece on `square`
    int kingSquare(bool white) const;

    // A position reached after playing `uciMove`, without touching this one.
    Position after(const std::string& uciMove) const;
    // The null move of teacher.md §0.4: what does the opponent threaten?
    Position afterNullMove() const;

private:
    void rebuildFromHistory();

    chess::Board m_board;
    std::string m_startFen;
    std::vector<std::string> m_history;
    std::vector<std::string> m_sanHistory;
};

// Square helpers shared by the whole core; 0 = a1, 63 = h8 as in the library.
int squareFromName(const std::string& name);   // -1 when unparsable
std::string squareName(int square);
inline int fileOf(int square) { return square & 7; }
inline int rankOf(int square) { return square >> 3; }

} // namespace core
} // namespace schach

#endif // SCHACH_CORE_POSITION_H
