#include "uci.h"
#include "board.h"
#include "search.h"
#include "move.h"
#include "eval.h"
#include <iostream>
#include <sstream>
#include <string>
#include <chrono>

using namespace std;

static uint64_t perft(Position& pos, int depth)
{
    if (depth == 0) return 1;

    uint64_t nodes = 0;

    for (const auto& move : MoveList(pos))
    {
        if (pos.legal(move))
        {
            StateInfo st;
            pos.do_move(move, st);
            nodes += perft(pos, depth - 1);
            pos.undo_move(move);
        }
    }

    return nodes;
}

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
        static StateInfo setupStates[1000];
        int stateIndex = 0;

        pos.set("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", false, &setupStates[0], nullptr);

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
                stateIndex = 0;
                pos.set("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", false, &setupStates[0], nullptr);
            }

            else if (token == "position")
            {
                string fen;
                stateIndex = 0;

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

                pos.set(fen, false, &setupStates[stateIndex++], nullptr);

                if (token == "moves")
                {
                    while (is >> token)
                    {
                        Move m = UCI::to_move(pos, token);
                        if (m == MOVE_NONE)
                        {
                            cout << "info string Invalid move: " << token << endl;
                            break;
                        }

                        if (!pos.legal(m))
                        {
                            cout << "info string Illegal move: " << token << endl;
                            break;
                        }

                        pos.do_move(m, setupStates[stateIndex++]);
                    }
                }
            }

            else if (token == "go")
            {
                Search::Limits limits;
                limits.depth = 8;
                limits.movetime = 0;

                while (is >> token)
                {
                    if (token == "depth")
                        is >> limits.depth;
                    else if (token == "movetime")
                        is >> limits.movetime;
                    else if (token == "wtime")
                    {
                        is >> limits.time[WHITE];
                    }
                    else if (token == "btime")
                    {
                        is >> limits.time[BLACK];
                    }
                    else if (token == "winc")
                        is >> limits.inc[WHITE];
                    else if (token == "binc")
                        is >> limits.inc[BLACK];
                    else if (token == "movestogo")
                        is >> limits.movestogo;
                    else if (token == "infinite")
                        limits.infinite = 1;
                    else if (token == "nodes")
                        is >> limits.nodes;
                    else if (token == "mate")
                        is >> limits.mate;
                }

                if (limits.movetime == 0 && limits.infinite == 0 && limits.time[pos.side_to_move()] > 0)
                {
                    int timeLeft = limits.time[pos.side_to_move()];
                    int increment = limits.inc[pos.side_to_move()];
                    int movesToGo = limits.movestogo > 0 ? limits.movestogo : 40;

                    limits.movetime = (timeLeft / movesToGo) + (increment * 4 / 5);
                    limits.movetime = std::max(10, std::min(limits.movetime, timeLeft / 3));

                    if (timeLeft < 1000)
                        limits.movetime = std::max(10, increment / 2);
                }

                Search::start(pos, limits);
            }

            else if (token == "d")
                cout << pos << endl;

            else if (token == "moves")
            {
                cout << "Legal moves: ";
                MoveList moveList(pos);
                int count = 0;

                for (const auto& m : moveList)
                {
                    if (pos.legal(m))
                    {
                        count++;
                        cout << move_to_uci(m) << " ";
                    }
                }

                if (count == 0)
                    cout << "none";
                cout << endl;
            }

            else if (token == "eval")
            {
                Value eval = Eval::evaluate(pos);
                cout << "Static evaluation: " << eval << " (cp)" << endl;
                cout << "From WHITE perspective (positive = good for White)" << endl;
            }

            else if (token == "analyze")
            {
                cout << "=== MOVE ANALYSIS ===" << endl;

                string testMoves[] = { "e2e4", "d2d4", "g1f3", "b1c3", "b1a3" };

                for (const string& moveStr : testMoves)
                {
                    string moveCopy = moveStr;
                    Move m = UCI::to_move(pos, moveCopy);

                    if (m != MOVE_NONE && pos.legal(m))
                    {
                        StateInfo st;
                        pos.do_move(m, st);
                        Value eval = Eval::evaluate(pos);
                        pos.undo_move(m);

                        cout << moveStr << ": " << eval << " cp" << endl;
                    }
                }
                cout << "=====================" << endl;
            }

            else if (token == "perft")
            {
                int depth = 4;
                if (is >> depth)
                {
                    auto start = chrono::steady_clock::now();
                    uint64_t nodes = perft(pos, depth);
                    auto end = chrono::steady_clock::now();
                    auto elapsed = chrono::duration_cast<chrono::milliseconds>(end - start).count();

                    cout << "Perft " << depth << ": " << nodes << " nodes in " << elapsed << "ms";
                    if (elapsed > 0)
                        cout << " (" << (nodes * 1000 / elapsed) << " nps)";
                    cout << endl;
                }
            }

            else if (token == "divide")
            {
                int depth = 1;
                if (is >> depth && depth > 0)
                {
                    cout << "Divide " << depth << ":" << endl;

                    uint64_t totalNodes = 0;
                    for (const auto& move : MoveList(pos))
                    {
                        if (pos.legal(move))
                        {
                            StateInfo st;
                            pos.do_move(move, st);
                            uint64_t nodes = depth == 1 ? 1 : perft(pos, depth - 1);
                            pos.undo_move(move);

                            cout << move_to_uci(move) << ": " << nodes << endl;
                            totalNodes += nodes;
                        }
                    }
                    cout << "\nTotal: " << totalNodes << " nodes" << endl;
                }
            }

            else if (token == "test")
            {
                cout << "Testing move generation:" << endl;
                int count = 0;
                for (const auto& move : MoveList(pos))
                {
                    if (pos.legal(move))
                    {
                        count++;
                        cout << count << ". " << move_to_uci(move) << endl;
                        if (count >= 5) break;
                    }
                }
                cout << "Total legal moves: " << count << endl;
            }

            else if (token == "compare")
            {
                cout << "=== COMPARING OPENING MOVES ===" << endl;

                struct MoveEval {
                    string move;
                    Value staticEval;
                    Value searchEval;
                };

                vector<MoveEval> moveEvals;
                string testMoves[] = { "e2e4", "d2d4", "g1f3", "b1c3", "b1a3" };

                for (const string& moveStr : testMoves)
                {
                    string moveCopy = moveStr;
                    Move m = UCI::to_move(pos, moveCopy);

                    if (m != MOVE_NONE && pos.legal(m))
                    {
                        StateInfo st;
                        pos.do_move(m, st);

                        Value staticEval = Eval::evaluate(pos);

                        Search::Limits limits;
                        limits.depth = 6;
                        limits.movetime = 1000;

                        cout.setstate(ios_base::failbit);
                        Search::start(pos, limits);
                        cout.clear();

                        Value searchEval = staticEval;

                        pos.undo_move(m);

                        moveEvals.push_back({ moveStr, staticEval, searchEval });
                    }
                }

                cout << "Move     | Static | Search" << endl;
                cout << "---------|--------|-------" << endl;
                for (const auto& me : moveEvals)
                {
                    cout << me.move << "   | " << me.staticEval << "    | " << me.searchEval << endl;
                }
                cout << "================================" << endl;
            }

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
        return move_to_uci(m);
    }

    Move to_move(const Position& pos, string& str)
    {
        if (str.length() == 5)
            str[4] = char(tolower(str[4]));

        if (str.length() < 4)
            return MOVE_NONE;

        for (const auto& m : MoveList(pos))
            if (str == move_to_uci(m) && pos.legal(m))
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