#include "search_utils.h"
#include "search.h"
#include "board.h"
#include "eval.h"
#include "tt.h"
#include <cstring>
#include <cmath>
#include <chrono>

using namespace std::chrono;

namespace Search
{
    static SearchInfo searchInfo;
    static int history[2][64][64];
    static Move killerMoves[64][2];
    static int reductions[64][64];

    void initTables()
    {
        memset(history, 0, sizeof(history));
        memset(killerMoves, 0, sizeof(killerMoves));

        for (int depth = 1; depth < 64; ++depth)
            for (int moveCount = 1; moveCount < 64; ++moveCount)
                reductions[depth][moveCount] = int(0.75 + log(double(depth)) * log(double(moveCount)) / 2.25);
    }

    SearchInfo& getSearchInfo()
    {
        return searchInfo;
    }

    void initSearch(const Limits& limits, const Position& pos)
    {
        searchInfo.startTime = steady_clock::now();
        searchInfo.stopped = false;
        searchInfo.nodeCount = 0;

        if (limits.movetime > 0)
        {
            searchInfo.timeLimit = limits.movetime;
            searchInfo.maxNodes = 100000000;
        }
        else if (limits.time[pos.side_to_move()] > 0)
        {
            int timeLeft = limits.time[pos.side_to_move()];
            searchInfo.timeLimit = timeLeft / 30;
            searchInfo.maxNodes = 50000000;
        }
        else
        {
            searchInfo.timeLimit = 60000;
            searchInfo.maxNodes = 20000000;
        }
    }

    Value scoreMove(const Position& pos, Move move, Move ttMove, int ply)
    {
        if (move == ttMove) return 30000;

        Square from = from_sq(move);
        Square to = to_sq(move);
        Piece moved = pos.piece_on(from);
        Piece captured = pos.piece_on(to);

        if (type_of(move) == PROMOTION)
        {
            PieceType promoteTo = promotion_type(move);
            Value promoValue = promoteTo == QUEEN ? 25000 :
                promoteTo == ROOK ? 24000 :
                promoteTo == BISHOP ? 23000 : 22000;
            if (captured != NO_PIECE)
                promoValue += Eval::PieceValues[type_of(captured)] * 10;
            return promoValue;
        }

        if (captured != NO_PIECE || type_of(move) == ENPASSANT)
        {
            PieceType capturedType = type_of(move) == ENPASSANT ? PAWN : type_of(captured);
            Value captureValue = Eval::PieceValues[capturedType] * 10 - Eval::PieceValues[type_of(moved)];
            return 20000 + captureValue;
        }

        if (type_of(move) == CASTLING)
            return 15000;

        if (ply < 64)
        {
            if (move == killerMoves[ply][0]) return 9000;
            if (move == killerMoves[ply][1]) return 8000;
        }

        return history[pos.side_to_move()][from][to];
    }

    bool timeUp()
    {
        if (searchInfo.nodeCount > searchInfo.maxNodes) return true;
        if (searchInfo.timeLimit <= 0) return false;
        return duration_cast<milliseconds>(steady_clock::now() - searchInfo.startTime).count() >= searchInfo.timeLimit;
    }

    void updateKillers(Move move, int ply)
    {
        if (ply < 64 && move != killerMoves[ply][0])
        {
            killerMoves[ply][1] = killerMoves[ply][0];
            killerMoves[ply][0] = move;
        }
    }

    void updateHistory(Color us, Square from, Square to, int depth, bool cutoff)
    {
        int bonus = cutoff ? depth * depth : -depth * depth / 4;
        history[us][from][to] += bonus;

        if (history[us][from][to] > 16000)
            history[us][from][to] = 16000;
        else if (history[us][from][to] < -16000)
            history[us][from][to] = -16000;
    }

    void clearKillers()
    {
        memset(killerMoves, 0, sizeof(killerMoves));
    }

    Move getKillerMove(int ply, int index)
    {
        if (ply < 64 && index < 2)
            return killerMoves[ply][index];
        return MOVE_NONE;
    }

    int getReduction(int depth, int moveCount)
    {
        if (depth < 64 && moveCount < 64)
            return reductions[depth][moveCount];
        return 0;
    }
}