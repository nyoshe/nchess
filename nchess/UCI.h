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
                /*Warning; Illegal pv move f4g3 from new
Info; info score cp -1360 depth 4 nodes 2625 nps 2625000 hash_hits: 3506560 hash_miss: 24943868 pv g4g3 f6g7 g3f4 g7g6 f4g3
Position; fen r2qkb1r/1ppn1ppp/p3bn2/3p4/3P1B2/2NBPN2/PP3PPP/R2QK2R w KQkq - 4 9
Moves; e1g1 f8b4 d1e2 e8g8 a1e1 b4c3 b2c3 f6e4 e2c2 c7c6 d3e4 d5e4 f3d2 f7f5 f2f3 e4f3 d2f3 d7f6 f3e5 a8c8 h2h3 c6c5 c2b1 b7b5 b1b2 f6e4 e1d1 d8d5 b2a3 d5a2 a3a2 e6a2 d4c5 a2b3 d1d7 g7g5 f4h2 c8c5 e5d3 c5d5 d7d5 b3d5 f1c1 g5g4 h3g4 f5g4 d3b4 d5a8 h2f4 a8b7 f4e5 g4g3 c3c4 f8d8 c4c5 e4c5 e5g3 d8d2 c1c2 d2c2 b4c2 b7e4 c2d4 b5b4 g3d6 b4b3 d4e2 b3b2 e2c3 c5a4 d6a3 a4c3 a3b2 c3b1 g1f2 a6a5 g2g3 a5a4 b2f6 a4a3 f2e1 a3a2 g3g4 g8f7 f6e5 b1a3 e1f2 a3c4 e5d4 h7h6 f2g3 c4e3 g3f4 e3c2 d4e5 e4c6 g4g5 h6g5 f4g5 a2a1q e5a1 c2a1 g5f4 a1c2 f4f5 c2d4 f5e5 d4f3 e5f5 c6d5 f5g4 f7f6 g4f4 f6g6 f4g4 f3e5 g4f4 e5f3 f4g4 g6f6*/
                std::istringstream test("fen r2qkb1r/1ppn1ppp/p3bn2/3p4/3P1B2/2NBPN2/PP3PPP/R2QK2R w KQkq - 4 9 moves e1g1 f8b4 d1e2 e8g8 a1e1 b4c3 b2c3 f6e4 e2c2 c7c6 d3e4 d5e4 f3d2 f7f5 f2f3");
                engine_.tc.winc = 100000000;
                engine_.tc.binc = 100000000;
            	setupBoard(test);
                engine_.b.printBoard();
                std::cout << engine_.b.boardString();
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