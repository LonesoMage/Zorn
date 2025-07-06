#include "eval.h"
#include "board.h"

namespace Eval
{
    const Value PieceValues[PIECE_TYPE_NB] =
    {
        0, 100, 320, 330, 500, 900, 0
    };

    const Value PieceValuesMG[PIECE_TYPE_NB] =
    {
        0, 124, 781, 825, 1276, 2538, 0
    };

    const Value PieceValuesEG[PIECE_TYPE_NB] =
    {
        0, 206, 854, 915, 1380, 2682, 0
    };

    const int PiecePhase[PIECE_TYPE_NB] = { 0, 0, 1, 1, 2, 4, 0 };

    const Value PawnTableMG[64] =
    {
        0,   0,   0,   0,   0,   0,   0,   0,
       98, 134,  61,  95,  68, 126,  34, -11,
       -6,   7,  26,  31,  65,  56,  25, -20,
      -14,  13,   6,  21,  23,  12,  17, -23,
      -27,  -2,  -5,  12,  17,   6,  10, -25,
      -26,  -4,  -4, -10,   3,   3,  33, -12,
      -35,  -1, -20, -23, -15,  24,  38, -22,
        0,   0,   0,   0,   0,   0,   0,   0
    };

    const Value PawnTableEG[64] =
    {
        0,   0,   0,   0,   0,   0,   0,   0,
      178, 173, 158, 134, 147, 132, 165, 187,
       94, 100,  85,  67,  56,  53,  82,  84,
       32,  24,  13,   5,  -2,   4,  17,  17,
       13,   9,  -3,  -7,  -7,  -8,   3,  -1,
        4,   7,  -6,   1,   0,  -5,  -1,  -8,
       13,   8,   8,  10,  13,   0,   2,  -7,
        0,   0,   0,   0,   0,   0,   0,   0
    };

    const Value KnightTableMG[64] =
    {
      -167, -89, -34, -49,  61, -97, -15, -107,
       -73, -41,  72,  36,  23,  62,   7,  -17,
       -47,  60,  37,  65,  84, 129,  73,   44,
        -9,  17,  19,  53,  37,  69,  18,   22,
       -13,   4,  16,  13,  28,  19,  21,   -8,
       -23,  -9,  12,  10,  19,  17,  25,  -16,
       -29, -53, -12,  -3,  -1,  18, -14,  -19,
      -105, -21, -58, -33, -17, -28, -19,  -23
    };

    const Value KnightTableEG[64] =
    {
      -58, -38, -13, -28, -31, -27, -63, -99,
      -25,  -8, -25,  -2,  -9, -25, -24, -52,
      -24, -20,  10,   9,  -1,  -9, -19, -41,
      -17,   3,  22,  22,  22,  11,   8, -18,
      -18,  -6,  16,  25,  16,  17,   4, -18,
      -23,  -3,  -1,  15,  10,  -3, -20, -22,
      -42, -20, -10,  -5,  -2, -20, -23, -44,
      -29, -51, -23, -15, -22, -18, -50, -64
    };

    const Value BishopTableMG[64] =
    {
      -29,   4, -82, -37, -25, -42,   7,  -8,
      -26,  16, -18, -13,  30,  59,  18, -47,
      -16,  37,  43,  40,  35,  50,  37,  -2,
       -4,   5,  19,  50,  37,  37,   7,  -2,
       -6,  13,  13,  26,  34,  12,  10,   4,
        0,  15,  15,  15,  14,  27,  18,  10,
        4,  15,  16,   0,   7,  21,  33,   1,
      -33,  -3, -14, -21, -13, -12, -39, -21
    };

    const Value BishopTableEG[64] =
    {
      -14, -21, -11,  -8, -7,  -9, -17, -24,
       -8,  -4,   7, -12, -3, -13,  -4, -14,
        2,  -8,   0,  -1, -2,   6,   0,   4,
       -3,   9,  12,   9, 14,  10,   3,   2,
       -6,   3,  13,  19,  7,  10,  -3,  -9,
      -12,  -3,   8,  10, 13,   3,  -7, -15,
      -14, -18,  -7,  -1,  4,  -9, -15, -27,
      -23,  -9, -23,  -5, -9, -16,  -5, -17
    };

    const Value RookTableMG[64] =
    {
       32,  42,  32,  51, 63,  9,  31,  43,
       27,  32,  58,  62, 80, 67,  26,  44,
       -5,  19,  26,  36, 17, 45,  61,  16,
      -24, -11,   7,  26, 24, 35,  -8, -20,
      -36, -26, -12,  -1,  9, -7,   6, -23,
      -45, -25, -16, -17,  3,  0,  -5, -33,
      -44, -16, -20,  -9, -1, 11,  -6, -71,
      -19, -13,   1,  17, 16,  7, -37, -26
    };

