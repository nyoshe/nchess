#include "pch.h"
#include "../nchess/Board.h"
#include "../nchess/Engine.h"
#include "../nchess/pgn.h"
#include  "../nchess/Memory.h"
#include <filesystem>
#include <map>
// Test board initializes to standard chess position


//www.chessprogramming.org/Perft_Results
std::map<std::string, std::vector<PerfT>> perft_test_data = {
    { "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 ", {
        {1, 20 	,0,0, 0, 0, 0, 0, 0},
        {2, 400,0,0, 0, 0, 0, 0, 0},
        {3, 8902 , 34,0, 0, 0, 12, 0, 0},
	    {4, 197281, 1576, 0, 0, 0, 469, 0, 0, 8},
	    {5, 4865609, 82719, 258, 0, 0, 27351, 6, 0, 347},
	    {6, 119060324, 2812008, 5248, 0, 0, 809099, 329, 46, 10828},
	    {7, 3195901860, 108329926, 319617, 883453, 0, 33103848, 18026, 1628, 435767},
	    {8, 84998978956, 3523740106, 7187977, 23605205, 0, 968981593, 847039, 147215, 9852036},
	    {9, 2439530234167, 125208536153, 319496827, 1784356000, 17334376, 36095901903, 37101713, 5547231, 400191963}
	}
},
    { "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - ", {
        {1, 48, 8, 0, 2, 0, 0, 0, 0, 0},
        {2, 2039, 351, 1, 91, 0, 3, 0, 0, 0},
        {3, 97862, 17102, 45, 3162, 0, 993, 0, 0, 1},
        {4, 4085603, 757163, 1929, 128013, 15172, 25523, 42, 6, 43},
        {5, 193690690, 35043416, 73365, 4993637, 8392, 3309887, 19883, 2637, 30171},
        {6, 8031647685, 1558445089, 3577504, 184513607, 56627920, 92238050, 568417, 54948, 360003}
    } }
};

struct TestPosition {
    std::string fen;                  // FEN position string
    std::string id;                   // Position ID (e.g. "ERET 001")
    std::string description;          // Description (e.g. "Relief")
    std::vector<std::string> moves;   // Best/avoid moves
    bool is_white_to_move;           // true if white to move
    bool is_avoid_move;              // true if moves are to be avoided (am) rather than best moves (bm)

    bool operator==(const TestPosition& other) const {
        return fen == other.fen &&
            id == other.id &&
            description == other.description &&
            moves == other.moves &&
            is_white_to_move == other.is_white_to_move &&
            is_avoid_move == other.is_avoid_move;
    }
};

TEST(BoardTest, InitializesToStartingPosition) {
    Board board;
    // a1 = 0, e1 = 4, e8 = 60, a8 = 56
    EXPECT_EQ(board.getSide(0), eWhite);  // a1 rook
    EXPECT_EQ(board.getSide(4), eWhite);  // e1 king
    EXPECT_EQ(board.getSide(60), eBlack); // e8 king
    EXPECT_EQ(board.getSide(56), eBlack); // a8 rook
    // Pawns
    for (int i = 8; i < 16; ++i)
        EXPECT_EQ(board.getSide(i), eWhite);
    for (int i = 48; i < 56; ++i)
        EXPECT_EQ(board.getSide(i), eBlack);
    // Empty squares
    for (int i = 16; i < 48; ++i)
        EXPECT_EQ(board.getSide(i), eSideNone);
}

// Test making and undoing a move
TEST(BoardTest, MakeAndUndoMove) {
    Board board;
    StaticVector<Move> moves;

    board.genPseudoLegalMoves(moves);
    board.filterToLegal(moves);
    // Find e2e4
    Move move;
    for (const auto& m : moves) {
        if (m.from() == 12 && m.to() == 28) { // e2 (12) to e4 (28)
            move = m;
            break;
        }
    }
    board.doMove(move);
    EXPECT_EQ(board.getSide(28), eWhite); // Pawn is on e4
    EXPECT_EQ(board.getSide(12), eSideNone); // e2 is empty
    board.undoMove();
    EXPECT_EQ(board.getSide(12), eWhite); // Pawn back on e2
    EXPECT_EQ(board.getSide(28), eSideNone); // e4 is empty
}

