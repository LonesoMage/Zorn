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