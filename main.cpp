#include <iostream>
#include "bitboard.h"
#include "board.h"
#include "uci.h"
#include "search.h"
#include "eval.h"

int main(int argc, char* argv[])
{
    std::cout << "Zorn " << "1.0" << " by Zorn Team" << std::endl;

    init_bitboards();
    Position::init();
    Search::init();
    Eval::init();
    UCI::init();

    UCI::loop(argc, argv);

    return 0;
}