// Test SAN move parsing and generation
TEST(BoardTest, MoveFromSanAndSanFromMove) {
    Board board;
    Move move = board.moveFromSan("e4");
    EXPECT_EQ(move.from(), 12); // e2
    EXPECT_EQ(move.to(), 28);   // e4
    board.doMove(move);
    std::string san = board.sanFromMove(move);
    EXPECT_EQ(san, "e4");
}

// Test castling moves
TEST(BoardTest, CastlingMoves) {
    Board board;
    // Set up a position where castling is legal
    // Remove pieces between king and rook
    for (int sq : {5, 6, 1, 2, 3}) {
        board.removePiece(sq);
    }
    // King-side castling
    StaticVector<Move> moves;

    board.genPseudoLegalMoves(moves);
    board.filterToLegal(moves);
    bool found_kingside = false, found_queenside = false;
    for (const auto& m : moves) {
        if (m.piece() == eKing && m.from() == 4 && m.to() == 6)
            found_kingside = true;
        if (m.piece() == eKing && m.from() == 4 && m.to() == 2)
            found_queenside = true;
    }
    EXPECT_TRUE(found_kingside);
    EXPECT_TRUE(found_queenside);
}

// Test getAttackers returns correct attackers for a square
TEST(BoardTest, GetAttackersReturnsCorrectAttackers) {
    Board board;
    // Clear the board first
    for (int sq = 0; sq < 64; ++sq) {
        board.removePiece(sq);
    }
    // Place a white rook on d4 (27), a black bishop on h4 (31), and a white knight on e6 (36)
    board.setPiece(d4, eWhite, eRook);    // d4
    board.setPiece(h5, eBlack, eBishop);  // h5
    board.setPiece(f6, eWhite, eKnight);  // f6

    // Test attackers to d6 (square 35)
    auto blackAttackers = board.getAttackers(g4, eWhite);
    auto whiteAttackers = board.getAttackers(g4, eBlack);

    // White rook on d4 attacks d6 (same file)
    // White knight on e6 attacks d6 (knight move)
    // Black bishop does not attack d6
    std::cout << board.boardString();
    // Check white attackers
    EXPECT_EQ(BB::popcnt(whiteAttackers), 2);
    // Check black attackers
    EXPECT_EQ(BB::popcnt(blackAttackers), 1);
}

TEST(BoardTest, TestGames) {
    Board board;
    std::ifstream file("output_with_evals.pgn");
    if (!file) {
        std::cout << "Current directory: " << std::filesystem::current_path() << std::endl;
    }
    
    auto allGames = pgn::read_pgn_file(file);
    for (int var = 50; var < 200; var+=2) {
        board.tunable = var;
        double sum_squared_diff = 0.0;
        int move_count = 0;
        for (int i = 0; i < 2000; i++) {

            for (auto gameMove : allGames[i]) {
                Move move = board.moveFromSan(gameMove.san);
                if (!move.raw()) continue;

                Board test_board = board;
                board.doMove(move);
                board.undoMove();

                EXPECT_EQ(board, test_board);

                board.doMove(move);

                double board_eval = static_cast<double>(board.us ? -board.getEval() : board.getEval());
                double diff = board_eval - static_cast<double>(gameMove.eval);

                //excludes checks from static eval
                if (std::abs(diff) <= 50000) {

                    sum_squared_diff += diff * diff;
                    move_count++;
                }

            }

            for (int i = 0; i < allGames.size(); i++) {
                board.undoMove();
            }


            EXPECT_EQ(board.getOccupancy(), 0xFFFF00000000FFFF);
        }
        if (move_count > 0) {
            double mean_square_diff = std::sqrt(sum_squared_diff) / move_count;
            // 240.605
            std::cout << "var: " << var;
            std::cout << "   Mean square difference: " << mean_square_diff << std::endl;
        }
        else {
            std::cout << "No moves to compare." << std::endl;
        }
    }
    
}

TEST(BoardTest, PerftPositions) {
    Engine engine;
    int depth = 4;
    for (const auto& test : perft_test_data) {
        std::string fen = test.first;
        std::vector<PerfT> expected_output = test.second;
        std::vector<PerfT> results = engine.doPerftSearch(fen, depth);

        for (int i = 0; i < depth; i++) {
            std::cout << "\n\nexpected: " << expected_output[i].to_string() << "\n";
            std::cout << "results: " << results[i].to_string() << "\n\n";
        }

        EXPECT_EQ(expected_output[0], results[0]);
    }
}