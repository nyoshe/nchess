// nchess.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <ctime>
#include "../nchess/Engine.h"
#include "../nchess/Board.h"
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include "../nchess/pgn.h"

int main()
{
    /*
    std::ifstream file("output_with_evals.pgn");
    if (!file) {
        throw std::invalid_argument("invalid file");
    }
    auto allGames = pgn::read_pgn_file(file);
	*/
    BB::init();
    SetConsoleOutputCP(CP_UTF8);

    Engine engine;
    int depth = 5;


    std::vector<PerfT> results = engine.doPerftSearch("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - ", depth);

    for (int i = 0; i < depth; i++) {
        std::cout << "results: " << results[i].to_string() << "\n\n";
    }


    srand(static_cast<unsigned int>(time(0)));
    
    Board test;
	test.loadFen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - ");
    test.printBitBoards();
    test.printBoard();

}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu
