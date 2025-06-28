#ifndef SEARCH_H
#define SEARCH_H

#include "types.h"
#include "move.h"
#include <vector>

class Position;

namespace Search
{
    struct Limits
    {
        Limits()
        {
            time[WHITE] = time[BLACK] = inc[WHITE] = inc[BLACK] = npmsec = movetime = movestogo = depth = nodes = mate = perft = infinite = 0;
        }

        bool use_time_management() const
        {
            return !(mate | movetime | depth | nodes | perft | infinite);
        }

        std::vector<Move> searchmoves;
        int time[COLOR_NB], inc[COLOR_NB], npmsec, movetime, movestogo, depth, nodes, mate, perft, infinite;
    };

    void init();
    void start(Position& pos, const Limits& limits);
    Value search(Position& pos, Value alpha, Value beta, Depth depth, bool cutNode = true);
}

class Thread
{
public:
    Thread() : idx(0), nodes(0), rootDepth(0) {}
    Thread(size_t n) : idx(n), nodes(0), rootDepth(0) {}
    virtual ~Thread() = default;

    size_t idx;
    uint64_t nodes;
    Depth rootDepth;
};

extern Thread* Threads;

#endif