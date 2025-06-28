#ifndef MOVE_H
#define MOVE_H

#include "types.h"
#include <string>

struct ExtMove
{
    Move move;
    Value value;

    operator Move() const
    {
        return move;
    }

    void operator=(Move m)
    {
        move = m;
    }

    bool operator<(const ExtMove& m) const
    {
        return value < m.value;
    }
};

class Position;

struct MoveList
{
private:
    ExtMove moves[256];
    ExtMove* last;

public:
    explicit MoveList(const Position& pos);

    const ExtMove* begin() const
    {
        return moves;
    }

    const ExtMove* end() const
    {
        return last;
    }

    size_t size() const
    {
        return last - moves;
    }

    bool contains(Move move) const;
};

template<GenType>
ExtMove* generate(const Position& pos, ExtMove* moveList);

template<>
ExtMove* generate<LEGAL>(const Position& pos, ExtMove* moveList);

std::string move_to_uci(Move m);
Move uci_to_move(const Position& pos, std::string& str);

#endif