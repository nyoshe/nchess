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

                std::istringstream test("fen r1bqkbnr/ppp1pppp/3p4/3Pn3/4P3/8/PPP2PPP/RNBQKBNR w KQkq - 1 4 moves b1c3 c8g4 f1e2 g4e2 g1e2 g8f6 e1g1 c7c6 c1g5 d8b6 g5f6 e7f6 a1b1 f8e7 e2g3 e8g8 g3f5 a8e8 d5c6 b7c6 f5e7 e8e7 d1d6 e7d7 d6a3 d7d2 b1c1 e5g4 c3d1 b6c7 a3g3 c7g3 h2g3 f8e8 d1c3 h7h6 c3b1 d2d4 b1c3 d4d2 a2a3 a7a6 c3b1 d2d4 b1c3 d4d2 a3a4 a6a5 c3b1 d2d4 b1c3 d4d2 b2b3 c6c5 c3b1 d2e2 b1c3 e2d2 c3d1 e8a8 d1b2 g4e5 f2f4 e5g4 b2c4 d2d4 e4e5 f6e5 f4e5 a8e8 c4d6 e8e5 d6f7 e5e2 f7h6 g7h6 c1e1 d4d2 e1e2 d2e2 f1d1 g4e3 d1d7 e2g2 g1h1 g2g3 c2c4 e3c2 d7d8 g8f7 d8d7 f7e8 d7c7 g3h3 h1g1 h3b3 c7c5 c2e3 c5c8 e8d7 c8h8 b3b1 g1h2 b1e1 c4c5 d7c6 h8c8 c6b7 c8d8 e1e2 h2g3 e3f5 g3f4 e2f2 f4e4 f5g3 e4d3 h6h5 d3c4 f2e2 c4b5 e2b2 b5a5 g3e4 d8h8 b2c2 c5c6 b7c7 h8h5 c7c6 h5h8 c2e2 h8c8 c6d7 c8g8 d7c7 a5b4 e2b2 b4a5 b2e2 a5b4 e4f6 g8g5 c7b8 a4a5 e2d2 a5a6 b8a7 g5g6 d2d4 b4c5 d4f4 c5d6 f6e4 d6e5 f4h4 e5d5 e4c3 d5e5 h4a4 g6g7 a7a6 e5f5 a4a2 g7d7 a2f2 f5g6 c3e4 d7e7 e4d2 g6h6 d2f3 h6g7 a6b5 e7e4 f2d2 e4e7 b5c4 g7g6 d2g2 g6f5 g2f2 f5f6 f3d4 f6g6 d4f3 e7c7 c4b3 c7b7 b3c3 b7c7 c3b2 c7b7 b2a2 b7a7 a2b2 a7b7 b2a2 b7a7 a2b1 a7b7 b1c2 b7c7 c2d2 g6f6 d2d3 f6f7 f3e5 f7e6 d3e4 c7h7 e5f3 e6f6 f2c2 h7f7 e4f4 f7e7 c2c6 f6g7 f4g3 e7e8 c6c7 g7f6 c7c6 f6f7 f3g5 f7g7 g5f3 e8e7 c6d6 g7h7 f3g5 h7g7 d6e6 e7d7 g5f3 g7f7 e6e4 f7g6 e4g4 g6f6 g4f4 f6e6 f4d4 d7c7 d4d2 c7g7 g3f2 g7c7 f2g3 c7g7 g3f2 g7c7 d2e2 e6f6 e2e4 c7h7 e4e3 h7f7 e3e1 f7g7 ");
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