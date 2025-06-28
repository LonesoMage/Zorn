#include "search.h"
#include "board.h"
#include "move.h"
#include "uci.h"
#include "eval.h"
#include "tt.h"
#include <iostream>
#include <algorithm>
#include <vector>
#include <chrono>
#include <cstring>
#include <cmath>

using namespace std;

Thread* Threads;

namespace Search
{
    struct SearchInfo
    {
        chrono::steady_clock::time_point startTime;
        int timeLimit = 0;
        bool stopped = false;
        uint64_t nodeCount = 0;
    };

    static SearchInfo searchInfo;

    static int history[2][64][64];
    static Move killerMoves[128][2];
    static int reductions[64][64];

    void init()
    {
        Threads = new Thread(0);
        TT.resize(64);

        for (int depth = 1; depth < 64; ++depth)
            for (int moveCount = 1; moveCount < 64; ++moveCount)
                reductions[depth][moveCount] = int(0.75 + log(double(depth)) * log(double(moveCount)) / 2.25);

        memset(history, 0, sizeof(history));
        memset(killerMoves, 0, sizeof(killerMoves));
    }

    static inline Value scoreMove(const Position& pos, Move move, Move ttMove, int ply)
    {
        if (move == ttMove) return 30000;

        Square to = to_sq(move);
        Piece captured = pos.piece_on(to);

        if (captured != NO_PIECE)
        {
            Square from = from_sq(move);
            Value see = Eval::PieceValues[type_of(captured)] - Eval::PieceValues[type_of(pos.piece_on(from))];
            return see >= 0 ? 20000 + see : 10000 + see;
        }

        if (move == killerMoves[ply][0]) return 9000;
        if (move == killerMoves[ply][1]) return 8000;

        Square from = from_sq(move);
        return history[pos.side_to_move()][from][to];
    }

