// nchess.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <ctime>
#include "../nchess/Board.h"
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include "../nchess/pgn.h"

int main()
{
    auto allGames = pgn::read_pgn_file("output_with_evals.pgn");

    SetConsoleOutputCP(CP_UTF8);


    srand(static_cast<unsigned int>(time(0)));
    BB::init();

    Board test;


    u64 test_board = 0;
    for (int i = 0; i < 15; i++) {
        test_board |= BB::set_bit(rand() % 64);
    }

    int test_space = rand() % 64;
    std::cout << BB::to_string(test_board) << "\n";
    std::cout << BB::to_string(BB::bishop_coverage[test_space]) << "\n";
    std::cout << BB::to_string(BB::bishop_coverage[test_space] & test_board) << "\n";

    std::cout << BB::to_string(BB::get_bishop_attacks(test_space, test_board)) << "\n";

    for (auto gameMove : allGames[0]) {
        //test.printBitBoards();
        test.printBoard();

        std::vector<Move> moves = test.getLegalMoves();

        for (auto move : moves) {
            std::cout << test.sanFromMove(move) << ", ";
        }
        std::cout << "\n";

        std::cout << "Move: " << gameMove.san << "\n";

        Move move = test.moveFromSan(gameMove.san);

        test.doMove(move);
    }

    test.printBoard();

    for (int i = 0; i < 40; i++) {
        test.undoMove();

        test.printBoard();
    }



}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu
