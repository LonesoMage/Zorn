#include "uci.h"
#include "board.h"
#include "search.h"
#include "move.h"
#include <iostream>
#include <sstream>
#include <string>

using namespace std;

namespace UCI
{
    void init()
    {
        Option::init();
    }

    void loop(int argc, char* argv[])
    {
        Position pos;
        string token, cmd;
        StateInfo si;

        pos.set("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", false, &si, nullptr);

        for (int i = 1; i < argc; ++i)
            cmd += string(argv[i]) + " ";

        do
        {
            if (argc == 1 && !getline(cin, cmd))
                cmd = "quit";

            istringstream is(cmd);

            token.clear();
            is >> skipws >> token;

            if (token == "quit" || token == "stop")
                break;

            else if (token == "uci")
            {
                cout << "id name Zorn 1.0" << endl;
                cout << "id author Zorn Team" << endl;
                cout << "uciok" << endl;
            }

            else if (token == "isready")
                cout << "readyok" << endl;

            else if (token == "ucinewgame")
            {
                pos.set("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", false, &si, nullptr);
            }

            else if (token == "position")
            {
                string fen;

                is >> token;

                if (token == "startpos")
                {
                    fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
                    is >> token;
                }
                else if (token == "fen")
                {
                    while (is >> token && token != "moves")
                        fen += token + " ";
                }

                pos.set(fen, false, &si, nullptr);

                while (is >> token)
                {
                    Move m = UCI::to_move(pos, token);
                    if (m == MOVE_NONE)
                        break;

                    StateInfo newSi;
                    pos.do_move(m, newSi);
                }
            }

            else if (token == "go")
            {
                Search::Limits limits;
                limits.depth = DEPTH_MAX;

                while (is >> token)
                {
                    if (token == "depth")
                        is >> limits.depth;
                    else if (token == "movetime")
                        is >> limits.time[WHITE];
                    else if (token == "wtime")
                        is >> limits.time[WHITE];
                    else if (token == "btime")
                        is >> limits.time[BLACK];
                    else if (token == "winc")
                        is >> limits.inc[WHITE];
                    else if (token == "binc")
                        is >> limits.inc[BLACK];
                    else if (token == "movestogo")
                        is >> limits.movestogo;
                }

                Search::start(pos, limits);
            }

            else if (token == "d")
                cout << pos << endl;

        } while (token != "quit" && argc == 1);
    }

    string value(Value v)
    {
        assert(-VALUE_INFINITE < v && v < VALUE_INFINITE);

        stringstream ss;

        if (abs(v) < VALUE_MATE - MAX_PLY)
            ss << "cp " << v * 100 / 208;
        else
            ss << "mate " << (v > 0 ? VALUE_MATE - v + 1 : -VALUE_MATE - v) / 2;

        return ss.str();
    }

    string square(Square s)
    {
        return string{ char('a' + file_of(s)), char('1' + rank_of(s)) };
    }

    string move(Move m, bool chess960)
    {
        Square from = from_sq(m);
        Square to = to_sq(m);

        if (m == MOVE_NONE)
            return "(none)";

        if (m == MOVE_NULL)
            return "0000";

        string move = square(from) + square(to);

        if (type_of(m) == PROMOTION)
            move += " pnbrqk"[promotion_type(m)];

        return move;
    }

    Move to_move(const Position& pos, string& str)
    {
        if (str.length() == 5)
            str[4] = char(tolower(str[4]));

        for (const auto& m : MoveList(pos))
            if (str == move(m, pos.is_chess960()))
                return m;

        return MOVE_NONE;
    }
}

namespace Option
{
    void init()
    {
    }
}