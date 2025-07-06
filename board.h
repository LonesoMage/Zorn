#ifndef BOARD_H
#define BOARD_H

#include "types.h"
#include "bitboard.h"
#include "move.h"
#include <string>
#include <vector>
#include <iostream>

struct StateInfo
{
    Key key;
    Key pawnKey;
    Key materialKey;
    Value npMaterial[COLOR_NB];
    int castlingRights;
    int rule50;
    int pliesFromNull;
    Square epSquare;

    Score psq;
    Piece capturedPiece;

    Bitboard checkersBB;
    Bitboard checkSquares[PIECE_TYPE_NB];

    StateInfo* previous;
};

#include "search.h"

class Position
{
private:
    Piece board[SQUARE_NB];
    U64 byTypeBB[PIECE_TYPE_NB];
    U64 byColorBB[COLOR_NB];
    int pieceCount[PIECE_NB];
    Square pieceList[PIECE_NB][16];
    int index[SQUARE_NB];
    int castlingRightsMask[SQUARE_NB];
    Square castlingRookSquare[CASTLING_RIGHT_NB];
    Bitboard castlingPath[CASTLING_RIGHT_NB];
    int gamePly;
    Color sideToMove;
    Thread* thisThread;
    StateInfo* st;
    bool chess960;

public:
    static void init();

    Position() = default;
    Position(const Position&) = delete;
    Position& operator=(const Position&) = delete;

    Position& set(const std::string& fenStr, bool isChess960, StateInfo* si, Thread* th);
    Position& set(const std::string& code, Color c, StateInfo* si);
    const std::string fen() const;
    const std::string pretty() const;

    Bitboard pieces() const;
    Bitboard pieces(PieceType pt) const;
    Bitboard pieces(PieceType pt1, PieceType pt2) const;
    Bitboard pieces(Color c) const;
    Bitboard pieces(Color c, PieceType pt) const;
    Bitboard pieces(Color c, PieceType pt1, PieceType pt2) const;
    Piece piece_on(Square s) const;
    Square ep_square() const;
    bool empty(Square s) const;
    Color side_to_move() const;
    template<PieceType Pt> int count(Color c) const;
    template<PieceType Pt> const Square* squares(Color c) const;
    template<PieceType Pt> Square square(Color c) const;
    bool can_castle(CastlingRights cr) const;
    bool castling_impeded(CastlingRights cr) const;
    Square castling_rook_square(CastlingRights cr) const;

    Bitboard checkers() const;
    Bitboard blockers_for_king(Color c) const;
    Bitboard check_squares(PieceType pt) const;
    Bitboard pinned_pieces(Color c) const;
    Bitboard discovered_check_candidates() const;

    bool legal(Move m) const;
    bool pseudo_legal(const Move m) const;
    bool gives_check(Move m) const;
    Piece moved_piece(Move m) const;
    Piece captured_piece() const;

    void do_move(Move m, StateInfo& newSt);
    void do_move(Move m, StateInfo& newSt, bool givesCheck);
    void undo_move(Move m);
    void do_null_move(StateInfo& newSt);
    void undo_null_move();

    Key key() const;
    Key key_after(Move m) const;
    Key material_key() const;
    Key pawn_key() const;
    int game_ply() const;

    Value psq_score() const;
    Value non_pawn_material(Color c) const;
    Value non_pawn_material() const;

    int rule50_count() const;
    bool opposite_bishops() const;
    bool is_chess960() const;
    Thread* this_thread() const;

    uint64_t nodes_searched() const;
    void set_nodes_searched(uint64_t n);

    bool is_draw(int ply) const;
    bool has_game_cycle(int ply) const;
    bool has_repeated() const;

    int see_ge(Move m, Value threshold = VALUE_ZERO) const;
    int see(Move m) const;

    bool advanced_pawn_push(Move m) const;
    bool passed_pawn_push(Move m) const;

    void put_piece(Piece pc, Square s);
    void remove_piece(Square s);
    void move_piece(Square from, Square to);

    void do_castling(Color us, Square from, Square& to, Square& rfrom, Square& rto);

    Bitboard slider_attacks(Bitboard occupied) const;

private:
    void set_castling_right(Color c, Square rfrom);
    void set_state(StateInfo* si) const;
    void set_check_info(StateInfo* si) const;

