#ifndef EVAL_H
#define EVAL_H

#include "types.h"

class Position;

namespace Eval
{
    extern const Value PieceValues[PIECE_TYPE_NB];

    Value evaluate(const Position& pos);
    void init();
}

#endif