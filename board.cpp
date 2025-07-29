#include "board.h"
#include "bitboard.h"
#include <sstream>

void Position::init()
{
}

Position& Position::set(const std::string& fenStr, bool isChess960, StateInfo* si, Thread* th)
{
    sideToMove = WHITE;
    st = si;
    thisThread = th;
    chess960 = isChess960;
    gamePly = 0;

    for (Square s = SQ_A1; s <= SQ_H8; s = Square(s + 1))
        board[s] = NO_PIECE;

    for (PieceType pt = PAWN; pt <= KING; pt = PieceType(pt + 1))
        byTypeBB[pt] = 0ULL;

    for (Color c = WHITE; c <= BLACK; c = Color(c + 1))
        byColorBB[c] = 0ULL;

    for (int i = 0; i < PIECE_NB; ++i)
    {
        pieceCount[i] = 0;
        for (int j = 0; j < 16; ++j)
            pieceList[i][j] = SQ_NONE;
    }

    for (Square s = SQ_A1; s <= SQ_H8; s = Square(s + 1))
        index[s] = -1;

    for (Square s = SQ_A1; s <= SQ_H8; s = Square(s + 1))
        castlingRightsMask[s] = 0;

    for (int cr = 0; cr < CASTLING_RIGHT_NB; ++cr)
    {
        castlingRookSquare[cr] = SQ_NONE;
        castlingPath[cr] = 0ULL;
    }

    castlingRookSquare[WHITE_OO] = SQ_H1;
    castlingRookSquare[WHITE_OOO] = SQ_A1;
    castlingRookSquare[BLACK_OO] = SQ_H8;
    castlingRookSquare[BLACK_OOO] = SQ_A8;

    castlingPath[WHITE_OO] = square_bb(SQ_F1) | square_bb(SQ_G1);
    castlingPath[WHITE_OOO] = square_bb(SQ_B1) | square_bb(SQ_C1) | square_bb(SQ_D1);
    castlingPath[BLACK_OO] = square_bb(SQ_F8) | square_bb(SQ_G8);
    castlingPath[BLACK_OOO] = square_bb(SQ_B8) | square_bb(SQ_C8) | square_bb(SQ_D8);

    castlingRightsMask[SQ_A1] |= WHITE_OOO;
    castlingRightsMask[SQ_E1] |= WHITE_CASTLING;
    castlingRightsMask[SQ_H1] |= WHITE_OO;
    castlingRightsMask[SQ_A8] |= BLACK_OOO;
    castlingRightsMask[SQ_E8] |= BLACK_CASTLING;
    castlingRightsMask[SQ_H8] |= BLACK_OO;

    std::istringstream ss(fenStr);
    std::string token;

    ss >> token;

    Square sq = SQ_A8;
    for (char c : token)
    {
        if (isdigit(c))
            sq = Square(sq + (c - '0'));
        else if (c == '/')
            sq = Square(sq - 16);
        else
        {
            Piece pc = NO_PIECE;
            switch (tolower(c))
            {
            case 'p': pc = make_piece(islower(c) ? BLACK : WHITE, PAWN); break;
            case 'n': pc = make_piece(islower(c) ? BLACK : WHITE, KNIGHT); break;
            case 'b': pc = make_piece(islower(c) ? BLACK : WHITE, BISHOP); break;
            case 'r': pc = make_piece(islower(c) ? BLACK : WHITE, ROOK); break;
            case 'q': pc = make_piece(islower(c) ? BLACK : WHITE, QUEEN); break;
            case 'k': pc = make_piece(islower(c) ? BLACK : WHITE, KING); break;
            }

            if (pc != NO_PIECE)
            {
                put_piece(pc, sq);
                ++sq;
            }
        }
    }

    ss >> token;
    sideToMove = (token == "w") ? WHITE : BLACK;

    ss >> token;
    st->castlingRights = 0;
    for (char c : token)
    {
        switch (c)
        {
        case 'K': st->castlingRights |= WHITE_OO; break;
        case 'Q': st->castlingRights |= WHITE_OOO; break;
        case 'k': st->castlingRights |= BLACK_OO; break;
        case 'q': st->castlingRights |= BLACK_OOO; break;
        }
    }

    ss >> token;
    st->epSquare = (token == "-") ? SQ_NONE : Square((token[0] - 'a') + 8 * (token[1] - '1'));

    ss >> st->rule50 >> gamePly;

    st->key = 0;
    st->pawnKey = 0;
    st->materialKey = 0;
    st->psq = SCORE_ZERO;
    st->npMaterial[WHITE] = st->npMaterial[BLACK] = VALUE_ZERO;
    st->checkersBB = 0ULL;
    st->capturedPiece = NO_PIECE;
    st->previous = nullptr;

    for (PieceType pt = PAWN; pt <= KING; ++pt)
        st->checkSquares[pt] = 0;

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

    return *this;
}

