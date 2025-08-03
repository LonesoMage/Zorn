#include "eval_features.h"
#include "board.h"
#include "bitboard.h"

namespace Eval
{
    const Value PawnTableMG[64] =
    {
        0,   0,   0,   0,   0,   0,   0,   0,
        5,  10,  15,  20,  20,  15,  10,   5,
       -5,   0,  10,  25,  25,  10,   0,  -5,
      -10,   0,  10,  25,  25,  10,   0, -10,
      -15,  -5,   5,  20,  20,   5,  -5, -15,
      -20, -10,   0,   5,   5,   0, -10, -20,
      -25, -15,  -5,   0,   0,  -5, -15, -25,
        0,   0,   0,   0,   0,   0,   0,   0
    };

    const Value PawnTableEG[64] =
    {
        0,   0,   0,   0,   0,   0,   0,   0,
      180, 175, 160, 135, 135, 160, 175, 180,
       95, 100,  85,  70,  70,  85, 100,  95,
       35,  25,  15,   5,   5,  15,  25,  35,
       15,  10,   0,  -5,  -5,   0,  10,  15,
        5,   5,  -5,   0,   0,  -5,   5,   5,
       15,  10,  10,  15,  15,  10,  10,  15,
        0,   0,   0,   0,   0,   0,   0,   0
    };

    const Value KnightTableMG[64] =
    {
     -150, -100, -80, -80, -80, -80, -100, -150,
      -100,  -50, -20, -10, -10, -20,  -50, -100,
       -80,  -20,   0,   5,   5,   0,  -20,  -80,
       -80,  -10,   5,  10,  10,   5,  -10,  -80,
       -80,  -10,   5,  10,  10,   5,  -10,  -80,
       -80,  -20,   0,   5,   5,   0,  -20,  -80,
      -100,  -50, -20, -10, -10, -20,  -50, -100,
      -200, -150, -120, -120, -120, -120, -150, -200
    };

    const Value KnightTableEG[64] =
    {
      -50, -40, -30, -30, -30, -30, -40, -50,
      -30, -20, -10, -10, -10, -10, -20, -30,
      -20, -10,   0,   5,   5,   0, -10, -20,
      -15,  -5,   5,  10,  10,   5,  -5, -15,
      -15,  -5,   5,  10,  10,   5,  -5, -15,
      -20, -10,   0,   5,   5,   0, -10, -20,
      -30, -20, -10, -10, -10, -10, -20, -30,
      -50, -40, -30, -30, -30, -30, -40, -50
    };

    const Value BishopTableMG[64] =
    {
      -20, -10, -10, -10, -10, -10, -10, -20,
      -10,   5,   0,   0,   0,   0,   5, -10,
      -10,  10,  10,  10,  10,  10,  10, -10,
      -10,   0,  10,  10,  10,  10,   0, -10,
      -10,   5,   5,  10,  10,   5,   5, -10,
      -10,   0,   5,  10,  10,   5,   0, -10,
      -10,   0,   0,   0,   0,   0,   0, -10,
      -20, -10, -10, -10, -10, -10, -10, -20
    };

    const Value BishopTableEG[64] =
    {
      -15, -10, -10, -10, -10, -10, -10, -15,
      -10,   0,   0,   0,   0,   0,   0, -10,
      -10,   0,   5,  10,  10,   5,   0, -10,
      -10,   5,   5,  10,  10,   5,   5, -10,
      -10,   0,  10,  10,  10,  10,   0, -10,
      -10,  10,  10,  10,  10,  10,  10, -10,
      -10,   5,   0,   0,   0,   0,   5, -10,
      -15, -10, -10, -10, -10, -10, -10, -15
    };

    const Value RookTableMG[64] =
    {
        0,   0,   0,   5,   5,   0,   0,   0,
       -5,   0,   0,   0,   0,   0,   0,  -5,
       -5,   0,   0,   0,   0,   0,   0,  -5,
       -5,   0,   0,   0,   0,   0,   0,  -5,
       -5,   0,   0,   0,   0,   0,   0,  -5,
       -5,   0,   0,   0,   0,   0,   0,  -5,
        5,  10,  10,  10,  10,  10,  10,   5,
        0,   0,   0,   0,   0,   0,   0,   0
    };