    static inline bool timeUp()
    {
        return searchInfo.timeLimit > 0 &&
            chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - searchInfo.startTime).count() >= searchInfo.timeLimit;
    }

    static inline void updateHistory(Color us, Square from, Square to, Depth depth, bool failHigh)
    {
        int bonus = failHigh ? depth * depth : -depth * depth / 4;
        history[us][from][to] += bonus;
        if (abs(history[us][from][to]) > 16000)
            history[us][from][to] /= 2;
    }

    static inline void updateKillers(Move move, int ply)
    {
        if (move != killerMoves[ply][0])
        {
            killerMoves[ply][1] = killerMoves[ply][0];
            killerMoves[ply][0] = move;
        }
    }

    static Value quiesce(Position& pos, Value alpha, Value beta, int ply)
    {
        ++searchInfo.nodeCount;

        Value standPat = Eval::evaluate(pos);
        if (standPat >= beta) return beta;
        if (standPat > alpha) alpha = standPat;

        if (standPat + 200 < alpha) return alpha;

        for (const auto& move : MoveList(pos))
        {
            if (!pos.legal(move)) continue;

            Square to = to_sq(move);
            Piece captured = pos.piece_on(to);
            if (captured == NO_PIECE) continue;

            Square from = from_sq(move);
            Value see = Eval::PieceValues[type_of(captured)] - Eval::PieceValues[type_of(pos.piece_on(from))];
            if (see < -50) continue;

            StateInfo st;
            pos.do_move(move, st);
            Value score = -quiesce(pos, -beta, -alpha, ply + 1);
            pos.undo_move(move);

            if (score >= beta) return beta;
            if (score > alpha) alpha = score;
        }

        return alpha;
    }

    static Value search(Position& pos, Value alpha, Value beta, Depth depth, int ply, bool cutNode)
    {
        if (timeUp()) return VALUE_ZERO;
        if (depth <= 0) return quiesce(pos, alpha, beta, ply);

        ++searchInfo.nodeCount;

        bool isPv = (beta - alpha) > 1;

        bool found;
        TTEntry* tte = TT.probe(pos.key(), found);
        Move ttMove = found ? tte->move() : MOVE_NONE;

        if (found && tte->depth() >= depth && !isPv)
        {
            Value ttValue = tte->value();
            if (tte->bound() == BOUND_EXACT) return ttValue;
            if (tte->bound() == BOUND_LOWER && ttValue >= beta) return ttValue;
            if (tte->bound() == BOUND_UPPER && ttValue <= alpha) return ttValue;
        }

        if (!isPv && depth >= 3)
        {
            int R = 3 + depth / 4;
            StateInfo st;
            pos.do_null_move(st);
            Value nullValue = -search(pos, -beta, -beta + 1, depth - R - 1, ply + 1, !cutNode);
            pos.undo_null_move();

            if (nullValue >= beta)
                return nullValue >= VALUE_MATE - 100 ? beta : nullValue;
        }

        Value bestValue = -VALUE_INFINITE;
        Move bestMove = MOVE_NONE;
        Value originalAlpha = alpha;

        struct ScoredMove { Move move; Value score; };
        ScoredMove moves[256];
        int moveCount = 0;

        for (const auto& move : MoveList(pos))
        {
            if (pos.legal(move))
            {
                moves[moveCount].move = move;
                moves[moveCount].score = scoreMove(pos, move, ttMove, ply);
                moveCount++;
            }
        }

        if (moveCount == 0) return VALUE_DRAW;

        for (int i = 0; i < moveCount - 1; i++)
        {
            for (int j = i + 1; j < moveCount; j++)
            {
                if (moves[j].score > moves[i].score)
                {
                    ScoredMove temp = moves[i];
                    moves[i] = moves[j];
                    moves[j] = temp;
                }
            }
        }

        for (int i = 0; i < moveCount; i++)
        {
            Move move = moves[i].move;

            bool isCapture = pos.piece_on(to_sq(move)) != NO_PIECE;
            bool isQuiet = !isCapture;

            StateInfo st;
            pos.do_move(move, st);

            Value value;

            if (i == 0)
            {
                value = -search(pos, -beta, -alpha, depth - 1, ply + 1, false);
            }
            else
            {
                int reduction = 0;

                if (depth >= 3 && i >= 3 && isQuiet)
                {
                    int maxDepth = depth > 63 ? 63 : depth;
                    int maxMove = i > 63 ? 63 : i;
                    reduction = reductions[maxDepth][maxMove];

                    if (isPv) reduction--;
                    if (cutNode) reduction++;

                    reduction = reduction > 0 ? reduction : 0;
                    int maxReduction = depth - 2;
                    reduction = reduction < maxReduction ? reduction : maxReduction;
                }

                value = -search(pos, -alpha - 1, -alpha, depth - 1 - reduction, ply + 1, true);

                if (value > alpha && reduction > 0)
                    value = -search(pos, -alpha - 1, -alpha, depth - 1, ply + 1, true);

                if (value > alpha && value < beta && isPv)
                    value = -search(pos, -beta, -alpha, depth - 1, ply + 1, false);
            }

            pos.undo_move(move);

            if (value > bestValue)
            {
                bestValue = value;
                bestMove = move;

                if (value > alpha)
                {
                    alpha = value;

                    if (value >= beta)
                    {
                        if (isQuiet)
                        {
                            updateHistory(pos.side_to_move(), from_sq(move), to_sq(move), depth, true);
                            updateKillers(move, ply);
                        }
                        break;
                    }
                }
            }

            if (isQuiet)
                updateHistory(pos.side_to_move(), from_sq(move), to_sq(move), depth, false);
        }

        Bound bound = bestValue >= beta ? BOUND_LOWER :
            bestValue > originalAlpha ? BOUND_EXACT : BOUND_UPPER;

        tte->save(pos.key(), bestValue, isPv, bound, depth, bestMove, Eval::evaluate(pos));

        return bestValue;
    }

    void start(Position& pos, const Limits& limits)
    {
        searchInfo.startTime = chrono::steady_clock::now();
        searchInfo.timeLimit = limits.movetime;
        searchInfo.stopped = false;
        searchInfo.nodeCount = 0;

        TT.new_search();
        memset(killerMoves, 0, sizeof(killerMoves));

        Value bestValue = -VALUE_INFINITE;
        Move bestMove = MOVE_NONE;

        int maxDepth = limits.depth < 50 ? limits.depth : 50;
        if (limits.movetime > 0) maxDepth = 50;

        for (Depth depth = 1; depth <= maxDepth; ++depth)
        {
            if (timeUp()) break;

            bestValue = search(pos, -VALUE_INFINITE, VALUE_INFINITE, depth, 0, false);

            bool found;
            TTEntry* tte = TT.probe(pos.key(), found);
            if (found) bestMove = tte->move();

            auto elapsed = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - searchInfo.startTime).count();
            Threads->nodes = searchInfo.nodeCount;

            cout << "info depth " << depth
                << " score " << UCI::value(bestValue)
                << " nodes " << searchInfo.nodeCount
                << " time " << elapsed
                << " nps " << (elapsed > 0 ? (searchInfo.nodeCount * 1000) / elapsed : 0)
                << " pv " << UCI::move(bestMove, pos.is_chess960())
                << endl;

            if (timeUp()) break;
        }

        if (bestMove != MOVE_NONE)
            cout << "bestmove " << UCI::move(bestMove, pos.is_chess960()) << endl;
        else
            cout << "bestmove (none)" << endl;
    }
}