const std::string Position::fen() const
{
    return "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
}

const std::string Position::pretty() const
{
    std::ostringstream ss;

    ss << "\n  +---+---+---+---+---+---+---+---+\n";

    for (int r = RANK_8; r >= RANK_1; r--)
    {
        ss << (r + 1) << " |";
        for (int f = FILE_A; f <= FILE_H; f++)
        {
            Piece pc = piece_on(make_square(File(f), Rank(r)));
            char symbol = ' ';

            if (pc != NO_PIECE)
            {
                char pieces[] = " pnbrqk";
                symbol = pieces[type_of(pc)];
                if (color_of(pc) == WHITE)
                    symbol = toupper(symbol);
            }

            ss << " " << symbol << " |";
        }
        ss << "\n  +---+---+---+---+---+---+---+---+\n";
    }

    ss << "    a   b   c   d   e   f   g   h\n";

    return ss.str();
}

bool Position::is_draw(int ply) const
{
    if (st->rule50 > 99)
        return true;

    if (ply > 4)
    {
        StateInfo* stp = st->previous;
        for (int i = 4; i <= ply && stp; i += 2)
        {
            stp = stp->previous;
            if (stp && stp->key == st->key)
                return true;
            if (stp) stp = stp->previous;
        }
    }

    return false;
}

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

void Position::do_castling(Color us, Square from, Square& to, Square& rfrom, Square& rto)
{
}

void Position::put_piece(Piece pc, Square s)
{
    if (pc == NO_PIECE || pc >= PIECE_NB || !is_ok(s))
        return;

    board[s] = pc;
    byTypeBB[type_of(pc)] |= square_bb(s);
    byColorBB[color_of(pc)] |= square_bb(s);

    if (pieceCount[pc] < 16)
    {
        pieceList[pc][pieceCount[pc]] = s;
        index[s] = pieceCount[pc];
        pieceCount[pc]++;
    }
}

void Position::remove_piece(Square s)
{
    if (!is_ok(s))
        return;

    Piece pc = board[s];
    if (pc == NO_PIECE || pc >= PIECE_NB)
        return;

    byTypeBB[type_of(pc)] ^= square_bb(s);
    byColorBB[color_of(pc)] ^= square_bb(s);

    board[s] = NO_PIECE;

    if (pieceCount[pc] > 0)
    {
        int pieceIndex = index[s];
        if (pieceIndex >= 0 && pieceIndex < pieceCount[pc])
        {
            pieceCount[pc]--;
            Square lastSquare = pieceList[pc][pieceCount[pc]];

            pieceList[pc][pieceIndex] = lastSquare;
            if (is_ok(lastSquare))
                index[lastSquare] = pieceIndex;
            pieceList[pc][pieceCount[pc]] = SQ_NONE;
        }
    }
}

void Position::move_piece(Square from, Square to)
{
    Piece pc = board[from];
    remove_piece(from);
    put_piece(pc, to);
}

