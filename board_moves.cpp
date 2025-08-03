#include "board.h"
#include "bitboard.h"

bool Position::legal(Move m) const
{
    if (!pseudo_legal(m))
        return false;

    Square from = from_sq(m);
    Square to = to_sq(m);
    Piece pc = piece_on(from);
    Color us = color_of(pc);
    Color them = ~us;

    if (type_of(m) == CASTLING)
    {
        Square kingSquare = from;

        if (attackers_to(kingSquare) & pieces(them))
            return false;

        Square step = to > from ? Square(from + 1) : Square(from - 1);
        for (Square s = step; s != to; s = to > from ? Square(s + 1) : Square(s - 1))
        {
            if (attackers_to(s) & pieces(them))
                return false;
        }

        if (attackers_to(to) & pieces(them))
            return false;

        return true;
    }

    Square kingSquare = square<KING>(us);
    if (type_of(pc) == KING)
        kingSquare = to;

    Bitboard occupied = pieces();
    occupied ^= square_bb(from);
    if (piece_on(to) != NO_PIECE && type_of(m) != ENPASSANT)
        occupied ^= square_bb(to);
    occupied |= square_bb(to);

    if (type_of(m) == ENPASSANT)
    {
        Square capturedSquare = Square(to - (us == WHITE ? 8 : -8));
        occupied ^= square_bb(capturedSquare);
    }

    return !(attackers_to(kingSquare, occupied) & pieces(them));
}

bool Position::pseudo_legal(const Move m) const
{
    Square from = from_sq(m);
    Square to = to_sq(m);

    if (!is_ok(from) || !is_ok(to) || from == to)
        return false;

    Piece pc = piece_on(from);
    if (pc == NO_PIECE || color_of(pc) != sideToMove)
        return false;

    if (type_of(m) == CASTLING)
    {
        if (type_of(pc) != KING)
            return false;

        CastlingRights cr = to == SQ_G1 ? WHITE_OO : to == SQ_C1 ? WHITE_OOO :
            to == SQ_G8 ? BLACK_OO : to == SQ_C8 ? BLACK_OOO : NO_CASTLING;

        return can_castle(cr) && !castling_impeded(cr);
    }

    Piece captured = piece_on(to);
    if (captured != NO_PIECE && color_of(captured) == sideToMove)
        return false;

    PieceType pt = type_of(pc);

    if (pt == PAWN)
    {
        int delta = to - from;
        Color us = color_of(pc);

        if (us == WHITE)
        {
            if (delta == 8 && captured == NO_PIECE)
                return true;
            if (delta == 16 && rank_of(from) == RANK_2 && captured == NO_PIECE)
                return true;
            if ((delta == 7 || delta == 9) && distance(from, to) == 1)
            {
                if (captured != NO_PIECE && color_of(captured) != us)
                    return true;
                if (to == ep_square())
                    return true;
            }
        }
        else
        {
            if (delta == -8 && captured == NO_PIECE)
                return true;
            if (delta == -16 && rank_of(from) == RANK_7 && captured == NO_PIECE)
                return true;
            if ((delta == -7 || delta == -9) && distance(from, to) == 1)
            {
                if (captured != NO_PIECE && color_of(captured) != us)
                    return true;
                if (to == ep_square())
                    return true;
            }
        }
        return false;
    }

    if (pt == KNIGHT)
    {
        int df = abs(file_of(to) - file_of(from));
        int dr = abs(rank_of(to) - rank_of(from));
        return (df == 1 && dr == 2) || (df == 2 && dr == 1);
    }

    if (pt == ROOK)
    {
        if (file_of(from) != file_of(to) && rank_of(from) != rank_of(to))
            return false;

        int step = from < to ? 1 : -1;
        if (file_of(from) == file_of(to))
        {
            step = from < to ? 8 : -8;
        }

        for (Square s = Square(from + step); s != to; s = Square(s + step))
        {
            if (piece_on(s) != NO_PIECE)
                return false;
        }
        return true;
    }

    if (pt == BISHOP)
    {
        int df = abs(file_of(to) - file_of(from));
        int dr = abs(rank_of(to) - rank_of(from));
        if (df != dr)
            return false;

        int stepf = file_of(to) > file_of(from) ? 1 : -1;
        int stepr = rank_of(to) > rank_of(from) ? 8 : -8;
        int step = stepf + stepr;

        for (Square s = Square(from + step); s != to; s = Square(s + step))
        {
            if (piece_on(s) != NO_PIECE)
                return false;
        }
        return true;
    }

    if (pt == QUEEN)
    {
        int df = abs(file_of(to) - file_of(from));
        int dr = abs(rank_of(to) - rank_of(from));

        bool straightMove = (file_of(from) == file_of(to)) || (rank_of(from) == rank_of(to));
        bool diagonalMove = (df == dr);

        if (!straightMove && !diagonalMove)
            return false;

        int step;
        if (straightMove)
        {
            step = from < to ? 1 : -1;
            if (file_of(from) == file_of(to))
                step = from < to ? 8 : -8;
        }
        else
        {
            int stepf = file_of(to) > file_of(from) ? 1 : -1;
            int stepr = rank_of(to) > rank_of(from) ? 8 : -8;
            step = stepf + stepr;
        }

        for (Square s = Square(from + step); s != to; s = Square(s + step))
        {
            if (piece_on(s) != NO_PIECE)
                return false;
        }
        return true;
    }

    if (pt == KING)
    {
        int df = abs(file_of(to) - file_of(from));
        int dr = abs(rank_of(to) - rank_of(from));
        return df <= 1 && dr <= 1;
    }

    return false;
}

