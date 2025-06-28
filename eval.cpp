#include "eval.h"
#include "board.h"

namespace Eval
{
    const Value PieceValues[PIECE_TYPE_NB] =
    {
        0,
        100,
        320,
        330,
        500,
        900,
        0
    };

    const Value PawnBonus[64] =
    {
         0,  0,  0,  0,  0,  0,  0,  0,
        50, 50, 50, 50, 50, 50, 50, 50,
        10, 10, 20, 30, 30, 20, 10, 10,
         5,  5, 10, 25, 25, 10,  5,  5,
         0,  0,  0, 20, 20,  0,  0,  0,
         5, -5,-10,  0,  0,-10, -5,  5,
         5, 10, 10,-20,-20, 10, 10,  5,
         0,  0,  0,  0,  0,  0,  0,  0
    };

    const Value KnightBonus[64] =
    {
        -50,-40,-30,-30,-30,-30,-40,-50,
        -40,-20,  0,  0,  0,  0,-20,-40,
        -30,  0, 10, 15, 15, 10,  0,-30,
        -30,  5, 15, 20, 20, 15,  5,-30,
        -30,  0, 15, 20, 20, 15,  0,-30,
        -30,  5, 10, 15, 15, 10,  5,-30,
        -40,-20,  0,  5,  5,  0,-20,-40,
        -50,-40,-20,-30,-30,-20,-40,-50
    };

    const Value BishopBonus[64] =
    {
        -20,-10,-10,-10,-10,-10,-10,-20,
        -10,  0,  0,  0,  0,  0,  0,-10,
        -10,  0,  5, 10, 10,  5,  0,-10,
        -10,  5,  5, 10, 10,  5,  5,-10,
        -10,  0, 10, 10, 10, 10,  0,-10,
        -10, 10, 10, 10, 10, 10, 10,-10,
        -10,  5,  0,  0,  0,  0,  5,-10,
        -20,-10,-40,-10,-10,-40,-10,-20
    };

    const Value RookBonus[64] =
    {
         0,  0,  0,  0,  0,  0,  0,  0,
         5, 10, 10, 10, 10, 10, 10,  5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
        -5,  0,  0,  0,  0,  0,  0, -5,
         0,  0,  0,  5,  5,  0,  0,  0
    };

    const Value QueenBonus[64] =
    {
        -20,-10,-10, -5, -5,-10,-10,-20,
        -10,  0,  0,  0,  0,  0,  0,-10,
        -10,  0,  5,  5,  5,  5,  0,-10,
         -5,  0,  5,  5,  5,  5,  0, -5,
          0,  0,  5,  5,  5,  5,  0, -5,
        -10,  5,  5,  5,  5,  5,  0,-10,
        -10,  0,  5,  0,  0,  0,  0,-10,
        -20,-10,-10, -5, -5,-10,-10,-20
    };

    const Value KingBonus[64] =
    {
        -30,-40,-40,-50,-50,-40,-40,-30,
        -30,-40,-40,-50,-50,-40,-40,-30,
        -30,-40,-40,-50,-50,-40,-40,-30,
        -30,-40,-40,-50,-50,-40,-40,-30,
        -20,-30,-30,-40,-40,-30,-30,-20,
        -10,-20,-20,-20,-20,-20,-20,-10,
         20, 20,  0,  0,  0,  0, 20, 20,
         20, 30, 10,  0,  0, 10, 30, 20
    };

    void init()
    {
    }

    static inline Value evaluatePieceSquare(Piece pc, Square sq)
    {
        PieceType pt = type_of(pc);
        Color c = color_of(pc);

        Square relativeSquare = c == WHITE ? sq : Square(sq ^ 56);

        switch (pt)
        {
        case PAWN:   return PawnBonus[relativeSquare];
        case KNIGHT: return KnightBonus[relativeSquare];
        case BISHOP: return BishopBonus[relativeSquare];
        case ROOK:   return RookBonus[relativeSquare];
        case QUEEN:  return QueenBonus[relativeSquare];
        case KING:   return KingBonus[relativeSquare];
        default:     return 0;
        }
    }

