#ifndef UCI_H
#define UCI_H

#include "types.h"
#include <string>

class Position;

namespace UCI
{
    void init();
    void loop(int argc, char* argv[]);
    std::string value(Value v);
    std::string square(Square s);
    std::string move(Move m, bool chess960);
    Move to_move(const Position& pos, std::string& str);
}

namespace Option
{
    void init();
}

#endif