void Position::do_move(Move m, StateInfo& newSt)
{
    newSt.previous = st;
    st = &newSt;
    ++gamePly;

    Square from = from_sq(m);
    Square to = to_sq(m);
    Piece pc = piece_on(from);

    st->capturedPiece = NO_PIECE;
    st->castlingRights = st->previous->castlingRights;
    st->epSquare = SQ_NONE;
    st->rule50 = st->previous->rule50 + 1;
    st->key = st->previous->key;
    st->pawnKey = st->previous->pawnKey;
    st->materialKey = st->previous->materialKey;
    st->psq = st->previous->psq;
    st->npMaterial[WHITE] = st->previous->npMaterial[WHITE];
    st->npMaterial[BLACK] = st->previous->npMaterial[BLACK];

    if (type_of(m) == NORMAL)
    {
        if (piece_on(to) != NO_PIECE)
        {
            st->capturedPiece = piece_on(to);
            remove_piece(to);
            st->rule50 = 0;
        }

        if (type_of(pc) == PAWN)
        {
            st->rule50 = 0;
            if (abs(to - from) == 16)
            {
                st->epSquare = Square((from + to) / 2);
            }
        }

        move_piece(from, to);
    }
    else if (type_of(m) == PROMOTION)
    {
        if (piece_on(to) != NO_PIECE)
        {
            st->capturedPiece = piece_on(to);
            remove_piece(to);
        }

        remove_piece(from);
        put_piece(make_piece(color_of(pc), promotion_type(m)), to);
        st->rule50 = 0;
    }
    else if (type_of(m) == ENPASSANT)
    {
        Square capturedSquare = Square(to - (sideToMove == WHITE ? 8 : -8));
        st->capturedPiece = piece_on(capturedSquare);
        remove_piece(capturedSquare);
        move_piece(from, to);
        st->rule50 = 0;
    }
    else if (type_of(m) == CASTLING)
    {
        Square rookFrom = castling_rook_square(to == SQ_G1 ? WHITE_OO :
            to == SQ_C1 ? WHITE_OOO :
            to == SQ_G8 ? BLACK_OO : BLACK_OOO);
        Square rookTo = to == SQ_G1 ? SQ_F1 : to == SQ_C1 ? SQ_D1 :
            to == SQ_G8 ? SQ_F8 : SQ_D8;

        move_piece(from, to);
        move_piece(rookFrom, rookTo);
    }

    st->castlingRights &= ~castlingRightsMask[from];
    st->castlingRights &= ~castlingRightsMask[to];

    sideToMove = Color(~sideToMove);

    st->checkersBB = 0ULL;
    if (count<KING>(WHITE) > 0 && count<KING>(BLACK) > 0)
    {
        st->checkersBB = attackers_to(square<KING>(sideToMove)) & pieces(~sideToMove);

        Square enemyKing = square<KING>(~sideToMove);
        st->checkSquares[PAWN] = pawn_attacks_bb(~sideToMove, enemyKing);
        st->checkSquares[KNIGHT] = knight_attacks_bb(enemyKing);
        st->checkSquares[BISHOP] = bishop_attacks_bb(enemyKing, pieces());
        st->checkSquares[ROOK] = rook_attacks_bb(enemyKing, pieces());
        st->checkSquares[QUEEN] = st->checkSquares[BISHOP] | st->checkSquares[ROOK];
        st->checkSquares[KING] = 0;
    }
}

void Position::do_move(Move m, StateInfo& newSt, bool givesCheck)
{
    do_move(m, newSt);
}

void Position::undo_move(Move m)
{
    sideToMove = Color(~sideToMove);

    Square from = from_sq(m);
    Square to = to_sq(m);

    if (type_of(m) == NORMAL)
    {
        move_piece(to, from);

        if (st->capturedPiece != NO_PIECE)
            put_piece(st->capturedPiece, to);
    }
    else if (type_of(m) == PROMOTION)
    {
        remove_piece(to);
        put_piece(make_piece(sideToMove, PAWN), from);

        if (st->capturedPiece != NO_PIECE)
            put_piece(st->capturedPiece, to);
    }
    else if (type_of(m) == ENPASSANT)
    {
        move_piece(to, from);
        Square capturedSquare = Square(to - (sideToMove == WHITE ? 8 : -8));
        put_piece(st->capturedPiece, capturedSquare);
    }
    else if (type_of(m) == CASTLING)
    {
        Square rookFrom = castling_rook_square(to == SQ_G1 ? WHITE_OO :
            to == SQ_C1 ? WHITE_OOO :
            to == SQ_G8 ? BLACK_OO : BLACK_OOO);
        Square rookTo = to == SQ_G1 ? SQ_F1 : to == SQ_C1 ? SQ_D1 :
            to == SQ_G8 ? SQ_F8 : SQ_D8;

        move_piece(to, from);
        move_piece(rookTo, rookFrom);
    }

    st = st->previous;
    --gamePly;
}

void Position::do_null_move(StateInfo& newSt)
{
    newSt.previous = st;
    st = &newSt;
    sideToMove = Color(~sideToMove);
    st->epSquare = SQ_NONE;
    st->rule50 = 0;
    st->key = st->previous->key;
    st->checkersBB = 0ULL;
    if (count<KING>(WHITE) > 0 && count<KING>(BLACK) > 0)
        st->checkersBB = attackers_to(square<KING>(sideToMove)) & pieces(~sideToMove);
}

void Position::undo_null_move()
{
    sideToMove = ~sideToMove;
    st = st->previous;
}

void Position::do_castling(Color us, Square from, Square& to, Square& rfrom, Square& rto)
{
}