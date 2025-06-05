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
                
                /* Warning; Illegal pv move c1b1 from new
Info; info score cp -1 depth 2 nodes 2538 nps 1269000 hash_hits: 1710388 hash_miss: 2082932 pv f6g6 c1b1 a5a4 
Position; fen r1bqk2r/ppp2pp1/n3pn1p/3p4/1b1P1B2/2NQPN2/PPP2PPP/R3KB1R w KQkq - 0 7
Moves; g2g3 e8g8 f1g2 c7c5 e1c1 c5c4 d3e2 b4c3 b2c3 f6e4 f3e5 e4c3 e2g4 g7g5 d1e1 g8h7 g4h3 g5f4 e5g4 d8g5 e3f4 g5g6 g4e5 g6f5 e5g4 f5g6 g4e5 g6f5 e5g4 h6h5 e1e5 c3e2 c1d2 e2f4 e5f5 f4h3 f5h5 h7g7 g2h3 g7g6 h5h4 a6b4 g4e5 g6g7 h1d1 f7f6 h4g4 g7h7 g4h4 h7g8 e5g6 f8f7 d1e1 f6f5 g6e5 f7c7 c2c3 b4c6 d2c1 c6e5 e1e5 c8d7 h3g2 a8d8 h4h5 c7c8 h5g5 g8f7 g2f3 f7f6 g5h5 f6g6 h2h4 a7a5 h5g5 g6f6 g5h5 f6g6 h5g5 g6f6 g5h5 
            	*/

                std::istringstream test("fen r1bqk2r/ppp2pp1/n3pn1p/3p4/1b1P1B2/2NQPN2/PPP2PPP/R3KB1R w KQkq - 0 7 moves g2g3 e8g8 f1g2 c7c5 e1c1 c5c4 d3e2 b4c3 b2c3 f6e4 f3e5 e4c3 e2g4 g7g5 d1e1 g8h7 g4h3 g5f4 e5g4 d8g5 e3f4 g5g6 g4e5 g6f5 e5g4 f5g6 g4e5 g6f5 e5g4 h6h5 e1e5 c3e2 c1d2 e2f4 e5f5 f4h3 f5h5 h7g7 g2h3 g7g6 h5h4 a6b4 g4e5 g6g7 h1d1 f7f6 h4g4 g7h7 g4h4 h7g8 e5g6 f8f7 d1e1 f6f5 g6e5 f7c7 c2c3 b4c6 d2c1 c6e5 e1e5 c8d7 h3g2 a8d8 h4h5 c7c8 h5g5 g8f7 g2f3 f7f6 g5h5 f6g6 h2h4 a7a5 h5g5 g6f6 g5h5 f6g6 h5g5 g6f6 g5h5 ");
                
				
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