    const Value RookTableEG[64] =
    {
        0,   0,   0,   0,   0,   0,   0,   0,
        5,  10,  10,  10,  10,  10,  10,   5,
       -5,   0,   0,   0,   0,   0,   0,  -5,
       -5,   0,   0,   0,   0,   0,   0,  -5,
       -5,   0,   0,   0,   0,   0,   0,  -5,
       -5,   0,   0,   0,   0,   0,   0,  -5,
       -5,   0,   0,   0,   0,   0,   0,  -5,
        0,   0,   0,   0,   0,   0,   0,   0
    };

    const Value QueenTableMG[64] =
    {
      -20, -10, -10,  -5,  -5, -10, -10, -20,
      -10,   0,   5,   0,   0,   0,   0, -10,
      -10,   5,   5,   5,   5,   5,   0, -10,
        0,   0,   5,   5,   5,   5,   0,  -5,
       -5,   0,   5,   5,   5,   5,   0,  -5,
      -10,   0,   5,   5,   5,   5,   0, -10,
      -10,   0,   0,   0,   0,   0,   0, -10,
      -20, -10, -10,  -5,  -5, -10, -10, -20
    };

    const Value QueenTableEG[64] =
    {
      -20, -10, -10,  -5,  -5, -10, -10, -20,
      -10,   0,   0,   0,   0,   0,   0, -10,
      -10,   0,   5,   5,   5,   5,   0, -10,
       -5,   0,   5,   5,   5,   5,   0,  -5,
        0,   0,   5,   5,   5,   5,   0,   0,
      -10,   5,   5,   5,   5,   5,   0, -10,
      -10,   0,   5,   0,   0,   0,   0, -10,
      -20, -10, -10,  -5,  -5, -10, -10, -20
    };

    const Value KingTableMG[64] =
    {
       20,  30,  10,   0,   0,  10,  30,  20,
       20,  20,   0,   0,   0,   0,  20,  20,
      -10, -20, -20, -20, -20, -20, -20, -10,
      -20, -30, -30, -40, -40, -30, -30, -20,
      -30, -40, -40, -50, -50, -40, -40, -30,
      -30, -40, -40, -50, -50, -40, -40, -30,
      -30, -40, -40, -50, -50, -40, -40, -30,
      -30, -40, -40, -50, -50, -40, -40, -30
    };

    const Value KingTableEG[64] =
    {
      -50, -30, -30, -30, -30, -30, -30, -50,
      -30, -30,   0,   0,   0,   0, -30, -30,
      -30, -10,  20,  30,  30,  20, -10, -30,
      -30, -10,  30,  40,  40,  30, -10, -30,
      -30, -10,  30,  40,  40,  30, -10, -30,
      -30, -10,  20,  30,  30,  20, -10, -30,
      -30, -20, -10,   0,   0, -10, -20, -30,
      -50, -40, -30, -20, -20, -30, -40, -50
    };

    Value evaluatePieceSquare(Piece pc, Square sq, bool isEndgame)
    {
        PieceType pt = type_of(pc);
        Color c = color_of(pc);
        Square relativeSquare = c == WHITE ? sq : Square(sq ^ 56);

        const Value* table;

        switch (pt)
        {
        case PAWN:   table = isEndgame ? PawnTableEG : PawnTableMG; break;
        case KNIGHT: table = isEndgame ? KnightTableEG : KnightTableMG; break;
        case BISHOP: table = isEndgame ? BishopTableEG : BishopTableMG; break;
        case ROOK:   table = isEndgame ? RookTableEG : RookTableMG; break;
        case QUEEN:  table = isEndgame ? QueenTableEG : QueenTableMG; break;
        case KING:   table = isEndgame ? KingTableEG : KingTableMG; break;
        default:     return 0;
        }

        return table[relativeSquare];
    }

