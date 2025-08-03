#ifndef SEARCH_UTILS_H
#define SEARCH_UTILS_H

#include "types.h"
#include "search.h"

class Position;

namespace Search
{
    SearchInfo& getSearchInfo();
    void initSearch(const Limits& limits, const Position& pos);
    void initTables();

    Value scoreMove(const Position& pos, Move move, Move ttMove, int ply);
    bool timeUp();
    void updateKillers(Move move, int ply);
    void updateHistory(Color us, Square from, Square to, int depth, bool cutoff);
    void clearKillers();
    Move getKillerMove(int ply, int index);
    int getReduction(int depth, int moveCount);
}

#endif