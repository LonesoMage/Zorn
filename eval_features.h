#ifndef EVAL_FEATURES_H
#define EVAL_FEATURES_H

#include "types.h"

class Position;

namespace Eval
{
    Value evaluatePieceSquare(Piece pc, Square sq, bool isEndgame);
    Value evaluateCenter(const Position& pos);
    Value evaluateKnightPenalties(const Position& pos);
    Value evaluateKingSafety(const Position& pos);
    Value evaluateMobility(const Position& pos);
}

#endif