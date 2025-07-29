#ifndef BITBOARD_H
#define BITBOARD_H

#include "types.h"
#include <algorithm>
#include <intrin.h>

extern U64 SquareBB[SQUARE_NB];
extern U64 FileBB[FILE_NB];
extern U64 RankBB[RANK_NB];
extern U64 AdjacentFilesBB[FILE_NB];
extern U64 ForwardRanksBB[COLOR_NB][RANK_NB];
extern U64 BetweenBB[SQUARE_NB][SQUARE_NB];
extern U64 LineBB[SQUARE_NB][SQUARE_NB];
extern U64 DistanceRingBB[SQUARE_NB][8];
extern U64 PseudoAttacks[PIECE_TYPE_NB][SQUARE_NB];
extern U64 PawnAttacks[COLOR_NB][SQUARE_NB];

extern U64 RookMasks[SQUARE_NB];
extern U64 BishopMasks[SQUARE_NB];
extern U64 RookMagics[SQUARE_NB];
extern U64 BishopMagics[SQUARE_NB];
extern U64* RookAttacks[SQUARE_NB];
extern U64* BishopAttacks[SQUARE_NB];
extern int RookShifts[SQUARE_NB];
extern int BishopShifts[SQUARE_NB];

void init_bitboards();

inline U64 square_bb(Square s)
{
    return SquareBB[s];
}

inline U64 file_bb(File f)
{
    return FileBB[f];
}

inline U64 file_bb(Square s)
{
    return file_bb(file_of(s));
}

inline U64 rank_bb(Rank r)
{
    return RankBB[r];
}

inline U64 rank_bb(Square s)
{
    return rank_bb(rank_of(s));
}

inline bool more_than_one(U64 b)
{
    return b & (b - 1);
}

inline bool is_set(U64 b, Square s)
{
    return b & square_bb(s);
}

inline Square lsb(U64 b)
{
    unsigned long idx;
    _BitScanForward64(&idx, b);
    return Square(idx);
}

inline Square msb(U64 b)
{
    unsigned long idx;
    _BitScanReverse64(&idx, b);
    return Square(idx);
}

inline Square pop_lsb(U64& b)
{
    Square s = lsb(b);
    b &= b - 1;
    return s;
}

inline int popcount(U64 b)
{
    return int(__popcnt64(b));
}

inline U64 shift(U64 b, int delta)
{
    return delta > 0 ? b << delta : b >> -delta;
}

template<Color C>
inline U64 pawn_attacks_bb(U64 b)
{
    return C == WHITE ? ((b & ~file_bb(FILE_H)) << 9) | ((b & ~file_bb(FILE_A)) << 7)
        : ((b & ~file_bb(FILE_H)) >> 7) | ((b & ~file_bb(FILE_A)) >> 9);
}

inline U64 shift_delta(U64 b, int delta)
{
    return delta == 9 ? (b & ~file_bb(FILE_H)) << 9
        : delta == 7 ? (b & ~file_bb(FILE_A)) << 7
        : delta == -9 ? (b & ~file_bb(FILE_A)) >> 9
        : delta == -7 ? (b & ~file_bb(FILE_H)) >> 7
        : delta > 0 ? b << delta : b >> -delta;
}

U64 attacks_bb(PieceType pt, Square s, U64 occupied);

inline U64 attacks_bb(Piece pc, Square s, U64 occupied)
{
    return attacks_bb(type_of(pc), s, occupied);
}

inline U64 rook_attacks_bb(Square s, U64 occupied)
{
    if (!is_ok(s) || !RookAttacks[s]) return 0ULL;

    U64 mask = occupied & RookMasks[s];
    size_t index = (mask * RookMagics[s]) >> RookShifts[s];

    return RookAttacks[s][index];
}

inline U64 bishop_attacks_bb(Square s, U64 occupied)
{
    if (!is_ok(s) || !BishopAttacks[s]) return 0ULL;

    U64 mask = occupied & BishopMasks[s];
    size_t index = (mask * BishopMagics[s]) >> BishopShifts[s];

    return BishopAttacks[s][index];
}

inline U64 queen_attacks_bb(Square s, U64 occupied)
{
    return rook_attacks_bb(s, occupied) | bishop_attacks_bb(s, occupied);
}

inline U64 king_attacks_bb(Square s)
{
    return PseudoAttacks[KING][s];
}

inline U64 knight_attacks_bb(Square s)
{
    return PseudoAttacks[KNIGHT][s];
}

inline U64 pawn_attacks_bb(Color c, Square s)
{
    return PawnAttacks[c][s];
}

inline U64 between_bb(Square s1, Square s2)
{
    return BetweenBB[s1][s2];
}

inline U64 line_bb(Square s1, Square s2)
{
    return LineBB[s1][s2];
}

inline U64 adjacent_files_bb(File f)
{
    return AdjacentFilesBB[f];
}

inline U64 forward_ranks_bb(Color c, Rank r)
{
    return ForwardRanksBB[c][r];
}

inline U64 forward_ranks_bb(Color c, Square s)
{
    return forward_ranks_bb(c, rank_of(s));
}

inline U64 forward_file_bb(Color c, Square s)
{
    return forward_ranks_bb(c, s) & file_bb(s);
}

inline U64 pawn_attack_span(Color c, Square s)
{
    return forward_ranks_bb(c, s) & adjacent_files_bb(file_of(s));
}

inline U64 passed_pawn_span(Color c, Square s)
{
    return pawn_attack_span(c, s) | forward_file_bb(c, s);
}

inline bool aligned(Square s1, Square s2, Square s3)
{
    return line_bb(s1, s2) & square_bb(s3);
}

inline int distance(Square x, Square y)
{
    return std::max(abs(int(file_of(x)) - int(file_of(y))), abs(int(rank_of(x)) - int(rank_of(y))));
}

inline int distance(File x, File y)
{
    return abs(int(x) - int(y));
}

inline int distance(Rank x, Rank y)
{
    return abs(int(x) - int(y));
}

#endif