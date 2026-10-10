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
#include "Position.h"

#include <algorithm>
#include <sstream>

namespace schach {
namespace core {

namespace {

using chess::Bitboard;
using chess::Board;
using chess::Color;
using chess::Move;
using chess::Movelist;
using chess::PieceType;
using chess::Square;

// Pawn units. The king is worth nothing here on purpose: SEE must never
// "win" a king, and the phase rule of teacher.md §0.4 counts without kings.
constexpr int kValues[7] = { 1, 3, 3, 5, 9, 0, 0 };

int valueOf(PieceType type)
{
    const int index = static_cast<int>(type);
    return (index >= 0 && index < 7) ? kValues[index] : 0;
}

char fenChar(chess::Piece piece)
{
    if (piece == chess::Piece::NONE)
        return ' ';
    static const char letters[] = "pnbrqk";
    const char c = letters[static_cast<int>(piece.type())];
    return piece.color() == Color::WHITE ? static_cast<char>(c - 'a' + 'A') : c;
}

// attacks::attackers() always uses the board occupancy; SEE needs to peel
// pieces off the square one by one so that x-rays behind them come into play,
// so it needs its own version with an explicit occupancy.
Bitboard attackersWithOcc(const Board& board, Square square, Bitboard occ)
{
    Bitboard atks;
    atks |= chess::attacks::pawn(Color::BLACK, square) & board.pieces(PieceType::PAWN, Color::WHITE);
    atks |= chess::attacks::pawn(Color::WHITE, square) & board.pieces(PieceType::PAWN, Color::BLACK);
    atks |= chess::attacks::knight(square) & board.pieces(PieceType::KNIGHT);
    atks |= chess::attacks::bishop(square, occ)
            & (board.pieces(PieceType::BISHOP) | board.pieces(PieceType::QUEEN));
    atks |= chess::attacks::rook(square, occ)
            & (board.pieces(PieceType::ROOK) | board.pieces(PieceType::QUEEN));
    atks |= chess::attacks::king(square) & board.pieces(PieceType::KING);
    return atks & occ;
}

// The cheapest attacker of `side` in `set`; clears its bit from `occ`.
Bitboard leastValuable(const Board& board, Bitboard set, Color side, Bitboard& occ, int& valueOut)
{
    static const PieceType::underlying order[6] = { PieceType::PAWN,   PieceType::KNIGHT,
                                                    PieceType::BISHOP, PieceType::ROOK,
                                                    PieceType::QUEEN,  PieceType::KING };
    for (PieceType::underlying type : order) {
        const Bitboard candidates = set & board.pieces(PieceType(type), side);
        if (candidates.empty())
            continue;
        const Bitboard one = Bitboard::fromSquare(candidates.lsb());
        occ ^= one;
        valueOut = valueOf(PieceType(type));
        return one;
    }
    valueOut = 0;
    return Bitboard();
}

} // namespace

int squareFromName(const std::string& name)
{
    if (name.size() < 2)
        return -1;
    const int file = name[0] - 'a';
    const int rank = name[1] - '1';
    if (file < 0 || file > 7 || rank < 0 || rank > 7)
        return -1;
    return rank * 8 + file;
}

std::string squareName(int square)
{
    if (square < 0 || square > 63)
        return std::string("-");
    std::string out;
    out += static_cast<char>('a' + fileOf(square));
    out += static_cast<char>('1' + rankOf(square));
    return out;
}

// ---------------------------------------------------------------------------

Position::Position()
    : m_board(chess::constants::STARTPOS)
    , m_startFen(chess::constants::STARTPOS)
{
}

Position::Position(const std::string& fen)
    : m_board(chess::constants::STARTPOS)
    , m_startFen(chess::constants::STARTPOS)
{
    setFen(fen);
}

bool Position::setFen(const std::string& fen)
{
    Board probe(chess::constants::STARTPOS);
    if (!probe.setFen(fen))
        return false;
    m_board = probe;
    m_startFen = m_board.getFen();
    m_history.clear();
    m_sanHistory.clear();
    return true;
}

void Position::reset()
{
    setFen(std::string(chess::constants::STARTPOS));
}

std::string Position::fen() const { return m_board.getFen(); }
std::string Position::epd() const { return m_board.getFen(false); }
bool Position::whiteToMove() const { return m_board.sideToMove() == Color::WHITE; }
int Position::ply() const { return static_cast<int>(m_history.size()); }
int Position::fullMoveNumber() const { return static_cast<int>(m_board.fullMoveNumber()); }
bool Position::inCheck() const { return m_board.inCheck(); }

std::vector<std::string> Position::legalMoves() const
{
    Movelist moves;
    chess::movegen::legalmoves(moves, m_board);
    std::vector<std::string> out;
    out.reserve(moves.size());
    for (const Move& move : moves)
        out.push_back(chess::uci::moveToUci(move, m_board.chess960()));
    return out;
}

std::vector<int> Position::legalTargets(int fromSquare) const
{
    std::vector<int> out;
    if (fromSquare < 0 || fromSquare > 63)
        return out;
    Movelist moves;
    chess::movegen::legalmoves(moves, m_board);
    for (const Move& move : moves) {
        if (move.from().index() != fromSquare)
            continue;
        // Castling is generated as "king takes rook"; the board shows the
        // king's destination, so translate it back for the UI.
        int target = move.to().index();
        if (move.typeOf() == Move::CASTLING) {
            const bool kingSide = move.to() > move.from();
            target = chess::Square(kingSide ? chess::File::FILE_G : chess::File::FILE_C,
                                   move.from().rank())
                         .index();
        }
        if (std::find(out.begin(), out.end(), target) == out.end())
            out.push_back(target);
    }
    return out;
}

bool Position::isLegal(const std::string& uciMove) const
{
    if (uciMove.size() < 4)
        return false;
    const Move move = chess::uci::uciToMove(m_board, uciMove);
    if (move == Move::NO_MOVE)
        return false;
    Movelist moves;
    chess::movegen::legalmoves(moves, m_board);
    for (const Move& candidate : moves) {
        if (candidate == move)
            return true;
    }
    return false;
}

bool Position::needsPromotion(int fromSquare, int toSquare) const
{
    if (fromSquare < 0 || toSquare < 0)
        return false;
    const chess::Piece piece = m_board.at<chess::Piece>(Square(fromSquare));
    if (piece == chess::Piece::NONE || piece.type() != PieceType::PAWN)
        return false;
    const int rank = rankOf(toSquare);
    return (piece.color() == Color::WHITE && rank == 7) || (piece.color() == Color::BLACK && rank == 0);
}

bool Position::play(const std::string& uciMove)
{
    if (!isLegal(uciMove))
        return false;
    const Move move = chess::uci::uciToMove(m_board, uciMove);
    m_sanHistory.push_back(chess::uci::moveToSan(m_board, move));
    m_board.makeMove(move);
    m_history.push_back(chess::uci::moveToUci(move, m_board.chess960()));
    return true;
}

bool Position::playSan(const std::string& san)
{
    const std::string uciMove = uciOf(san);
    return !uciMove.empty() && play(uciMove);
}

bool Position::undo()
{
    if (m_history.empty())
        return false;
    m_history.pop_back();
    m_sanHistory.pop_back();
    rebuildFromHistory();
    return true;
}

void Position::rebuildFromHistory()
{
    // The library can unmake a move, but only with the move object and in
    // exact order; replaying from the start FEN is simpler, always correct,
    // and fast enough for a 100-ply game on a phone.
    const std::vector<std::string> moves = m_history;
    m_board.setFen(m_startFen);
    m_history.clear();
    m_sanHistory.clear();
    for (const std::string& move : moves)
        play(move);
}

std::string Position::sanOf(const std::string& uciMove) const
{
    if (!isLegal(uciMove))
        return std::string();
    return chess::uci::moveToSan(m_board, chess::uci::uciToMove(m_board, uciMove));
}

std::string Position::uciOf(const std::string& san) const
{
    if (san.empty())
        return std::string();
    try {
        const Move move = chess::uci::parseSan(m_board, san);
        if (move == Move::NO_MOVE)
            return std::string();
        return chess::uci::moveToUci(move, m_board.chess960());
    } catch (const std::exception&) {
        return std::string();
    }
}

std::string Position::sanIn(const std::string& fen, const std::string& uciMove)
{
    Position position(fen);
    return position.sanOf(uciMove);
}

std::string Position::lastMove() const
{
    return m_history.empty() ? std::string() : m_history.back();
}

std::string Position::lastMoveSan() const
{
    return m_sanHistory.empty() ? std::string() : m_sanHistory.back();
}

std::string Position::toPgn(const PgnTags& tags, const std::vector<std::string>& comments) const
{
    std::ostringstream out;
    const std::string date = tags.date.empty() ? std::string("????.??.??") : tags.date;

    out << "[Event \"" << tags.event << "\"]\n";
    out << "[Site \"" << tags.site << "\"]\n";
    out << "[Date \"" << date << "\"]\n";
    out << "[Round \"" << tags.round << "\"]\n";
    out << "[White \"" << tags.white << "\"]\n";
    out << "[Black \"" << tags.black << "\"]\n";
    out << "[Result \"" << tags.result << "\"]\n";
    if (!tags.eco.empty())
        out << "[ECO \"" << tags.eco << "\"]\n";
    if (!tags.opening.empty())
        out << "[Opening \"" << tags.opening << "\"]\n";
    if (!tags.timeControl.empty())
        out << "[TimeControl \"" << tags.timeControl << "\"]\n";
    if (m_startFen != chess::constants::STARTPOS) {
        out << "[SetUp \"1\"]\n";
        out << "[FEN \"" << m_startFen << "\"]\n";
    }
    out << "\n";

    // The move numbers have to start at the start FEN, not at 1.
    Board replay(m_startFen);
    std::string line;
    const int firstFull = static_cast<int>(replay.fullMoveNumber());
    bool blackStarts = replay.sideToMove() == Color::BLACK;

    auto append = [&out, &line](const std::string& token) {
        if (line.size() + token.size() + 1 > 79) {
            out << line << "\n";
            line.clear();
        }
        if (!line.empty())
            line += ' ';
        line += token;
    };

    for (std::size_t i = 0; i < m_sanHistory.size(); ++i) {
        const int fullMove = firstFull + static_cast<int>((i + (blackStarts ? 1 : 0)) / 2);
        const bool whiteMove = (i % 2 == 0) != blackStarts;
        if (whiteMove)
            append(std::to_string(fullMove) + ".");
        else if (i == 0)
            append(std::to_string(fullMove) + "...");
        append(m_sanHistory[i]);
        if (i < comments.size() && !comments[i].empty())
            append("{" + comments[i] + "}");
    }
    append(tags.result);
    if (!line.empty())
        out << line << "\n";
    return out.str();
}

Outcome Position::outcome() const
{
    const std::pair<chess::GameResultReason, chess::GameResult> result = m_board.isGameOver();
    if (result.second == chess::GameResult::NONE)
        return Outcome::Ongoing;
    if (result.second == chess::GameResult::DRAW)
        return Outcome::Draw;
    // LOSE is always "the side to move lost".
    return whiteToMove() ? Outcome::BlackWins : Outcome::WhiteWins;
}

EndReason Position::endReason() const
{
    switch (m_board.isGameOver().first) {
    case chess::GameResultReason::CHECKMATE:
        return EndReason::Checkmate;
    case chess::GameResultReason::STALEMATE:
        return EndReason::Stalemate;
    case chess::GameResultReason::INSUFFICIENT_MATERIAL:
        return EndReason::InsufficientMaterial;
    case chess::GameResultReason::FIFTY_MOVE_RULE:
        return EndReason::FiftyMoves;
    case chess::GameResultReason::THREEFOLD_REPETITION:
        return EndReason::Repetition;
    default:
        return EndReason::None;
    }
}

int Position::pieceValue(PieceType type) { return valueOf(type); }

int Position::materialFor(bool white) const
{
    const Color colour = white ? Color::WHITE : Color::BLACK;
    int sum = 0;
    for (int type = 0; type < 5; ++type)
        sum += kValues[type] * m_board.pieces(PieceType(static_cast<PieceType::underlying>(type)), colour).count();
    return sum;
}

int Position::materialBalance() const { return materialFor(true) - materialFor(false); }

int Position::officers() const
{
    int count = 0;
    for (int type = 1; type < 5; ++type)
        count += m_board.pieces(PieceType(static_cast<PieceType::underlying>(type))).count();
    return count;
}

int Position::heavyMaterial() const
{
    int sum = 0;
    for (int type = 1; type < 5; ++type)
        sum += kValues[type] * m_board.pieces(PieceType(static_cast<PieceType::underlying>(type))).count();
    return sum;
}

Phase Position::phase() const
{
    // teacher.md §0.4. The endgame test comes first: with 12 officers left the
    // heavy material is far above 13, so the two can never both be true.
    if (heavyMaterial() <= 13)
        return Phase::Endgame;
    if (ply() < 24 && officers() >= 12)
        return Phase::Opening;
    return Phase::Middlegame;
}

int Position::pieceCount() const { return m_board.occ().count(); }

// --- SEE --------------------------------------------------------------------

int Position::see(const std::string& uciMove) const
{
    if (uciMove.size() < 4)
        return 0;
    const Move move = chess::uci::uciToMove(m_board, uciMove);
    if (move == Move::NO_MOVE)
        return 0;

    const Square from = move.from();
    const Square to = move.to();
    const chess::Piece mover = m_board.at<chess::Piece>(from);
    if (mover == chess::Piece::NONE)
        return 0;

    int gain[32];
    int depth = 0;

    int captured = 0;
    if (move.typeOf() == Move::ENPASSANT) {
        captured = kValues[0];
    } else {
        const chess::Piece victim = m_board.at<chess::Piece>(to);
        captured = (victim == chess::Piece::NONE) ? 0 : valueOf(victim.type());
    }

    int movedValue = valueOf(mover.type());
    if (move.typeOf() == Move::PROMOTION) {
        const int promoted = valueOf(move.promotionType());
        captured += promoted - kValues[0];
        movedValue = promoted;
    }
    if (move.typeOf() == Move::CASTLING)
        return 0;   // the king never lands on an attacked square

    gain[0] = captured;

    Bitboard occ = m_board.occ();
    occ ^= Bitboard::fromSquare(from);
    if (move.typeOf() == Move::ENPASSANT)
        occ ^= Bitboard::fromSquare(to.ep_square());
    else
        occ &= ~Bitboard::fromSquare(to);

    Color side = ~mover.color();
    Bitboard attackers = attackersWithOcc(m_board, to, occ);

    while (true) {
        ++depth;
        if (depth >= 31)
            break;
        gain[depth] = movedValue - gain[depth - 1];
        int nextValue = 0;
        const Bitboard next = leastValuable(m_board, attackers, side, occ, nextValue);
        if (next.empty())
            break;
        attackers = attackersWithOcc(m_board, to, occ);
        movedValue = nextValue;
        side = ~side;
        // A king may only take last: if the square is still defended after it
        // would capture, the capture is illegal, so stop the sequence.
        if (nextValue == 0 && !(attackers & m_board.us(side)).empty()) {
            --depth;
            break;
        }
    }

    while (--depth > 0)
        gain[depth - 1] = -std::max(-gain[depth - 1], gain[depth]);
    return gain[0];
}

int Position::seeOnSquare(int square, bool attackerIsWhite) const
{
    if (square < 0 || square > 63)
        return 0;
    const chess::Piece victim = m_board.at<chess::Piece>(Square(square));
    if (victim == chess::Piece::NONE)
        return 0;
    if ((victim.color() == Color::WHITE) == attackerIsWhite)
        return 0;

    // Try every attacker and keep the best outcome; the swap algorithm itself
    // starts with the cheapest, but the *best* capture is what decides whether
    // a piece hangs.
    const Color side = attackerIsWhite ? Color::WHITE : Color::BLACK;
    Bitboard attackers = attackersWithOcc(m_board, Square(square), m_board.occ()) & m_board.us(side);
    int best = 0;
    bool any = false;
    while (!attackers.empty()) {
        const int from = attackers.pop();
        // SEE is defined for the side to move; evaluate on a board where it is.
        Board probe = m_board;
        if (probe.sideToMove() != side) {
            std::string fen = probe.getFen();
            const std::size_t space = fen.find(' ');
            if (space == std::string::npos || space + 1 >= fen.size())
                continue;
            fen[space + 1] = attackerIsWhite ? 'w' : 'b';
            // Dropping the en-passant square avoids an illegal FEN when the
            // side to move changes.
            Board swapped(chess::constants::STARTPOS);
            if (!swapped.setFen(fen))
                continue;
            probe = swapped;
        }
        Position helper;
        helper.m_board = probe;
        const std::string uciMove = squareName(from) + squareName(square);
        if (!helper.isLegal(uciMove))
            continue;
        const int value = helper.see(uciMove);
        if (!any || value > best) {
            best = value;
            any = true;
        }
    }
    return best;
}

std::vector<Hanging> Position::hanging(bool colourWhite) const
{
    std::vector<Hanging> out;
    const Color owner = colourWhite ? Color::WHITE : Color::BLACK;
    Bitboard men = m_board.us(owner);
    while (!men.empty()) {
        const int square = men.pop();
        const chess::Piece piece = m_board.at<chess::Piece>(Square(square));
        if (piece.type() == PieceType::KING)
            continue;
        const int value = seeOnSquare(square, !colourWhite);
        if (value > 0) {
            Hanging entry;
            entry.square = square;
            entry.value = valueOf(piece.type());
            entry.piece = fenChar(piece);
            entry.see = value;
            out.push_back(entry);
        }
    }
    std::sort(out.begin(), out.end(), [](const Hanging& a, const Hanging& b) {
        if (a.value != b.value)
            return a.value > b.value;
        return a.square < b.square;
    });
    return out;
}

std::vector<int> Position::attackersOf(int square, bool byWhite) const
{
    std::vector<int> out;
    if (square < 0 || square > 63)
        return out;
    Bitboard set = chess::attacks::attackers(m_board, byWhite ? Color::WHITE : Color::BLACK, Square(square));
    while (!set.empty())
        out.push_back(set.pop());
    return out;
}

int Position::attackerCount(int square, bool byWhite) const
{
    if (square < 0 || square > 63)
        return 0;
    return chess::attacks::attackers(m_board, byWhite ? Color::WHITE : Color::BLACK, Square(square)).count();
}

std::string Position::bestFreeCapture(int minSee, int* seeOut) const
{
    std::string best;
    int bestValue = 0;
    Movelist moves;
    chess::movegen::legalmoves<chess::movegen::MoveGenType::CAPTURE>(moves, m_board);
    for (const Move& move : moves) {
        const std::string uciMove = chess::uci::moveToUci(move, m_board.chess960());
        const int value = see(uciMove);
        if (value >= minSee && value > bestValue) {
            bestValue = value;
            best = uciMove;
        }
    }
    if (seeOut)
        *seeOut = bestValue;
    return best;
}

std::vector<std::string> Position::checksAndCaptures() const
{
    std::vector<std::string> out;
    Movelist moves;
    chess::movegen::legalmoves(moves, m_board);
    for (const Move& move : moves) {
        const std::string uciMove = chess::uci::moveToUci(move, m_board.chess960());
        if (m_board.isCapture(move) || givesCheck(uciMove))
            out.push_back(uciMove);
    }
    return out;
}

bool Position::isCapture(const std::string& uciMove) const
{
    if (!isLegal(uciMove))
        return false;
    return m_board.isCapture(chess::uci::uciToMove(m_board, uciMove));
}

bool Position::givesCheck(const std::string& uciMove) const
{
    if (uciMove.size() < 4)
        return false;
    const Move move = chess::uci::uciToMove(m_board, uciMove);
    if (move == Move::NO_MOVE)
        return false;
    Board probe = m_board;
    probe.makeMove(move);
    return probe.inCheck();
}

bool Position::isPromotion(const std::string& uciMove) const
{
    if (uciMove.size() < 5)
        return false;
    const Move move = chess::uci::uciToMove(m_board, uciMove);
    return move != Move::NO_MOVE && move.typeOf() == Move::PROMOTION;
}

bool Position::isQuiet(const std::string& uciMove) const
{
    if (!isLegal(uciMove))
        return false;
    if (isCapture(uciMove) || givesCheck(uciMove))
        return false;
    // Lichess' quietMove: it must not create an immediate capture threat
    // either, otherwise every developing move with a tactical point would
    // count as quiet (teacher.md §2.2).
    const Position next = after(uciMove);
    if (next.inCheck())
        return true;
    const Position passed = next.afterNullMove();
    int gain = 0;
    passed.bestFreeCapture(1, &gain);
    return gain <= 0;
}

char Position::pieceAt(int square) const
{
    if (square < 0 || square > 63)
        return ' ';
    return fenChar(m_board.at<chess::Piece>(Square(square)));
}

int Position::mobilityOf(int square) const
{
    if (square < 0 || square > 63)
        return 0;
    const chess::Piece piece = m_board.at<chess::Piece>(Square(square));
    if (piece == chess::Piece::NONE)
        return 0;
    const Position& source = *this;
    Position helper;
    const Position* use = &source;
    Position passed;
    if ((piece.color() == Color::WHITE) != whiteToMove()) {
        if (inCheck())
            return 0;   // the side to move is in check, a null move is not defined
        passed = afterNullMove();
        use = &passed;
    }
    (void)helper;
    int count = 0;
    Movelist moves;
    chess::movegen::legalmoves(moves, use->m_board);
    for (const Move& move : moves) {
        if (move.from().index() == square)
            ++count;
    }
    return count;
}

int Position::kingSquare(bool white) const
{
    return m_board.kingSq(white ? Color::WHITE : Color::BLACK).index();
}

Position Position::after(const std::string& uciMove) const
{
    Position next = *this;
    next.play(uciMove);
    return next;
}

Position Position::afterNullMove() const
{
    Position next = *this;
    if (next.m_board.inCheck())
        return next;   // undefined; the caller checks inCheck() first
    next.m_board.makeNullMove();
    // The history is no longer a legal line, so it is cut: a null-move
    // position is only ever asked for an evaluation, never for a PGN.
    next.m_startFen = next.m_board.getFen();
    next.m_history.clear();
    next.m_sanHistory.clear();
    return next;
}

} // namespace core
} // namespace schach
