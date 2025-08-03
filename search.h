#ifndef SEARCH_H
#define SEARCH_H

#include "types.h"
#include "move.h"
#include <vector>
#include <chrono>

class Position;

namespace Search
{
    struct SearchInfo
    {
        std::chrono::steady_clock::time_point startTime;
        int timeLimit = 0;
        bool stopped = false;
        uint64_t nodeCount = 0;
        uint64_t maxNodes = 20000000;
    };

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