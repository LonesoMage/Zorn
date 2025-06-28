#include "move.h"
#include "board.h"
#include <string>

MoveList::MoveList(const Position& pos) : last(moves)
{
    last = generate<LEGAL>(pos, moves);
}

bool MoveList::contains(Move move) const
{
    for (const ExtMove* it = begin(); it != end(); ++it)
        if (it->move == move)
            return true;
    return false;
}

template<>
ExtMove* generate<LEGAL>(const Position& pos, ExtMove* moveList)
{
    ExtMove* cur = moveList;

    // Генеруємо тільки кілька базових ходів для тестування
    if (pos.side_to_move() == WHITE)
    {
        // Ходи пішаків
        for (int f = FILE_A; f <= FILE_H; f++)
        {
            Square from = make_square(File(f), RANK_2);
            if (pos.piece_on(from) == W_PAWN)
            {
                Square to = make_square(File(f), RANK_3);
                if (pos.empty(to))
                {
                    cur->move = make_move(from, to);
                    cur->value = VALUE_ZERO;
                    ++cur;

                    to = make_square(File(f), RANK_4);
                    if (pos.empty(to))
                    {
                        cur->move = make_move(from, to);
                        cur->value = VALUE_ZERO;
                        ++cur;
                    }
                }
            }
        }

        // Ходи коней
        Square knightSquares[] = { SQ_B1, SQ_G1 };
        for (Square from : knightSquares)
        {
            if (pos.piece_on(from) == W_KNIGHT)
            {
                Square moves[] = { SQ_A3, SQ_C3, SQ_F3, SQ_H3 };
                for (Square to : moves)
                {
                    if (pos.empty(to))
                    {
                        cur->move = make_move(from, to);
                        cur->value = VALUE_ZERO;
                        ++cur;
                    }
                }
            }
        }
    }
    else
    {
        // Аналогічно для чорних
        for (int f = FILE_A; f <= FILE_H; f++)
        {
            Square from = make_square(File(f), RANK_7);
            if (pos.piece_on(from) == B_PAWN)
            {
                Square to = make_square(File(f), RANK_6);
                if (pos.empty(to))
                {
                    cur->move = make_move(from, to);
                    cur->value = VALUE_ZERO;
                    ++cur;

                    to = make_square(File(f), RANK_5);
                    if (pos.empty(to))
                    {
                        cur->move = make_move(from, to);
                        cur->value = VALUE_ZERO;
                        ++cur;
                    }
                }
            }
        }
    }

    return cur;
}

std::string move_to_uci(Move m)
{
    if (m == MOVE_NONE)
        return "(none)";
    if (m == MOVE_NULL)
        return "0000";

    Square from = from_sq(m);
    Square to = to_sq(m);

    std::string move;
    move += char('a' + file_of(from));
    move += char('1' + rank_of(from));
    move += char('a' + file_of(to));
    move += char('1' + rank_of(to));

    if (type_of(m) == PROMOTION)
        move += " pnbrqk"[promotion_type(m)];

    return move;
}

Move uci_to_move(const Position& pos, std::string& str)
{
    if (str.length() == 5)
        str[4] = char(tolower(str[4]));

    for (const auto& m : MoveList(pos))
        if (str == move_to_uci(m))
            return m;

    return MOVE_NONE;
}