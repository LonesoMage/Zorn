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

Thread* Threads = nullptr;

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
    static Move counterMoves[64][64];
    static int reductions[64][64];

    static const Value FutilityMargins[5] = { 0, 200, 300, 500, 900 };

    void init()
    {
        Threads = new Thread(0);
        TT.resize(64);

        for (int depth = 1; depth < 64; ++depth)
            for (int moveCount = 1; moveCount < 64; ++moveCount)
                reductions[depth][moveCount] = int(0.75 + log(double(depth)) * log(double(moveCount)) / 2.25);

        memset(history, 0, sizeof(history));
        memset(killerMoves, 0, sizeof(killerMoves));
        memset(counterMoves, 0, sizeof(counterMoves));
    }

    static inline Value scoreMove(const Position& pos, Move move, Move ttMove, int ply, Move previousMove)
    {
        if (move == ttMove) return 30000;

        Square to = to_sq(move);
        Square from = from_sq(move);
        Piece captured = pos.piece_on(to);

        if (type_of(move) == PROMOTION)
        {
            PieceType promoteTo = promotion_type(move);
            Value promoBonus = promoteTo == QUEEN ? 25000 :
                promoteTo == ROOK ? 24000 :
                promoteTo == BISHOP ? 23000 : 22000;

            if (captured != NO_PIECE)
                promoBonus += Eval::PieceValues[type_of(captured)];

            return promoBonus;
        }

        if (type_of(move) == ENPASSANT)
        {
            return 21000;
        }

        if (captured != NO_PIECE)
        {
            Value see = Eval::PieceValues[type_of(captured)] - Eval::PieceValues[type_of(pos.piece_on(from))];
            return see >= 0 ? 20000 + see : 10000 + see;
        }

        if (type_of(move) == CASTLING)
            return 15000;

        if (ply < 128)
        {
            if (move == killerMoves[ply][0]) return 9000;
            if (move == killerMoves[ply][1]) return 8000;
        }

        if (previousMove != MOVE_NONE)
        {
            if (move == counterMoves[from_sq(previousMove)][to_sq(previousMove)])
                return 7000;
        }

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
        if (ply < 128 && move != killerMoves[ply][0])
        {
            killerMoves[ply][1] = killerMoves[ply][0];
            killerMoves[ply][0] = move;
        }
    }

    static inline void updateCounters(Move move, Move previousMove)
    {
        if (previousMove != MOVE_NONE)
            counterMoves[from_sq(previousMove)][to_sq(previousMove)] = move;
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
            Square to = to_sq(move);
            Piece captured = pos.piece_on(to);
            if (captured == NO_PIECE && type_of(move) != ENPASSANT) continue;

            Square from = from_sq(move);
            if (captured != NO_PIECE)
            {
                Value see = Eval::PieceValues[type_of(captured)] - Eval::PieceValues[type_of(pos.piece_on(from))];
                if (see < -50) continue;
            }

            StateInfo st;
            pos.do_move(move, st);
            Value score = -quiesce(pos, -beta, -alpha, ply + 1);
            pos.undo_move(move);

            if (score >= beta) return beta;
            if (score > alpha) alpha = score;
        }

        return alpha;
    }

    static Value search(Position& pos, Value alpha, Value beta, Depth depth, int ply, bool cutNode, Move previousMove, Move excludedMove);

    static bool isSingular(Position& pos, Move ttMove, Value ttValue, Depth depth, Value beta)
    {
        Value singularBeta = ttValue - depth * 2;
        Depth singularDepth = depth / 2;

        struct ScoredMove
        {
            Move move = MOVE_NONE;
            Value score = VALUE_ZERO;
        };
        ScoredMove moves[256];
        int moveCount = 0;

        for (const auto& move : MoveList(pos))
        {
            if (move != ttMove)
            {
                moves[moveCount].move = move;
                moves[moveCount].score = scoreMove(pos, move, MOVE_NONE, 0, MOVE_NONE);
                moveCount++;
            }
        }

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

            StateInfo st;
            pos.do_move(move, st);
            Value score = -search(pos, -singularBeta - 1, -singularBeta, singularDepth, 0, true, MOVE_NONE, move);
            pos.undo_move(move);

            if (score > singularBeta)
                return false;
        }

        return true;
    }

    static Value search(Position& pos, Value alpha, Value beta, Depth depth, int ply, bool cutNode, Move previousMove, Move excludedMove)
    {
        if (timeUp()) return VALUE_ZERO;
        if (depth <= 0) return quiesce(pos, alpha, beta, ply);

        ++searchInfo.nodeCount;

        bool isPv = (beta - alpha) > 1;
        bool inCheck = pos.checkers() != 0;

        bool found;
        TTEntry* tte = TT.probe(pos.key(), found);
        Move ttMove = found ? tte->move() : MOVE_NONE;

        if (excludedMove != MOVE_NONE && ttMove == excludedMove)
            ttMove = MOVE_NONE;

        if (found && tte->depth() >= depth && !isPv && excludedMove == MOVE_NONE)
        {
            Value ttValue = tte->value();
            if (tte->bound() == BOUND_EXACT) return ttValue;
            if (tte->bound() == BOUND_LOWER && ttValue >= beta) return ttValue;
            if (tte->bound() == BOUND_UPPER && ttValue <= alpha) return ttValue;
        }

        Value staticEval = inCheck ? VALUE_NONE : Eval::evaluate(pos);

        if (!isPv && !inCheck && depth >= 3 && excludedMove == MOVE_NONE)
        {
            int R = 3 + depth / 4;
            StateInfo st;
            pos.do_null_move(st);
            Value nullValue = -search(pos, -beta, -beta + 1, depth - R - 1, ply + 1, !cutNode, MOVE_NONE, MOVE_NONE);
            pos.undo_null_move();

            if (nullValue >= beta)
                return nullValue >= VALUE_MATE - 100 ? beta : nullValue;
        }

        if (!isPv && !inCheck && depth <= 4 && excludedMove == MOVE_NONE)
        {
            Value futilityValue = staticEval + FutilityMargins[depth];
            if (futilityValue <= alpha)
                return futilityValue;
        }

        if (!isPv && !inCheck && depth <= 8 && staticEval - 85 * depth >= beta && excludedMove == MOVE_NONE)
            return staticEval;

        Value bestValue = -VALUE_INFINITE;
        Move bestMove = MOVE_NONE;
        Value originalAlpha = alpha;
        int moveCount = 0;
        int quietsSearched = 0;

        struct ScoredMove
        {
            Move move = MOVE_NONE;
            Value score = VALUE_ZERO;
        };
        ScoredMove moves[256];

        for (const auto& move : MoveList(pos))
        {
            if (move != excludedMove)
            {
                moves[moveCount].move = move;
                moves[moveCount].score = scoreMove(pos, move, ttMove, ply, previousMove);
                moveCount++;
            }
        }

        if (moveCount == 0)
        {
            if (excludedMove != MOVE_NONE)
                return alpha;
            return inCheck ? VALUE_MATE + ply : VALUE_DRAW;
        }

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

            bool isCapture = pos.piece_on(to_sq(move)) != NO_PIECE || type_of(move) == ENPASSANT;
            bool isPromotion = type_of(move) == PROMOTION;
            bool isQuiet = !isCapture && !isPromotion;
            bool isKiller = ply < 128 && (move == killerMoves[ply][0] || move == killerMoves[ply][1]);
            bool givesCheck = pos.gives_check(move);

            if (!isPv && !inCheck && isQuiet && bestValue > -VALUE_MATE && depth <= 4)
            {
                Value futilityValue = staticEval + FutilityMargins[depth] + 150;
                if (futilityValue <= alpha && !givesCheck)
                    continue;
            }

            if (!isPv && !inCheck && isQuiet && quietsSearched >= depth * depth)
                continue;

            int extension = 0;

            if (move == ttMove && depth >= 6 && tte->depth() >= depth - 3 && !inCheck &&
                tte->bound() == BOUND_LOWER && abs(tte->value()) < VALUE_MATE - 100 && excludedMove == MOVE_NONE)
            {
                if (isSingular(pos, ttMove, tte->value(), depth, beta))
                    extension = 1;
            }

            if (inCheck && depth <= 8)
                extension = 1;

            StateInfo st;
            pos.do_move(move, st);

            Value value;
            Depth newDepth = depth + extension - 1;

            if (i == 0)
            {
                value = -search(pos, -beta, -alpha, newDepth, ply + 1, false, move, MOVE_NONE);
            }
            else
            {
                int reduction = 0;

                if (depth >= 3 && i >= 3 && isQuiet && !isKiller && !givesCheck && extension == 0)
                {
                    int maxDepth = depth > 63 ? 63 : depth;
                    int maxMove = i > 63 ? 63 : i;
                    reduction = reductions[maxDepth][maxMove];

                    if (isPv) reduction--;
                    if (cutNode) reduction++;
                    if (move == ttMove) reduction--;

                    reduction = reduction > 0 ? reduction : 0;
                    int maxReduction = newDepth - 1;
                    reduction = reduction < maxReduction ? reduction : maxReduction;
                }

                value = -search(pos, -alpha - 1, -alpha, newDepth - reduction, ply + 1, true, move, MOVE_NONE);

                if (value > alpha && reduction > 0)
                    value = -search(pos, -alpha - 1, -alpha, newDepth, ply + 1, true, move, MOVE_NONE);

                if (value > alpha && value < beta && isPv)
                    value = -search(pos, -beta, -alpha, newDepth, ply + 1, false, move, MOVE_NONE);
            }

            pos.undo_move(move);

            if (isQuiet)
                quietsSearched++;

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
                            updateCounters(move, previousMove);
                        }
                        break;
                    }
                }
            }

            if (isQuiet)
                updateHistory(pos.side_to_move(), from_sq(move), to_sq(move), depth, false);
        }

        if (excludedMove == MOVE_NONE)
        {
            Bound bound = bestValue >= beta ? BOUND_LOWER :
                bestValue > originalAlpha ? BOUND_EXACT : BOUND_UPPER;

            tte->save(pos.key(), bestValue, isPv, bound, depth, bestMove, staticEval);
        }

        return bestValue;
    }

    static Value aspirationSearch(Position& pos, Value prevScore, Depth depth)
    {
        Value delta = Value(16);
        Value alpha = std::max(Value(prevScore - delta), Value(-VALUE_INFINITE));
        Value beta = std::min(Value(prevScore + delta), Value(VALUE_INFINITE));

        int failHighCount = 0;
        int failLowCount = 0;

        while (true)
        {
            Value score = search(pos, alpha, beta, depth, 0, false, MOVE_NONE, MOVE_NONE);

            if (score <= alpha)
            {
                beta = (alpha + beta) / 2;
                alpha = std::max(Value(score - delta), Value(-VALUE_INFINITE));
                failLowCount++;
            }
            else if (score >= beta)
            {
                beta = std::min(Value(score + delta), Value(VALUE_INFINITE));
                failHighCount++;
            }
            else
            {
                return score;
            }

            delta += delta / 4 + 5;

            if (failHighCount + failLowCount > 4)
            {
                alpha = -VALUE_INFINITE;
                beta = VALUE_INFINITE;
            }
        }
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

            if (depth == 1)
                bestValue = search(pos, -VALUE_INFINITE, VALUE_INFINITE, depth, 0, false, MOVE_NONE, MOVE_NONE);
            else
                bestValue = aspirationSearch(pos, bestValue, depth);

            bool found;
            TTEntry* tte = TT.probe(pos.key(), found);
            if (found) bestMove = tte->move();

            auto elapsed = int(chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - searchInfo.startTime).count());
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