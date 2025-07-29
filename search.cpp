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
        uint64_t maxNodes = 20000000;
    };

    static SearchInfo searchInfo;
    static int history[2][64][64];
    static Move killerMoves[64][2];
    static int reductions[64][64];

    void init()
    {
        Threads = new Thread(0);
        TT.resize(64);
        memset(history, 0, sizeof(history));
        memset(killerMoves, 0, sizeof(killerMoves));

        for (int depth = 1; depth < 64; ++depth)
            for (int moveCount = 1; moveCount < 64; ++moveCount)
                reductions[depth][moveCount] = int(0.75 + log(double(depth)) * log(double(moveCount)) / 2.25);
    }

    static inline Value scoreMove(const Position& pos, Move move, Move ttMove, int ply)
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

    static inline bool timeUp()
    {
        if (searchInfo.nodeCount > searchInfo.maxNodes) return true;
        if (searchInfo.timeLimit <= 0) return false;
        return chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - searchInfo.startTime).count() >= searchInfo.timeLimit;
    }

    static inline void updateKillers(Move move, int ply)
    {
        if (ply < 64 && move != killerMoves[ply][0])
        {
            killerMoves[ply][1] = killerMoves[ply][0];
            killerMoves[ply][0] = move;
        }
    }

    static inline void updateHistory(Color us, Square from, Square to, int depth, bool cutoff)
    {
        int bonus = cutoff ? depth * depth : -depth * depth / 4;
        history[us][from][to] += bonus;

        if (history[us][from][to] > 16000)
            history[us][from][to] = 16000;
        else if (history[us][from][to] < -16000)
            history[us][from][to] = -16000;
    }

    static Value quiesce(Position& pos, Value alpha, Value beta, int ply)
    {
        if (ply >= 100) return Eval::evaluate(pos);
        if (timeUp()) return VALUE_ZERO;

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

            if (!pos.legal(move)) continue;

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

    static Value search(Position& pos, Value alpha, Value beta, Depth depth, int ply, bool cutNode)
    {
        if (timeUp()) return VALUE_ZERO;
        if (depth <= 0) return quiesce(pos, alpha, beta, ply);
        if (ply >= 100) return Eval::evaluate(pos);

        ++searchInfo.nodeCount;

        if (searchInfo.nodeCount % 200000 == 0)
        {
            auto elapsed = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - searchInfo.startTime).count();
            if (elapsed > 60000) return Eval::evaluate(pos);
        }

        bool isPv = (beta - alpha) > 1;
        bool inCheck = pos.checkers() != 0;

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

        Value staticEval = inCheck ? VALUE_NONE : Eval::evaluate(pos);

        if (!isPv && !inCheck && depth >= 3)
        {
            int R = 3 + depth / 4;
            StateInfo st;
            pos.do_null_move(st);
            Value nullValue = -search(pos, -beta, -beta + 1, depth - R - 1, ply + 1, !cutNode);
            pos.undo_null_move();

            if (nullValue >= beta)
                return nullValue >= VALUE_MATE - 100 ? beta : nullValue;
        }

        if (!isPv && !inCheck && depth <= 3 && staticEval - 200 * depth >= beta)
            return staticEval;

        struct ScoredMove
        {
            Move move = MOVE_NONE;
            Value score = VALUE_ZERO;
        };

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

        if (moveCount == 0)
        {
            return inCheck ? -VALUE_MATE + ply : VALUE_DRAW;
        }

        for (int i = 0; i < moveCount - 1; i++)
        {
            int bestIdx = i;
            for (int j = i + 1; j < moveCount; j++)
            {
                if (moves[j].score > moves[bestIdx].score)
                    bestIdx = j;
            }
            if (bestIdx != i)
            {
                ScoredMove temp = moves[i];
                moves[i] = moves[bestIdx];
                moves[bestIdx] = temp;
            }
        }

        Value bestValue = -VALUE_INFINITE;
        Move bestMove = MOVE_NONE;
        Value originalAlpha = alpha;
        int movesSearched = 0;

        for (int i = 0; i < moveCount; i++)
        {
            if (movesSearched >= 64) break;

            Move move = moves[i].move;
            Square from = from_sq(move);
            Square to = to_sq(move);
            bool isCapture = pos.piece_on(to) != NO_PIECE || type_of(move) == ENPASSANT;
            bool isPromotion = type_of(move) == PROMOTION;
            bool isQuiet = !isCapture && !isPromotion;
            bool isKiller = ply < 64 && (move == killerMoves[ply][0] || move == killerMoves[ply][1]);
            bool givesCheck = pos.gives_check(move);

            if (!isPv && !inCheck && isQuiet && movesSearched >= 8 && depth <= 6)
            {
                Value futilityValue = staticEval + 150 + 200 * depth;
                if (futilityValue <= alpha && !givesCheck) continue;
            }

            if (!isPv && !inCheck && isQuiet && movesSearched >= depth * depth + 6)
                break;

            int extension = 0;
            if (inCheck) extension = 1;

            StateInfo st;
            pos.do_move(move, st);

            Value value;
            Depth newDepth = depth + extension - 1;

            if (i == 0)
            {
                value = -search(pos, -beta, -alpha, newDepth, ply + 1, false);
            }
            else
            {
                int reduction = 0;

                if (depth >= 3 && i >= 4 && isQuiet && !isKiller && !givesCheck)
                {
                    int depthIdx = depth < 64 ? depth : 63;
                    int moveIdx = i < 64 ? i : 63;
                    reduction = reductions[depthIdx][moveIdx];

                    if (isPv) reduction--;
                    if (cutNode) reduction++;
                    if (move == ttMove) reduction -= 2;

                    reduction = std::max(0, std::min(reduction, newDepth - 1));
                }

                value = -search(pos, -alpha - 1, -alpha, newDepth - reduction, ply + 1, true);

                if (value > alpha && reduction > 0)
                    value = -search(pos, -alpha - 1, -alpha, newDepth, ply + 1, true);

                if (value > alpha && value < beta && isPv)
                    value = -search(pos, -beta, -alpha, newDepth, ply + 1, false);
            }

            pos.undo_move(move);
            movesSearched++;

            if (timeUp()) return bestValue > -VALUE_INFINITE ? bestValue : VALUE_ZERO;

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
                            updateKillers(move, ply);
                            updateHistory(pos.side_to_move(), from, to, depth, true);
                        }
                        break;
                    }
                }
            }

            if (isQuiet)
                updateHistory(pos.side_to_move(), from, to, depth, false);
        }

        if (bestValue == -VALUE_INFINITE)
            bestValue = VALUE_DRAW;

        Bound bound = bestValue >= beta ? BOUND_LOWER :
            bestValue > originalAlpha ? BOUND_EXACT : BOUND_UPPER;

        tte->save(pos.key(), bestValue, isPv, bound, depth, bestMove, staticEval);

        return bestValue;
    }

    void start(Position& pos, const Limits& limits)
    {
        searchInfo.startTime = chrono::steady_clock::now();
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

        int maxDepth = limits.depth > 0 ? std::min(limits.depth, 20) : 12;

        TT.new_search();
        memset(killerMoves, 0, sizeof(killerMoves));

        Value bestValue = -VALUE_INFINITE;
        Move bestMove = MOVE_NONE;

        for (Depth depth = 1; depth <= maxDepth; ++depth)
        {
            if (timeUp()) break;

            auto depthStart = chrono::steady_clock::now();
            bestValue = search(pos, -VALUE_INFINITE, VALUE_INFINITE, depth, 0, false);
            auto depthEnd = chrono::steady_clock::now();

            if (timeUp()) break;

            bool found;
            TTEntry* tte = TT.probe(pos.key(), found);
            if (found && tte->move() != MOVE_NONE)
                bestMove = tte->move();

            auto elapsed = int(chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - searchInfo.startTime).count());
            auto depthTime = int(chrono::duration_cast<chrono::milliseconds>(depthEnd - depthStart).count());
            Threads->nodes = searchInfo.nodeCount;

            cout << "info depth " << depth
                << " score " << UCI::value(bestValue)
                << " nodes " << searchInfo.nodeCount
                << " time " << elapsed
                << " nps " << (elapsed > 0 ? (searchInfo.nodeCount * 1000) / elapsed : 0)
                << " pv " << UCI::move(bestMove, pos.is_chess960())
                << endl;

            if (abs(bestValue) >= VALUE_MATE - 100)
                break;

            if (depth >= 8 && depthTime > searchInfo.timeLimit / 2)
                break;
        }

        if (bestMove != MOVE_NONE)
            cout << "bestmove " << UCI::move(bestMove, pos.is_chess960()) << endl;
        else
        {
            for (const auto& move : MoveList(pos))
            {
                if (pos.legal(move))
                {
                    bestMove = move;
                    break;
                }
            }
            if (bestMove != MOVE_NONE)
                cout << "bestmove " << UCI::move(bestMove, pos.is_chess960()) << endl;
            else
                cout << "bestmove (none)" << endl;
        }
    }
}