    static Value evaluatePawns(const Position& pos)
    {
        Value value = 0;

        for (File f = FILE_A; f <= FILE_H; f = File(f + 1))
        {
            int whitePawns = 0, blackPawns = 0;

            for (Rank r = RANK_2; r <= RANK_7; r = Rank(r + 1))
            {
                Square sq = make_square(f, r);
                Piece pc = pos.piece_on(sq);

                if (pc == W_PAWN) whitePawns++;
                else if (pc == B_PAWN) blackPawns++;
            }

            if (whitePawns > 1) value -= 50 * (whitePawns - 1);
            if (blackPawns > 1) value += 50 * (blackPawns - 1);

            bool whiteIsolated = whitePawns > 0;
            bool blackIsolated = blackPawns > 0;

            if (f > FILE_A)
            {
                for (Rank r = RANK_2; r <= RANK_7; r = Rank(r + 1))
                {
                    Square sq = make_square(File(f - 1), r);
                    Piece pc = pos.piece_on(sq);
                    if (pc == W_PAWN) whiteIsolated = false;
                    if (pc == B_PAWN) blackIsolated = false;
                }
            }

            if (f < FILE_H)
            {
                for (Rank r = RANK_2; r <= RANK_7; r = Rank(r + 1))
                {
                    Square sq = make_square(File(f + 1), r);
                    Piece pc = pos.piece_on(sq);
                    if (pc == W_PAWN) whiteIsolated = false;
                    if (pc == B_PAWN) blackIsolated = false;
                }
            }

            if (whiteIsolated && whitePawns > 0) value -= 20;
            if (blackIsolated && blackPawns > 0) value += 20;
        }

        return value;
    }

    static Value evaluateKingSafety(const Position& pos)
    {
        Value value = 0;

        Square whiteKing = SQ_NONE, blackKing = SQ_NONE;
        for (Square s = SQ_A1; s <= SQ_H8; s = Square(s + 1))
        {
            Piece pc = pos.piece_on(s);
            if (pc == W_KING) whiteKing = s;
            else if (pc == B_KING) blackKing = s;
        }

        if (whiteKing != SQ_NONE && blackKing != SQ_NONE)
        {
            if (rank_of(whiteKing) == RANK_1)
            {
                File f = file_of(whiteKing);
                int shield = 0;

                if (f > FILE_A && pos.piece_on(make_square(File(f - 1), RANK_2)) == W_PAWN) shield++;
                if (pos.piece_on(make_square(f, RANK_2)) == W_PAWN) shield++;
                if (f < FILE_H && pos.piece_on(make_square(File(f + 1), RANK_2)) == W_PAWN) shield++;

                value += shield * 15;
            }

            if (rank_of(blackKing) == RANK_8)
            {
                File f = file_of(blackKing);
                int shield = 0;

                if (f > FILE_A && pos.piece_on(make_square(File(f - 1), RANK_7)) == B_PAWN) shield++;
                if (pos.piece_on(make_square(f, RANK_7)) == B_PAWN) shield++;
                if (f < FILE_H && pos.piece_on(make_square(File(f + 1), RANK_7)) == B_PAWN) shield++;

                value -= shield * 15;
            }
        }

        return value;
    }

    static Value evaluateDevelopment(const Position& pos)
    {
        Value value = 0;

        if (pos.piece_on(SQ_B1) != W_KNIGHT) value += 10;
        if (pos.piece_on(SQ_G1) != W_KNIGHT) value += 10;
        if (pos.piece_on(SQ_C1) != W_BISHOP) value += 8;
        if (pos.piece_on(SQ_F1) != W_BISHOP) value += 8;

        if (pos.piece_on(SQ_B8) != B_KNIGHT) value -= 10;
        if (pos.piece_on(SQ_G8) != B_KNIGHT) value -= 10;
        if (pos.piece_on(SQ_C8) != B_BISHOP) value -= 8;
        if (pos.piece_on(SQ_F8) != B_BISHOP) value -= 8;

        return value;
    }

    Value evaluate(const Position& pos)
    {
        Value value = VALUE_ZERO;

        for (Square s = SQ_A1; s <= SQ_H8; s = Square(s + 1))
        {
            Piece pc = pos.piece_on(s);
            if (pc != NO_PIECE)
            {
                Value pieceValue = PieceValues[type_of(pc)];
                Value positionValue = evaluatePieceSquare(pc, s);

                if (color_of(pc) == WHITE)
                    value += pieceValue + positionValue;
                else
                    value -= pieceValue + positionValue;
            }
        }

        if (abs(value) < 200)
        {
            value += evaluatePawns(pos);
            value += evaluateKingSafety(pos);

            if (pos.game_ply() < 20)
                value += evaluateDevelopment(pos);
        }

        value += (pos.side_to_move() == WHITE) ? 10 : -10;

        return pos.side_to_move() == WHITE ? value : -value;
    }
}