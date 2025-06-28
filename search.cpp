#include "search.h"
#include "board.h"
#include "move.h"
#include "uci.h"
#include "eval.h"
#include <iostream>
#include <algorithm>

using namespace std;

Thread* Threads;

namespace Search
{
    void init()
    {
        Threads = new Thread(0);
    }

    void start(Position& pos, const Limits& limits)
    {
        Threads->nodes = 0;
        Value bestValue = -VALUE_INFINITE;
        Move bestMove = MOVE_NONE;

        for (Depth depth = 1; depth <= limits.depth; ++depth)
        {
            bestValue = -VALUE_INFINITE;

            for (int from = SQ_A1; from <= SQ_H8; from++)
            {
                Piece pc = pos.piece_on(Square(from));
                if (pc == NO_PIECE || color_of(pc) != pos.side_to_move())
                    continue;

                for (int to = SQ_A1; to <= SQ_H8; to++)
                {
                    if (from == to)
                        continue;

                    Move move = make_move(Square(from), Square(to));
                    if (!pos.pseudo_legal(move) || !pos.legal(move))
                        continue;

                    StateInfo st;
                    pos.do_move(move, st);

                    Value value = -search(pos, -VALUE_INFINITE, VALUE_INFINITE, depth - 1);

                    pos.undo_move(move);

                    if (value > bestValue)
                    {
                        bestValue = value;
                        bestMove = move;
                    }
                }
            }

            cout << "info depth " << depth
                << " score " << UCI::value(bestValue)
                << " nodes " << Threads->nodes
                << " pv " << UCI::move(bestMove, pos.is_chess960())
                << endl;
        }

        cout << "bestmove " << UCI::move(bestMove, pos.is_chess960()) << endl;
    }

    Value search(Position& pos, Value alpha, Value beta, Depth depth, bool cutNode)
    {
        if (depth <= 0)
            return 0;

        ++Threads->nodes;

        Value bestValue = -VALUE_INFINITE;

        for (const auto& move : MoveList(pos))
        {
            if (!pos.legal(move))
                continue;

            StateInfo st;
            pos.do_move(move, st);

            Value value = -search(pos, -beta, -alpha, depth - 1, !cutNode);

            pos.undo_move(move);

            if (value > bestValue)
            {
                bestValue = value;

                if (value > alpha)
                {
                    alpha = value;

                    if (value >= beta)
                        break;
                }
            }
        }

        return bestValue;
    }
}