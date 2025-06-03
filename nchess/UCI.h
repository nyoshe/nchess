#pragma once

#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>
#include "Engine.h"

class UCI
{
public:
    UCI()
        : engine_(), debug_mode_(false)
    {
    }

    void setupBoard(std::istringstream& iss) {
        std::string token;
        engine_.b.reset();
        iss >> token;
        if (token == "fen") {
            engine_.setBoardFEN(iss);
        }
        engine_.setBoardUCI(iss);
    }

    void loop()
    {
        std::string line;
        while (std::getline(std::cin, line))
        {
            std::istringstream iss(line);
            std::string token;
            iss >> token;

            if (token == "uci")
            {
                sendId();
                sendOptions();
                std::cout << "uciok" << std::endl;
            }
            else if (token == "isready")
            {
                std::cout << "readyok" << std::endl;
            }
            else if (token == "setoption")
            {
                // Option parsing can be implemented here
            }
            else if (token == "ucinewgame")
            {
                engine_ = Engine();
            }
            else if (token == "position")
            {
                setupBoard(iss);
            }
            else if (token == "go")
            {
                handleGo(iss);
            }
            else if (token == "stop")
            {
                // Not implemented: should stop search
            }
            else if (token == "quit")
            {
                break;
            }
            else if (token == "debug")
            {
                std::string mode;
                iss >> mode;
                debug_mode_ = (mode == "on");
            }
            else if (token == "test")
            {
                /*
                 * Warning; Illegal pv move c1d2 from new
                 Info; info score cp 47 depth 3 nodes 22286 max_time 283 given_time 3687 time 15 hash_hits: 302761 hash_miss: 5864413 pv c8e6 c2c4 d4c3 c1d2 
Position; fen rnbqk1nr/ppp1ppbp/6p1/4P3/3p4/3P2Q1/PPP2PPP/RNB1KBNR b KQkq - 3 5
Moves; b8c6 g1f3 g8h6 c1h6 g7h6 b1d2 e8g8 e1c1 f7f6 d1e1  */
                std::istringstream test("fen rnbqk1nr/ppp1ppbp/6p1/4P3/3p4/3P2Q1/PPP2PPP/RNB1KBNR b KQkq - 3 5 moves b8c6 g1f3 g8h6 c1h6 g7h6 b1d2 e8g8 e1c1 f7f6 d1e1 ");
                engine_.tc.winc = 100000000;
                engine_.tc.binc = 100000000;
            	setupBoard(test);
                engine_.b.printBitBoards();
                engine_.b.printBoard();
                std::istringstream go_stream("go movetime 10000000");
                handleGo(go_stream);
            }
            // Add more UCI commands as needed
        }
    }

private:
    Engine engine_;
    bool debug_mode_;

    void sendId()
    {
        std::cout << "id name nchess" << std::endl;
        std::cout << "id author nia" << std::endl;
    }

    void sendOptions()
    {
        // Example: std::cout << "option name Hash type spin default 16 min 1 max 128" << std::endl;
    }



    void handleGo(std::istringstream& iss)
    {
        int depth = 0;
        std::string token;
        TimeControl tc;
        while (iss >> token) {
            if (token == "wtime") iss >> tc.wtime;
            if (token == "winc") iss >> tc.winc;
            if (token == "btime") iss >> tc.btime;
            if (token == "binc") iss >> tc.binc;
            if (token == "depth") iss >> depth;
            if (token == "movetime") iss >> tc.movetime;
        }
        engine_.tc = tc;
        Move best_move = engine_.search(depth > 0 ? depth : 7);
        

    	std::cout << "bestmove " << best_move.toUci() << std::endl;

    }
};