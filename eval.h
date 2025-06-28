#ifndef EVAL_H
#define EVAL_H

#include "types.h"

class Position;

namespace Eval
{
    Value evaluate(const Position& pos);
    void init();
}

#endif