    const Value RookTableEG[64] =
    {
       13, 10, 18, 15, 12,  12,   8,   5,
       11, 13, 13, 11, -3,   3,   8,   3,
        7,  7,  7,  5,  4,  -3,  -5,  -3,
        4,  3, 13,  1,  2,   1,  -1,   2,
        3,  5,  8,  4, -5,  -6,  -8, -11,
       -4,  0, -5, -1, -7, -12,  -8, -16,
       -6, -6,  0,  2, -9,  -9, -11,  -3,
       -9,  2,  3, -1, -5, -13,   4, -20
    };

    const Value QueenTableMG[64] =
    {
      -28,   0,  29,  12,  59,  44,  43,  45,
      -24, -39,  -5,   1, -16,  57,  28,  54,
      -13, -17,   7,   8,  29,  56,  47,  57,
      -27, -27, -16, -16,  -1,  17,  -2,   1,
       -9, -26,  -9, -10,  -2,  -4,   3,  -3,
      -14,   2, -11,  -2,  -5,   2,  14,   5,
      -35,  -8,  11,   2,   8,  15,  -3,   1,
       -1, -18,  -9,  10, -15, -25, -31, -50
    };

    const Value QueenTableEG[64] =
    {
       -9,  22,  22,  27,  27,  19,  10,  20,
      -17,  20,  32,  41,  58,  25,  30,   0,
      -20,   6,   9,  49,  47,  35,  19,   9,
        3,  22,  24,  45,  57,  40,  57,  36,
      -18,  28,  19,  47,  31,  34,  39,  23,
      -16, -27,  15,   6,   9,  17,  10,   5,
      -22, -23, -30, -16, -16, -23, -36, -32,
      -33, -28, -22, -43,  -5, -32, -20, -41
    };

    const Value KingTableMG[64] =
    {
      -65,  23,  16, -15, -56, -34,   2,  13,
       29,  -1, -20,  -7,  -8,  -4, -38, -29,
       -9,  24,   2, -16, -20,   6,  22, -22,
      -17, -20, -12, -27, -30, -25, -14, -36,
      -49,  -1, -27, -39, -46, -44, -33, -51,
      -14, -14, -22, -46, -44, -30, -15, -27,
        1,   7,  -8, -64, -43, -16,   9,   8,
      -15,  36,  12, -54,   8, -28,  24,  14
    };

    const Value KingTableEG[64] =
    {
      -74, -35, -18, -18, -11,  15,   4, -17,
      -12,  17,  14,  17,  17,  38,  23,  11,
       10,  17,  23,  15,  20,  45,  44,  13,
       -8,  22,  24,  27,  26,  33,  26,   3,
      -18,  -4,  21,  24,  27,  23,   9, -11,
      -19,  -3,  11,  21,  23,  16,   7,  -9,
      -27, -11,   4,  13,  14,   4,  -5, -17,
      -53, -34, -21, -11, -28, -14, -24, -43
    };

    void init()
    {
    }

    static inline Value evaluatePieceSquare(Piece pc, Square sq, bool isEndgame)
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

    static Value evaluateMobility(const Position& pos)
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
                Value mobilityBonus = mobility * (pt == QUEEN ? 2 : pt == ROOK ? 3 : 4);

                if (c == WHITE)
                    value += mobilityBonus;
                else
                    value -= mobilityBonus;
            }
        }

        return value;
    }

    Value evaluate(const Position& pos)
    {
        Value mgScore = 0, egScore = 0;
        int gamePhase = 0;

        for (Square s = SQ_A1; s <= SQ_H8; s = Square(s + 1))
        {
            Piece pc = pos.piece_on(s);
            if (pc != NO_PIECE)
            {
                PieceType pt = type_of(pc);
                Color c = color_of(pc);

                Value mgPieceValue = PieceValuesMG[pt];
                Value egPieceValue = PieceValuesEG[pt];
                Value mgPST = evaluatePieceSquare(pc, s, false);
                Value egPST = evaluatePieceSquare(pc, s, true);

                if (c == WHITE)
                {
                    mgScore += mgPieceValue + mgPST;
                    egScore += egPieceValue + egPST;
                }
                else
                {
                    mgScore -= mgPieceValue + mgPST;
                    egScore -= egPieceValue + egPST;
                }

                gamePhase += PiecePhase[pt];
            }
        }

        mgScore += evaluatePawns(pos);
        egScore += evaluatePawns(pos);

        mgScore += evaluateKingSafety(pos);

        mgScore += evaluateMobility(pos);
        egScore += evaluateMobility(pos);

        mgScore += (pos.side_to_move() == WHITE) ? 10 : -10;
        egScore += (pos.side_to_move() == WHITE) ? 10 : -10;

        int maxPhase = 24;
        int finalPhase = gamePhase > maxPhase ? maxPhase : gamePhase;

        Value score = (mgScore * finalPhase + egScore * (maxPhase - finalPhase)) / maxPhase;

        return pos.side_to_move() == WHITE ? score : -score;
    }
}