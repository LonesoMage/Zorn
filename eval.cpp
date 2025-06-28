#include "eval.h"
#include "board.h"

namespace Eval
{
    const Value PieceValues[PIECE_TYPE_NB] = {
        0,    // NO_PIECE_TYPE
        100,  // PAWN
        320,  // KNIGHT  
        330,  // BISHOP
        500,  // ROOK
        900,  // QUEEN
        0     // KING
    };

    void init()
    {
    }

    Value evaluate(const Position& pos)
    {
        Value value = VALUE_ZERO;

        // Підраховуємо матеріал
        for (Square s = SQ_A1; s <= SQ_H8; s = Square(s + 1))
        {
            Piece pc = pos.piece_on(s);
            if (pc != NO_PIECE)
            {
                Value pieceValue = PieceValues[type_of(pc)];
                if (color_of(pc) == WHITE)
                    value += pieceValue;
                else
                    value -= pieceValue;
            }
        }

        // Бонус за розвиток центральних пішаків
        if (pos.piece_on(SQ_E4) == W_PAWN) value += 30;
        if (pos.piece_on(SQ_D4) == W_PAWN) value += 30;
        if (pos.piece_on(SQ_E5) == B_PAWN) value -= 30;
        if (pos.piece_on(SQ_D5) == B_PAWN) value -= 30;

        // Штраф за ходи крайніми пішаками
        if (pos.piece_on(SQ_A3) == W_PAWN) value -= 20;
        if (pos.piece_on(SQ_H3) == W_PAWN) value -= 20;
        if (pos.piece_on(SQ_A6) == B_PAWN) value += 20;
        if (pos.piece_on(SQ_H6) == B_PAWN) value += 20;

        return pos.side_to_move() == WHITE ? value : -value;
    }
}