Bitboard Position::slider_attacks(Bitboard occupied) const
{
    Bitboard result = 0ULL;

    for (Square s = SQ_A1; s <= SQ_H8; s = Square(s + 1))
    {
        Piece pc = piece_on(s);
        if (pc == NO_PIECE)
            continue;

        PieceType pt = type_of(pc);
        if (pt == ROOK || pt == QUEEN)
            result |= rook_attacks_bb(s, occupied);
        if (pt == BISHOP || pt == QUEEN)
            result |= bishop_attacks_bb(s, occupied);
    }

    return result;
}

Bitboard Position::blockers_for_king(Color c) const
{
    return 0ULL;
}

Bitboard Position::check_squares(PieceType pt) const
{
    return st->checkSquares[pt];
}

Bitboard Position::pinned_pieces(Color c) const
{
    return 0ULL;
}

Bitboard Position::discovered_check_candidates() const
{
    return 0ULL;
}

Bitboard Position::attackers_to(Square s) const
{
    return attackers_to(s, pieces());
}

Bitboard Position::attackers_to(Square s, Bitboard occupied) const
{
    if (!is_ok(s)) return 0ULL;

    Bitboard attackers = 0ULL;

    Bitboard pawns = pieces(PAWN) & occupied;
    attackers |= pawn_attacks_bb(WHITE, s) & (pawns & pieces(BLACK));
    attackers |= pawn_attacks_bb(BLACK, s) & (pawns & pieces(WHITE));

    Bitboard knights = pieces(KNIGHT) & occupied;
    attackers |= knight_attacks_bb(s) & knights;

    Bitboard kings = pieces(KING) & occupied;
    attackers |= king_attacks_bb(s) & kings;

    Bitboard rooksQueens = (pieces(ROOK) | pieces(QUEEN)) & occupied;
    if (rooksQueens)
        attackers |= rook_attacks_bb(s, occupied) & rooksQueens;

    Bitboard bishopsQueens = (pieces(BISHOP) | pieces(QUEEN)) & occupied;
    if (bishopsQueens)
        attackers |= bishop_attacks_bb(s, occupied) & bishopsQueens;

    return attackers;
}

Key Position::key_after(Move m) const
{
    return 0ULL;
}

uint64_t Position::nodes_searched() const
{
    return thisThread ? thisThread->nodes : 0;
}

void Position::set_nodes_searched(uint64_t n)
{
    if (thisThread)
        thisThread->nodes = n;
}

bool Position::has_game_cycle(int ply) const
{
    return false;
}

bool Position::has_repeated() const
{
    return false;
}

int Position::see_ge(Move m, Value threshold) const
{
    return 0;
}

int Position::see(Move m) const
{
    return 0;
}

bool Position::advanced_pawn_push(Move m) const
{
    return false;
}

bool Position::passed_pawn_push(Move m) const
{
    return false;
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

Position& Position::set(const std::string& code, Color c, StateInfo* si)
{
    return *this;
}

void Position::set_castling_right(Color c, Square rfrom)
{
}

void Position::set_state(StateInfo* si) const
{
}

void Position::set_check_info(StateInfo* si) const
{
}

Bitboard Position::slider_blockers(Bitboard sliders, Square s, Bitboard& pinners) const
{
    return 0ULL;
}

Key Position::key() const
{
    Key k = 0;

    for (Square s = SQ_A1; s <= SQ_H8; s = Square(s + 1))
    {
        Piece pc = piece_on(s);
        if (pc != NO_PIECE)
        {
            k ^= U64(pc) * U64(s + 1) * 0x9E3779B97F4A7C15ULL;
        }
    }

    if (sideToMove == BLACK)
        k ^= 0x123456789ABCDEFULL;

    k ^= U64(st->castlingRights) * 0x517CC1B727220A95ULL;

    if (st->epSquare != SQ_NONE)
        k ^= U64(st->epSquare) * 0x6C5D5C6B5AED4A2BULL;

    return k;
}

int Position::game_ply() const
{
    return gamePly;
}

std::ostream& operator<<(std::ostream& os, const Position& pos)
{
    os << pos.pretty();
    return os;
}