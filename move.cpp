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
    Color us = pos.side_to_move();

    for (Square from = SQ_A1; from <= SQ_H8; from = Square(from + 1))
    {
        Piece pc = pos.piece_on(from);
        if (pc == NO_PIECE || color_of(pc) != us)
            continue;

        PieceType pt = type_of(pc);

        if (pt == PAWN)
        {
            int delta = (us == WHITE) ? 8 : -8;
            Square to = Square(from + delta);

            if (is_ok(to) && pos.empty(to))
            {
                if ((us == WHITE && rank_of(to) == RANK_8) || (us == BLACK && rank_of(to) == RANK_1))
                {
                    cur->move = make<PROMOTION>(from, to, QUEEN);
                    cur->value = VALUE_ZERO;
                    ++cur;
                    cur->move = make<PROMOTION>(from, to, ROOK);
                    cur->value = VALUE_ZERO;
                    ++cur;
                    cur->move = make<PROMOTION>(from, to, BISHOP);
                    cur->value = VALUE_ZERO;
                    ++cur;
                    cur->move = make<PROMOTION>(from, to, KNIGHT);
                    cur->value = VALUE_ZERO;
                    ++cur;
                }
                else
                {
                    cur->move = make_move(from, to);
                    cur->value = VALUE_ZERO;
                    ++cur;

                    if ((us == WHITE && rank_of(from) == RANK_2) ||
                        (us == BLACK && rank_of(from) == RANK_7))
                    {
                        to = Square(from + 2 * delta);
                        if (is_ok(to) && pos.empty(to))
                        {
                            cur->move = make_move(from, to);
                            cur->value = VALUE_ZERO;
                            ++cur;
                        }
                    }
                }
            }

            for (int side = -1; side <= 1; side += 2)
            {
                to = Square(from + delta + side);
                if (is_ok(to) && abs(int(file_of(to)) - int(file_of(from))) == 1)
                {
                    Piece captured = pos.piece_on(to);
                    if (captured != NO_PIECE && color_of(captured) != us)
                    {
                        if ((us == WHITE && rank_of(to) == RANK_8) || (us == BLACK && rank_of(to) == RANK_1))
                        {
                            cur->move = make<PROMOTION>(from, to, QUEEN);
                            cur->value = VALUE_ZERO;
                            ++cur;
                            cur->move = make<PROMOTION>(from, to, ROOK);
                            cur->value = VALUE_ZERO;
                            ++cur;
                            cur->move = make<PROMOTION>(from, to, BISHOP);
                            cur->value = VALUE_ZERO;
                            ++cur;
                            cur->move = make<PROMOTION>(from, to, KNIGHT);
                            cur->value = VALUE_ZERO;
                            ++cur;
                        }
                        else
                        {
                            cur->move = make_move(from, to);
                            cur->value = VALUE_ZERO;
                            ++cur;
                        }
                    }
                    else if (to == pos.ep_square())
                    {
                        cur->move = make<ENPASSANT>(from, to);
                        cur->value = VALUE_ZERO;
                        ++cur;
                    }
                }
            }
        }
        else if (pt == KNIGHT)
        {
            int deltas[] = { -17, -15, -10, -6, 6, 10, 15, 17 };
            for (int delta : deltas)
            {
                Square to = Square(from + delta);
                if (is_ok(to))
                {
                    int fileDiff = abs(int(file_of(to)) - int(file_of(from)));
                    int rankDiff = abs(int(rank_of(to)) - int(rank_of(from)));

                    if ((fileDiff == 2 && rankDiff == 1) || (fileDiff == 1 && rankDiff == 2))
                    {
                        Piece captured = pos.piece_on(to);
                        if (captured == NO_PIECE || color_of(captured) != us)
                        {
                            cur->move = make_move(from, to);
                            cur->value = VALUE_ZERO;
                            ++cur;
                        }
                    }
                }
            }
        }
        else if (pt == BISHOP)
        {
            int directions[] = { -9, -7, 7, 9 };
            for (int dir : directions)
            {
                for (Square to = Square(from + dir); is_ok(to) && distance(from, to) <= 7; to = Square(to + dir))
                {
                    int fileDiff = abs(int(file_of(to)) - int(file_of(from)));
                    int rankDiff = abs(int(rank_of(to)) - int(rank_of(from)));

                    if (fileDiff != rankDiff)
                        break;

                    Piece captured = pos.piece_on(to);
                    if (captured != NO_PIECE)
                    {
                        if (color_of(captured) != us)
                        {
                            cur->move = make_move(from, to);
                            cur->value = VALUE_ZERO;
                            ++cur;
                        }
                        break;
                    }

                    cur->move = make_move(from, to);
                    cur->value = VALUE_ZERO;
                    ++cur;
                }
            }
        }
        else if (pt == ROOK)
        {
            int directions[] = { -8, -1, 1, 8 };
            for (int dir : directions)
            {
                for (Square to = Square(from + dir); is_ok(to) && distance(from, to) <= 7; to = Square(to + dir))
                {
                    int fileDiff = abs(int(file_of(to)) - int(file_of(from)));
                    int rankDiff = abs(int(rank_of(to)) - int(rank_of(from)));

                    if ((fileDiff != 0 && rankDiff != 0))
                        break;

                    Piece captured = pos.piece_on(to);
                    if (captured != NO_PIECE)
                    {
                        if (color_of(captured) != us)
                        {
                            cur->move = make_move(from, to);
                            cur->value = VALUE_ZERO;
                            ++cur;
                        }
                        break;
                    }

                    cur->move = make_move(from, to);
                    cur->value = VALUE_ZERO;
                    ++cur;
                }
            }
        }
        else if (pt == QUEEN)
        {
            int directions[] = { -9, -8, -7, -1, 1, 7, 8, 9 };
            for (int dir : directions)
            {
                for (Square to = Square(from + dir); is_ok(to) && distance(from, to) <= 7; to = Square(to + dir))
                {
                    int fileDiff = abs(int(file_of(to)) - int(file_of(from)));
                    int rankDiff = abs(int(rank_of(to)) - int(rank_of(from)));

                    bool validQueenMove = (fileDiff == 0 || rankDiff == 0 || fileDiff == rankDiff);
                    if (!validQueenMove)
                        break;

                    Piece captured = pos.piece_on(to);
                    if (captured != NO_PIECE)
                    {
                        if (color_of(captured) != us)
                        {
                            cur->move = make_move(from, to);
                            cur->value = VALUE_ZERO;
                            ++cur;
                        }
                        break;
                    }

                    cur->move = make_move(from, to);
                    cur->value = VALUE_ZERO;
                    ++cur;
                }
            }
        }
        else if (pt == KING)
        {
            int directions[] = { -9, -8, -7, -1, 1, 7, 8, 9 };
            for (int dir : directions)
            {
                Square to = Square(from + dir);
                if (is_ok(to) && distance(from, to) == 1)
                {
                    Piece captured = pos.piece_on(to);
                    if (captured == NO_PIECE || color_of(captured) != us)
                    {
                        cur->move = make_move(from, to);
                        cur->value = VALUE_ZERO;
                        ++cur;
                    }
                }
            }

            if (pos.can_castle(us == WHITE ? WHITE_OO : BLACK_OO))
            {
                Square kto = us == WHITE ? SQ_G1 : SQ_G8;
                if (pos.empty(Square(from + 1)) && pos.empty(Square(from + 2)))
                {
                    cur->move = make<CASTLING>(from, kto);
                    cur->value = VALUE_ZERO;
                    ++cur;
                }
            }

            if (pos.can_castle(us == WHITE ? WHITE_OOO : BLACK_OOO))
            {
                Square kto = us == WHITE ? SQ_C1 : SQ_C8;
                if (pos.empty(Square(from - 1)) && pos.empty(Square(from - 2)) && pos.empty(Square(from - 3)))
                {
                    cur->move = make<CASTLING>(from, kto);
                    cur->value = VALUE_ZERO;
                    ++cur;
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
    {
        char pieces[] = " nbrq";
        move += pieces[promotion_type(m) - KNIGHT + 1];
    }

    return move;
}

Move uci_to_move(const Position& pos, std::string& str)
{
    if (str.length() < 4)
        return MOVE_NONE;

    Square from = make_square(File(str[0] - 'a'), Rank(str[1] - '1'));
    Square to = make_square(File(str[2] - 'a'), Rank(str[3] - '1'));

    for (const auto& m : MoveList(pos))
    {
        if (from_sq(m) == from && to_sq(m) == to)
        {
            if (str.length() == 5 && type_of(m) == PROMOTION)
            {
                PieceType pt = KNIGHT;
                switch (tolower(str[4]))
                {
                case 'n': pt = KNIGHT; break;
                case 'b': pt = BISHOP; break;
                case 'r': pt = ROOK; break;
                case 'q': pt = QUEEN; break;
                }
                if (promotion_type(m) == pt)
                    return m;
            }
            else if (str.length() == 4 && type_of(m) != PROMOTION)
            {
                return m;
            }
        }
    }

    return MOVE_NONE;
}