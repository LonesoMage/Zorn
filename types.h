#ifndef TYPES_H
#define TYPES_H

#include <cstdint>
#include <cassert>
#include <cstdlib>

typedef uint64_t U64;
typedef uint32_t U32;
typedef uint16_t U16;
typedef uint8_t U8;

typedef int32_t Score;
typedef int16_t Value;
typedef int8_t Depth;

typedef U64 Bitboard;
typedef U64 Key;

enum Color : U8
{
    WHITE = 0,
    BLACK = 1,
    COLOR_NB = 2
};

enum PieceType : U8
{
    NO_PIECE_TYPE = 0,
    PAWN = 1,
    KNIGHT = 2,
    BISHOP = 3,
    ROOK = 4,
    QUEEN = 5,
    KING = 6,
    PIECE_TYPE_NB = 7
};

enum Piece : U8
{
    NO_PIECE = 0,
    W_PAWN = 1, W_KNIGHT = 2, W_BISHOP = 3, W_ROOK = 4, W_QUEEN = 5, W_KING = 6,
    B_PAWN = 9, B_KNIGHT = 10, B_BISHOP = 11, B_ROOK = 12, B_QUEEN = 13, B_KING = 14,
    PIECE_NB = 16
};

enum Square : U8
{
    SQ_A1 = 0, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
    SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
    SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
    SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
    SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
    SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6,
    SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
    SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8,
    SQ_NONE = 64,
    SQUARE_NB = 64
};

enum Rank : U8
{
    RANK_1 = 0, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8,
    RANK_NB = 8
};

enum File : U8
{
    FILE_A = 0, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H,
    FILE_NB = 8
};

enum CastlingRights : U8
{
    NO_CASTLING = 0,
    WHITE_OO = 1,
    WHITE_OOO = 2,
    BLACK_OO = 4,
    BLACK_OOO = 8,
    WHITE_CASTLING = WHITE_OO | WHITE_OOO,
    BLACK_CASTLING = BLACK_OO | BLACK_OOO,
    ANY_CASTLING = WHITE_CASTLING | BLACK_CASTLING,
    CASTLING_RIGHT_NB = 16
};

enum MoveType : U8
{
    NORMAL = 0,
    PROMOTION = 1,
    ENPASSANT = 2,
    CASTLING = 3
};

enum GenType : U8
{
    CAPTURES,
    QUIETS,
    QUIET_CHECKS,
    EVASIONS,
    NON_EVASIONS,
    LEGAL
};

typedef U16 Move;

constexpr Move MOVE_NONE = 0;
constexpr Move MOVE_NULL = 65;

constexpr Value VALUE_ZERO = 0;
constexpr Value VALUE_DRAW = 0;
constexpr Value VALUE_MATE = 32000;
constexpr Value VALUE_INFINITE = 32001;
constexpr Value VALUE_NONE = 32002;

constexpr Depth MAX_PLY = 128;
constexpr Depth DEPTH_ZERO = 0;
constexpr Depth DEPTH_MAX = 127;

constexpr Score SCORE_ZERO = 0;

constexpr U64 EMPTY_BB = 0ULL;
constexpr U64 ALL_SQUARES_BB = ~EMPTY_BB;

inline Color& operator++(Color& c) { return c = Color(int(c) + 1); }
inline Color& operator--(Color& c) { return c = Color(int(c) - 1); }
inline PieceType& operator++(PieceType& pt) { return pt = PieceType(int(pt) + 1); }
inline PieceType& operator--(PieceType& pt) { return pt = PieceType(int(pt) - 1); }
inline Piece& operator++(Piece& pc) { return pc = Piece(int(pc) + 1); }
inline Piece& operator--(Piece& pc) { return pc = Piece(int(pc) - 1); }
inline Square& operator++(Square& s) { return s = Square(int(s) + 1); }
inline Square& operator--(Square& s) { return s = Square(int(s) - 1); }
inline File& operator++(File& f) { return f = File(int(f) + 1); }
inline File& operator--(File& f) { return f = File(int(f) - 1); }
inline Rank& operator++(Rank& r) { return r = Rank(int(r) + 1); }
inline Rank& operator--(Rank& r) { return r = Rank(int(r) - 1); }

inline Color operator~(Color c)
{
    return Color(c ^ BLACK);
}

inline Square make_square(File f, Rank r)
{
    return Square((r << 3) + f);
}

inline File file_of(Square s)
{
    return File(s & 7);
}

inline Rank rank_of(Square s)
{
    return Rank(s >> 3);
}

inline Piece make_piece(Color c, PieceType pt)
{
    return Piece((c << 3) + pt);
}

inline PieceType type_of(Piece pc)
{
    return PieceType(pc & 7);
}

inline Color color_of(Piece pc)
{
    return Color(pc >> 3);
}

inline Square relative_square(Color c, Square s)
{
    return Square(s ^ (c * 56));
}

inline Rank relative_rank(Color c, Rank r)
{
    return Rank(r ^ (c * 7));
}

inline Rank relative_rank(Color c, Square s)
{
    return relative_rank(c, rank_of(s));
}

inline bool opposite_colors(Square s1, Square s2)
{
    return bool(((file_of(s1) + rank_of(s1)) ^ (file_of(s2) + rank_of(s2))) & 1);
}

inline bool is_ok(Square s)
{
    return s >= SQ_A1 && s <= SQ_H8;
}

inline Move make_move(Square from, Square to)
{
    return Move(to | (from << 6));
}

template<MoveType T>
inline Move make(Square from, Square to, PieceType pt = KNIGHT)
{
    return T == NORMAL ? make_move(from, to) :
        T == PROMOTION ? Move(to | (from << 6) | (T << 12) | ((pt - KNIGHT) << 14)) :
        Move(to | (from << 6) | (T << 12));
}

inline Square from_sq(Move m)
{
    return Square((m >> 6) & 0x3F);
}

inline Square to_sq(Move m)
{
    return Square(m & 0x3F);
}

inline MoveType type_of(Move m)
{
    return MoveType(m >> 12);
}

inline PieceType promotion_type(Move m)
{
    return PieceType(((m >> 14) & 3) + KNIGHT);
}

inline bool is_ok(Move m)
{
    return from_sq(m) != to_sq(m);
}

#endif