    Value evaluateCenter(const Position& pos)
    {
        Value value = 0;

        if (pos.piece_on(SQ_E4) == W_PAWN) value += 20;
        if (pos.piece_on(SQ_D4) == W_PAWN) value += 20;
        if (pos.piece_on(SQ_E5) == B_PAWN) value -= 20;
        if (pos.piece_on(SQ_D5) == B_PAWN) value -= 20;

        if (pos.piece_on(SQ_E3) == W_PAWN) value += 10;
        if (pos.piece_on(SQ_D3) == W_PAWN) value += 10;
        if (pos.piece_on(SQ_E6) == B_PAWN) value -= 10;
        if (pos.piece_on(SQ_D6) == B_PAWN) value -= 10;

        return value;
    }

    Value evaluateKnightPenalties(const Position& pos)
    {
        Value value = 0;

        if (pos.piece_on(SQ_A3) == W_KNIGHT) value -= 50;  // менший штраф
        if (pos.piece_on(SQ_H3) == W_KNIGHT) value -= 50;
        if (pos.piece_on(SQ_A6) == B_KNIGHT) value += 50;
        if (pos.piece_on(SQ_H6) == B_KNIGHT) value += 50;

        if (pos.piece_on(SQ_A1) == W_KNIGHT) value -= 100; // штраф за кут
        if (pos.piece_on(SQ_H1) == W_KNIGHT) value -= 100;
        if (pos.piece_on(SQ_A8) == B_KNIGHT) value += 100;
        if (pos.piece_on(SQ_H8) == B_KNIGHT) value += 100;

        if (pos.piece_on(SQ_C3) == W_KNIGHT) value += 30;
        if (pos.piece_on(SQ_F3) == W_KNIGHT) value += 30;
        if (pos.piece_on(SQ_C6) == B_KNIGHT) value -= 30;
        if (pos.piece_on(SQ_F6) == B_KNIGHT) value -= 30;

        return value;
    }

    Value evaluateKingSafety(const Position& pos)
    {
        Value value = 0;

        Square whiteKing = pos.square<KING>(WHITE);
        Square blackKing = pos.square<KING>(BLACK);

        if (rank_of(whiteKing) == RANK_1)
        {
            File f = file_of(whiteKing);
            int shield = 0;

            if (f > FILE_A && pos.piece_on(make_square(File(f - 1), RANK_2)) == W_PAWN) shield++;
            if (pos.piece_on(make_square(f, RANK_2)) == W_PAWN) shield++;
            if (f < FILE_H && pos.piece_on(make_square(File(f + 1), RANK_2)) == W_PAWN) shield++;

            value += shield * 10;
        }

        if (rank_of(blackKing) == RANK_8)
        {
            File f = file_of(blackKing);
            int shield = 0;

            if (f > FILE_A && pos.piece_on(make_square(File(f - 1), RANK_7)) == B_PAWN) shield++;
            if (pos.piece_on(make_square(f, RANK_7)) == B_PAWN) shield++;
            if (f < FILE_H && pos.piece_on(make_square(File(f + 1), RANK_7)) == B_PAWN) shield++;

            value -= shield * 10;
        }

        return value;
    }

    Value evaluateMobility(const Position& pos)
    {
        Value value = 0;

        for (Square s = SQ_A1; s <= SQ_H8; s = Square(s + 1))
        {
            Piece pc = pos.piece_on(s);
            if (pc == NO_PIECE) continue;

            PieceType pt = type_of(pc);
            Color c = color_of(pc);

            if (pt == KNIGHT || pt == BISHOP || pt == ROOK || pt == QUEEN)
            {
                Bitboard attacks = attacks_bb(pt, s, pos.pieces());
                attacks &= ~pos.pieces(c);

                int mobility = popcount(attacks);
                Value mobilityBonus = mobility * (pt == QUEEN ? 1 : pt == ROOK ? 1 : 2);

                if (c == WHITE)
                    value += mobilityBonus;
                else
                    value -= mobilityBonus;
            }
        }

        return value;
    }
}