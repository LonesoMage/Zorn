#include "move.h"
#include "board.h"
#include <string>

template<>
ExtMove* generate<LEGAL>(const Position& pos, ExtMove* moveList);

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
                }

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

            for (int side = -1; side <= 1; side += 2)
            {
                to = Square(from + delta + side);
                if (is_ok(to) && abs(int(file_of(to)) - int(file_of(from))) == 1)
                {
                    Piece captured = pos.piece_on(to);
                    bool isEnPassant = (to == pos.ep_square());

                    if ((captured != NO_PIECE && color_of(captured) != us) || isEnPassant)
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
                        else if (isEnPassant)
                        {
                            cur->move = make<ENPASSANT>(from, to);
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

            if (us == WHITE)
            {
                if (pos.can_castle(WHITE_OO) && !pos.castling_impeded(WHITE_OO))
                {
                    cur->move = make<CASTLING>(SQ_E1, SQ_G1);
                    cur->value = VALUE_ZERO;
                    ++cur;
                }
                if (pos.can_castle(WHITE_OOO) && !pos.castling_impeded(WHITE_OOO))
                {
                    cur->move = make<CASTLING>(SQ_E1, SQ_C1);
                    cur->value = VALUE_ZERO;
                    ++cur;
                }
            }
            else
            {
                if (pos.can_castle(BLACK_OO) && !pos.castling_impeded(BLACK_OO))
                {
                    cur->move = make<CASTLING>(SQ_E8, SQ_G8);
                    cur->value = VALUE_ZERO;
                    ++cur;
                }
                if (pos.can_castle(BLACK_OOO) && !pos.castling_impeded(BLACK_OOO))
                {
                    cur->move = make<CASTLING>(SQ_E8, SQ_C8);
                    cur->value = VALUE_ZERO;
                    ++cur;
                }
            }
        }
    }

    ExtMove* end = cur;
    for (ExtMove* it = moveList; it != end; )
    {
        if (!pos.pseudo_legal(it->move))
        {
            *it = *(--end);
        }
        else
        {
            ++it;
        }
    }

    return end;
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
        PieceType pt = promotion_type(m);
        if (pt == QUEEN) move += 'q';
        else if (pt == ROOK) move += 'r';
        else if (pt == BISHOP) move += 'b';
        else if (pt == KNIGHT) move += 'n';
    }

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