    Bitboard slider_blockers(Bitboard sliders, Square s, Bitboard& pinners) const;
    Bitboard attackers_to(Square s) const;
    Bitboard attackers_to(Square s, Bitboard occupied) const;

    template<PieceType Pt>
    Bitboard attacks_from(Square s) const;

    template<PieceType Pt>
    Bitboard attacks_from(Square s, Color c) const;

    Bitboard attacks_from(Piece pc, Square s) const;
};

extern std::ostream& operator<<(std::ostream& os, const Position& pos);

inline Bitboard Position::pieces() const
{
    return byColorBB[WHITE] | byColorBB[BLACK];
}

inline Bitboard Position::pieces(PieceType pt) const
{
    return byTypeBB[pt];
}

inline Bitboard Position::pieces(PieceType pt1, PieceType pt2) const
{
    return byTypeBB[pt1] | byTypeBB[pt2];
}

inline Bitboard Position::pieces(Color c) const
{
    return byColorBB[c];
}

inline Bitboard Position::pieces(Color c, PieceType pt) const
{
    return byColorBB[c] & byTypeBB[pt];
}

inline Bitboard Position::pieces(Color c, PieceType pt1, PieceType pt2) const
{
    return byColorBB[c] & (byTypeBB[pt1] | byTypeBB[pt2]);
}

template<PieceType Pt>
inline int Position::count(Color c) const
{
    return pieceCount[make_piece(c, Pt)];
}

template<PieceType Pt>
inline const Square* Position::squares(Color c) const
{
    return pieceList[make_piece(c, Pt)];
}

template<PieceType Pt>
inline Square Position::square(Color c) const
{
    return pieceList[make_piece(c, Pt)][0];
}

inline Piece Position::piece_on(Square s) const
{
    return board[s];
}

inline bool Position::empty(Square s) const
{
    return board[s] == NO_PIECE;
}

inline Color Position::side_to_move() const
{
    return sideToMove;
}

inline Piece Position::moved_piece(Move m) const
{
    return board[from_sq(m)];
}

inline Bitboard Position::checkers() const
{
    return st->checkersBB;
}

inline bool Position::gives_check(Move m) const
{
    return st->checkSquares[type_of(piece_on(from_sq(m)))] & to_sq(m)
        || (discovered_check_candidates() & from_sq(m) && !aligned(from_sq(m), to_sq(m), square<KING>(~sideToMove)));
}

inline Key Position::pawn_key() const
{
    return st->pawnKey;
}

inline Key Position::material_key() const
{
    return st->materialKey;
}

inline Value Position::psq_score() const
{
    return Value(st->psq);
}

inline Value Position::non_pawn_material(Color c) const
{
    return st->npMaterial[c];
}

inline Value Position::non_pawn_material() const
{
    return st->npMaterial[WHITE] + st->npMaterial[BLACK];
}

inline int Position::rule50_count() const
{
    return st->rule50;
}

inline bool Position::opposite_bishops() const
{
    return count<BISHOP>(WHITE) == 1
        && count<BISHOP>(BLACK) == 1
        && opposite_colors(square<BISHOP>(WHITE), square<BISHOP>(BLACK));
}

inline bool Position::is_chess960() const
{
    return chess960;
}

inline bool Position::can_castle(CastlingRights cr) const
{
    return st->castlingRights & cr;
}

inline bool Position::castling_impeded(CastlingRights cr) const
{
    return pieces() & castlingPath[cr];
}

inline Square Position::castling_rook_square(CastlingRights cr) const
{
    return castlingRookSquare[cr];
}

inline Bitboard Position::attacks_from(Piece pc, Square s) const
{
    return attacks_bb(pc, s, pieces());
}

template<PieceType Pt>
inline Bitboard Position::attacks_from(Square s) const
{
    return Pt == BISHOP || Pt == ROOK ? attacks_bb(Pt, s, pieces())
        : Pt == QUEEN ? attacks_from<ROOK>(s) | attacks_from<BISHOP>(s)
        : PseudoAttacks[Pt][s];
}

template<PieceType Pt>
inline Bitboard Position::attacks_from(Square s, Color c) const
{
    assert(Pt == PAWN);
    return PawnAttacks[c][s];
}

inline Square Position::ep_square() const
{
    return st->epSquare;
}

inline Piece Position::captured_piece() const
{
    return st->capturedPiece;
}

inline Thread* Position::this_thread() const
{
    return thisThread;
}

#endif