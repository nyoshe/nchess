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
                
                /* Warning; Illegal pv move e4f5 from new
Info; info score cp 2270 depth 6 nodes 266250 nps 4512711 pv h5h4 e4f5 e6f7 f5e5 h4h3 e5d5 h3h2
Position; fen rnb1kb1r/ppq2ppp/2p1pn2/3p4/3P4/1P2PN2/PBP1BPPP/RN1Q1RK1 w kq - 0 8
Moves; c2c4 f8e7 b1c3 e8g8 a1c1 h7h6 h2h3 a7a6 e2d3 f8d8 f1e1 d5c4 d3c4 c6c5 c4d3 b8c6 c3e4 b7b6 d3b1 f6e4 b1e4 c8b7 d1e2 a8c8 a2a4 e7f8 b2c3 f8d6 e2b2 c6b4 e4b7 b4d3 b7c8 d3b2 c8a6 c5d4 c3b2 c7a7 a6d3 d4e3 f2e3 a7a5 e1d1 d6c5 f3d4 d8b8 d3e4 b8d8 b2c3 a5a7 d4c2 d8d1 c1d1 a7c7 c3b4 f7f5 e4f3 c5b4 c2b4 c7c5 d1d4 c5c3 d4d8 g8h7 b4d3 c3b3 d8d6 b3a3 d6d8 a3a4 g1h2 a4a7 d8a8 a7c7 h2h1 c7d6 d3b2 d6d2 b2d1 e6e5 a8a7 e5e4 f3h5 h7h8 a7a8 h8h7 a8a7 d2d5 a7e7 f5f4 h5g4 h6h5 e7d7 d5a5 g4e6 a5e1 h1h2 e1g3 h2h1 f4f3 g2f3 e4f3 e6f5 h7h6 d7d2 g3e1 h1h2 e1d2 h2g3 h6g5 d1f2 d2e3 f2e4 g5f5 e4d6 f5e6 d6e8 e6d7 e8g7 e3g5 g3f3 g5g7 f3f4 d7e6 h3h4 g7g4 f4e3 g4h4 e3d3 h4f2 d3e4
            	*/

                std::istringstream test("fen r1bqkbnr/ppp1pppp/3p4/3Pn3/4P3/8/PPP2PPP/RNBQKBNR w KQkq - 1 4 moves f2f4 c8g4 f1e2 g4e2 g1e2 e5g6 e2d4 c7c5 d5c6");
                
				
                //std::istringstream test("fen rn1qkb1r/ppp1ppp1/7p/3p4/6bB/2P2N2/PPP1QPPP/R3KB1R b KQkq - 1 7 moves b8c6 e1c1 a8c8 c1b1 g7g5 h4g3 f8g7 h2h3 g4h5 h3h4 e7e5 h4g5 h6g5 h1h5 h8h5 f3e5 g7e5 e2h5 e5g3 f2g3 d8f6 f1b5 f6e5 d1f1 e5e7 h5h1 e8d7 h1h3 d7d8 h3f5 e7e6 f5f7 e6f7 f1f7 a7a6 b5c6 b7c6 f7g7 d8e8 a2a3 e8f8 g7g5 c8e8 g5f5 f8g7 f5g5 g7f6 g5g4 e8e1 b1a2 a6a5 g4f4 f6g7 f4g4 g7h7 g4f4 h7g7 c3c4 e1e2 c4d5 c6d5 c2c4 c7c6 c4d5 c6d5 f4f5 e2d2 a3a4 d2d3 g3g4 d3d4 a2b1 d4a4 f5d5 a4g4 d5a5 g4g2 a5a7 g7g6 a7d7 g2e2 b1c1 e2e4 c1b1 g6f5 d7f7 f5g4 f7g7 g4f3 g7f7 f3g4 b1c2 e4e2 c2c3 g4g3 b2b4 e2e1 b4b5 e1b1 c3c4 b1b2 c4c5 b2c2 c5b6 c2b2 f7d7 b2b1 d7g7 g3f3 g7f7 f3g3 f7g7 g3f3 b6c6 b1b2 b5b6 b2c2 c6d5 c2d2 d5c4 d2c2 c4d3 c2a2 b6b7 a2b2 g7f7 f3g3 d3c3 b2b5 f7g7 g3f4 g7c7 f4f3 ");
            	engine_.tc.winc = 100000000;
                engine_.tc.binc = 100000000;
            	setupBoard(test);
                //engine_.b.printBoard();
                //std::cout << engine_.b.boardString();
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