#include "search.h"
#include "search_utils.h"
#include "board.h"
#include "move.h"
#include "uci.h"
#include "eval.h"
#include "tt.h"
#include <iostream>
#include <algorithm>
#include <vector>
#include <chrono>

using namespace std;
using namespace std::chrono;

Thread* Threads = nullptr;

namespace Search
{
    void init()
    {
        Threads = new Thread(0);
        TT.resize(64);
        initTables();
    }
    static Value quiesce(Position& pos, Value alpha, Value beta, int ply)
    {
        if (ply >= 100) return Eval::evaluate(pos);
        if (timeUp()) return VALUE_ZERO;

        ++getSearchInfo().nodeCount;

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

        ++getSearchInfo().nodeCount;

        if (getSearchInfo().nodeCount % 200000 == 0)
        {
            auto elapsed = duration_cast<milliseconds>(steady_clock::now() - getSearchInfo().startTime).count();
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
            bool isKiller = ply < 64 && (move == getKillerMove(ply, 0) || move == getKillerMove(ply, 1));
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
                    reduction = getReduction(depthIdx, moveIdx);

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
        initSearch(limits, pos);

        TT.new_search();
        clearKillers();

        Value bestValue = -VALUE_INFINITE;
        Move bestMove = MOVE_NONE;

        int maxDepth = limits.depth > 0 ? std::min(limits.depth, 20) : 15;

        for (Depth depth = 2; depth <= maxDepth; ++depth)
        {
            if (timeUp()) break;

            auto depthStart = steady_clock::now();
            bestValue = search(pos, -VALUE_INFINITE, VALUE_INFINITE, depth, 0, false);
            auto depthEnd = steady_clock::now();

            if (timeUp()) break;

            bool found;
            TTEntry* tte = TT.probe(pos.key(), found);
            if (found && tte->move() != MOVE_NONE)
                bestMove = tte->move();

            auto elapsed = int(duration_cast<milliseconds>(steady_clock::now() - getSearchInfo().startTime).count());
            auto depthTime = int(duration_cast<milliseconds>(depthEnd - depthStart).count());
            Threads->nodes = getSearchInfo().nodeCount;

            cout << "info depth " << depth
                << " score " << UCI::value(bestValue)
                << " nodes " << getSearchInfo().nodeCount
                << " time " << elapsed
                << " nps " << (elapsed > 0 ? (getSearchInfo().nodeCount * 1000) / elapsed : 0)
                << " pv " << UCI::move(bestMove, pos.is_chess960())
                << endl;

            if (abs(bestValue) >= VALUE_MATE - 100)
                break;

            if (depth >= 8 && depthTime > getSearchInfo().timeLimit / 2)
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