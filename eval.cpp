#include "eval.h"
#include "eval_features.h"
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

    void init()
    {
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

        mgScore += evaluateCenter(pos);
        mgScore += evaluateKnightPenalties(pos);
        mgScore += evaluateKingSafety(pos);
        mgScore += evaluateMobility(pos);
        mgScore += evaluateThreats(pos);

        egScore += evaluateKingSafety(pos);
        egScore += evaluateMobility(pos);
        egScore += evaluateThreats(pos);

        mgScore += (pos.side_to_move() == WHITE) ? 10 : -10;
        egScore += (pos.side_to_move() == WHITE) ? 10 : -10;

        int maxPhase = 24;
        int finalPhase = gamePhase > maxPhase ? maxPhase : gamePhase;

        Value score = (mgScore * finalPhase + egScore * (maxPhase - finalPhase)) / maxPhase;

        return score;
    }
}