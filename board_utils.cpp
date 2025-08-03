#include "board.h"
#include "bitboard.h"

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

Bitboard Position::slider_blockers(Bitboard sliders, Square s, Bitboard& pinners) const
{
    return 0ULL;
}