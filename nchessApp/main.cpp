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
#include "../nchess/UCI.h"

int main()
{

    BB::init();
    UCI uci;
    Board b;

    uci.loop();
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu
