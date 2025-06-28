#include "bitboard.h"

U64 SquareBB[SQUARE_NB];
U64 FileBB[FILE_NB];
U64 RankBB[RANK_NB];
U64 AdjacentFilesBB[FILE_NB];
U64 ForwardRanksBB[COLOR_NB][RANK_NB];
U64 BetweenBB[SQUARE_NB][SQUARE_NB];
U64 LineBB[SQUARE_NB][SQUARE_NB];
U64 DistanceRingBB[SQUARE_NB][8];
U64 PseudoAttacks[PIECE_TYPE_NB][SQUARE_NB];
U64 PawnAttacks[COLOR_NB][SQUARE_NB];

U64 RookMasks[SQUARE_NB];
U64 BishopMasks[SQUARE_NB];
U64 RookMagics[SQUARE_NB];
U64 BishopMagics[SQUARE_NB];
U64* RookAttacks[SQUARE_NB];
U64* BishopAttacks[SQUARE_NB];
int RookShifts[SQUARE_NB];
int BishopShifts[SQUARE_NB];

namespace
{
    U64 RookTable[0x19000];
    U64 BishopTable[0x1480];

    enum Direction : int
    {
        NORTH = 8,
        EAST = 1,
        SOUTH = -NORTH,
        WEST = -EAST,
        NORTH_EAST = NORTH + EAST,
        SOUTH_EAST = SOUTH + EAST,
        SOUTH_WEST = SOUTH + WEST,
        NORTH_WEST = NORTH + WEST
    };

    U64 sliding_attack(Square sq, U64 occupied, Direction deltas[])
    {
        U64 attack = 0;

        for (int i = 0; i < 4; ++i)
            for (Square s = Square(sq + deltas[i]); is_ok(s) && distance(s, Square(s - deltas[i])) == 1; s = Square(s + deltas[i]))
            {
                attack |= square_bb(s);

                if (occupied & square_bb(s))
                    break;
            }

        return attack;
    }

    void init_magics(U64 table[], U64* attacks[], U64 magics[], U64 masks[], int shifts[], Direction deltas[], PieceType pt);

    U64 index_to_U64(int index, int bits, U64 m)
    {
        U64 result = 0ULL;
        for (int i = 0; i < bits; i++)
        {
            Square j = pop_lsb(m);
            if (index & (1 << i))
                result |= square_bb(j);
        }
        return result;
    }

    const U64 rook_magics[SQUARE_NB] = {
        0xa8002c000108020ULL, 0x4440200140003000ULL, 0x8080200010011880ULL, 0x380180080040800ULL,
        0x1A00060008040800ULL, 0x410040008200800ULL, 0x10c0100400100004ULL, 0x82040800401001ULL,
        0x40002080100100ULL, 0x4070c0040200300ULL, 0x40200080400080ULL, 0x2c00500800100200ULL,
        0x4200100080100080ULL, 0x200080100008080ULL, 0x4000100020004ULL, 0x8002000800084ULL,
        0x40004008000200ULL, 0x2000200040002000ULL, 0x80020004000800ULL, 0x800008001000ULL,
        0x800008001000ULL, 0x1000800008001000ULL, 0x400800100008001ULL, 0x2c00840080ULL,
        0x80002000400080ULL, 0x4002400208002000ULL, 0x2001002008002000ULL, 0x800800801000ULL,
        0x800800801000ULL, 0x1001002008001000ULL, 0x400880100008001ULL, 0x4200840080ULL,
        0x80004000200080ULL, 0x4002002000800ULL, 0x801002000800ULL, 0x800800801000ULL,
        0x800800801000ULL, 0x1001002008001000ULL, 0x400880100008001ULL, 0x4200840080ULL,
        0x80004000200080ULL, 0x4002002000800ULL, 0x801002000800ULL, 0x800800801000ULL,
        0x800800801000ULL, 0x1001002008001000ULL, 0x400880100008001ULL, 0x4200840080ULL,
        0x80002000008480ULL, 0x4002400020010ULL, 0x2001002008001ULL, 0x800800800801ULL,
        0x800800800801ULL, 0x1000800008001ULL, 0x400800100008ULL, 0x2001000840ULL,
        0x80004001000420ULL, 0x2001400200100ULL, 0x801000800804ULL, 0x800800800801ULL,
        0x800800800801ULL, 0x1000800008001ULL, 0x400800100008ULL, 0x201000840ULL
    };

    const U64 bishop_magics[SQUARE_NB] = {
        0x40040844404084ULL, 0x2004208a004208ULL, 0x10190041080202ULL, 0x108060845042010ULL,
        0x581104180800210ULL, 0x2112080446200010ULL, 0x1080820820060210ULL, 0x3c0808410220200ULL,
        0x4050404440404ULL, 0x21001420088ULL, 0x24d0080801082102ULL, 0x1020a0a020400ULL,
        0x40308200402ULL, 0x4011002100800ULL, 0x401484104104005ULL, 0x801010402020200ULL,
        0x400210c3880100ULL, 0x404022024108200ULL, 0x810018200204102ULL, 0x4002801a02003ULL,
        0x85040820080400ULL, 0x810102c808880400ULL, 0xe900410884800ULL, 0x8002020480840102ULL,
        0x220200865090201ULL, 0x2010100a02021202ULL, 0x152048408022401ULL, 0x20080002081110ULL,
        0x4001001021004000ULL, 0x800040400a011002ULL, 0xe4004081011002ULL, 0x1c004001012080ULL,
        0x8004200962a00220ULL, 0x8422100208500202ULL, 0x2000402200300c08ULL, 0x8646020080080080ULL,
        0x80020a0200100808ULL, 0x2010004880111000ULL, 0x623000a080011400ULL, 0x42008c0340209202ULL,
        0x209188240001000ULL, 0x400408a884001800ULL, 0x110400a6080400ULL, 0x1840060a44020800ULL,
        0x90080104000041ULL, 0x201011000808101ULL, 0x1a2208080504f080ULL, 0x8012020600211212ULL,
        0x500861011240000ULL, 0x180806108200800ULL, 0x4000020e01040044ULL, 0x300000261044000aULL,
        0x802241102020002ULL, 0x20906061210001ULL, 0x5a84841004010310ULL, 0x4010801011c04ULL,
        0xa010109502200ULL, 0x4a02012000ULL, 0x500201010098b028ULL, 0x8040002811040900ULL,
        0x28000010020204ULL, 0x6000020202d0240ULL, 0x8918844842082200ULL, 0x4010011029020020ULL
    };
}

U64 attacks_bb(PieceType pt, Square s, U64 occupied)
{
    switch (pt)
    {
    case BISHOP: return bishop_attacks_bb(s, occupied);
    case ROOK:   return rook_attacks_bb(s, occupied);
    case QUEEN:  return bishop_attacks_bb(s, occupied) | rook_attacks_bb(s, occupied);
    default:     return PseudoAttacks[pt][s];
    }
}

void init_bitboards()
{
    for (Square s = SQ_A1; s <= SQ_H8; s = Square(s + 1))
        SquareBB[s] = 1ULL << s;

    for (File f = FILE_A; f <= FILE_H; f = File(f + 1))
        FileBB[f] = f > FILE_A ? FileBB[f - 1] << 1 : 0x0101010101010101ULL;

    for (Rank r = RANK_1; r <= RANK_8; r = Rank(r + 1))
        RankBB[r] = 0xFFULL << (8 * r);

    for (File f = FILE_A; f <= FILE_H; f = File(f + 1))
        AdjacentFilesBB[f] = (f > FILE_A ? FileBB[f - 1] : 0) | (f < FILE_H ? FileBB[f + 1] : 0);

    for (Rank r = RANK_1; r < RANK_8; ++r)
    {
        ForwardRanksBB[WHITE][r] = 0ULL;
        ForwardRanksBB[BLACK][r + 1] = ForwardRanksBB[BLACK][r] | RankBB[r];
        ForwardRanksBB[WHITE][r] = ~ForwardRanksBB[BLACK][r + 1];
    }

    for (Square s1 = SQ_A1; s1 <= SQ_H8; ++s1)
        for (Square s2 = SQ_A1; s2 <= SQ_H8; ++s2)
            if (s1 != s2)
            {
                int dist = distance(s1, s2);
                DistanceRingBB[s1][dist] |= square_bb(s2);
            }

    int steps[][5] = { {}, { 7, 9 }, { 6, 10, 15, 17 }, {}, {}, {}, { 1, 7, 8, 9 } };

    for (Color c = WHITE; c <= BLACK; ++c)
        for (PieceType pt = PAWN; pt <= KING; ++pt)
            for (Square s = SQ_A1; s <= SQ_H8; s = Square(s + 1))
                for (int i = 0; steps[pt][i]; ++i)
                {
                    Square to = Square(s + (c == WHITE ? steps[pt][i] : -steps[pt][i]));

                    if (is_ok(to) && distance(s, to) < 3)
                    {
                        if (pt == PAWN)
                            PawnAttacks[c][s] |= square_bb(to);
                        else
                            PseudoAttacks[pt][s] |= square_bb(to);
                    }
                }

    Direction RookDeltas[] = { NORTH, EAST, SOUTH, WEST };
    Direction BishopDeltas[] = { NORTH_EAST, SOUTH_EAST, SOUTH_WEST, NORTH_WEST };

    init_magics(RookTable, RookAttacks, RookMagics, RookMasks, RookShifts, RookDeltas, ROOK);
    init_magics(BishopTable, BishopAttacks, BishopMagics, BishopMasks, BishopShifts, BishopDeltas, BISHOP);

    for (Square s1 = SQ_A1; s1 <= SQ_H8; s1 = Square(s1 + 1))
    {
        PseudoAttacks[QUEEN][s1] = PseudoAttacks[BISHOP][s1] = attacks_bb(BISHOP, s1, 0);
        PseudoAttacks[QUEEN][s1] |= PseudoAttacks[ROOK][s1] = attacks_bb(ROOK, s1, 0);

        for (PieceType pt = BISHOP; pt <= ROOK; pt = PieceType(pt + 1))
            for (Square s2 = SQ_A1; s2 <= SQ_H8; s2 = Square(s2 + 1))
            {
                if (PseudoAttacks[pt][s1] & square_bb(s2))
                {
                    LineBB[s1][s2] = (attacks_bb(pt, s1, 0) & attacks_bb(pt, s2, 0)) | square_bb(s1) | square_bb(s2);
                    BetweenBB[s1][s2] = attacks_bb(pt, s1, square_bb(s2)) & attacks_bb(pt, s2, square_bb(s1));
                }
            }
    }
}

namespace
{
    void init_magics(U64 table[], U64* attacks[], U64 magics[], U64 masks[], int shifts[], Direction deltas[], PieceType pt)
    {
        U64 occupancy[4096], reference[4096], edges, b;
        int size = 0;

        for (Square s = SQ_A1; s <= SQ_H8; ++s)
        {
            edges = ((RankBB[RANK_1] | RankBB[RANK_8]) & ~RankBB[rank_of(s)]) |
                ((FileBB[FILE_A] | FileBB[FILE_H]) & ~FileBB[file_of(s)]);

            masks[s] = sliding_attack(s, 0, deltas) & ~edges;
            shifts[s] = 64 - popcount(masks[s]);

            if (pt == ROOK)
                magics[s] = rook_magics[s];
            else
                magics[s] = bishop_magics[s];

            attacks[s] = s == SQ_A1 ? table : attacks[s - 1] + size;

            b = size = 0;
            do
            {
                occupancy[size] = b;
                reference[size] = sliding_attack(s, b, deltas);
                attacks[s][(b * magics[s]) >> shifts[s]] = reference[size];
                size++;
                b = (b - masks[s]) & masks[s];
            } while (b);
        }
    }
}