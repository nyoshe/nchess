#include "pch.h"
#include "../nchess/Board.h"

// Test board initializes to standard chess position
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
    auto moves = board.getLegalMoves();
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
    auto moves = board.getLegalMoves();
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
    board.setPiece(h4, eBlack, eBishop);  // h4
    board.setPiece(e6, eWhite, eKnight);  // e6

    // Test attackers to d6 (square 35)
    auto blackAttackers = board.getAttackers(35, eWhite);
    auto whiteAttackers = board.getAttackers(35, eBlack);

    // White rook on d4 attacks d6 (same file)
    // White knight on e6 attacks d6 (knight move)
    // Black bishop does not attack d6
    std::cout << board.boardString();
    // Check white attackers
    EXPECT_EQ(whiteAttackers.size(), 2);
    // Check black attackers
    EXPECT_EQ(blackAttackers.size(), 0);
}
