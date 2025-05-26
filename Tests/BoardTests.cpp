#include "pch.h"


// Helper: get a Move for e2e4
Move make_e2e4(Board& board) {
    for (const auto& m : board.getLegalMoves()) {
        if (m.from() == 12 && m.to() == 28) // e2 to e4
            return m;
    }
    throw std::runtime_error("e2e4 not found");
}

TEST(BoardTests, SetOccupancySetsCorrectOccupancy) {
    Board board;
    board.setOccupancy();
    u64 expected_white = 0x000000000000FFFF;
    u64 expected_black = 0xFFFF000000000000;
    ASSERT_EQ(board.getOccupancy(), (expected_white | expected_black));
}

TEST(BoardTests, MovePieceMovesPieceCorrectly) {
    Board board;
    uint8_t from = 12; // e2
    uint8_t to = 28;   // e4
    board.movePiece(from, to);
    ASSERT_TRUE(board.getPieceBoard(ePawn) & BB::set_bit(to));
    ASSERT_FALSE(board.getPieceBoard(ePawn) & BB::set_bit(from));
    ASSERT_TRUE(board.getOccupancy() & BB::set_bit(to));
    ASSERT_EQ(board.getSide(to), eWhite);
}

TEST(BoardTests, SetPieceAndRemovePieceWorkAsExpected) {
    Board board;
    uint8_t sq = 20; // e3
    board.setPiece(sq, eWhite, eKnight);
    ASSERT_TRUE(board.getPieceBoard(eKnight) & BB::set_bit(sq));
    ASSERT_EQ(board.getSide(sq), eWhite);
    board.removePiece(sq);
    ASSERT_FALSE(board.getPieceBoard(eKnight) & BB::set_bit(sq));
    ASSERT_EQ(board.getSide(sq), eSideNone);
}

TEST(BoardTests, GetSideReturnsCorrectSide) {
    Board board;
    ASSERT_EQ(board.getSide(0), eWhite);
    ASSERT_EQ(board.getSide(63), eBlack);
    ASSERT_EQ(board.getSide(32), eSideNone);
}

TEST(BoardTests, DoMoveAndUndoMoveUpdateBoardState) {
    Board board;
    auto move = make_e2e4(board);
    board.doMove(move);
    ASSERT_EQ(board.getSide(28), eWhite);
    ASSERT_EQ(board.getSide(12), eSideNone);
    board.undoMove();
    ASSERT_EQ(board.getSide(12), eWhite);
    ASSERT_EQ(board.getSide(28), eSideNone);
}

TEST(BoardTests, MoveFromSanParsesSanCorrectly) {
    Board board;
    Move move = board.moveFromSan("e4");
    ASSERT_EQ(move.from(), 12);
    ASSERT_EQ(move.to(), 28);
    ASSERT_EQ(move.piece(), ePawn);

    Move move2 = board.moveFromSan("Nf3");
    ASSERT_EQ(move2.piece(), eKnight);
}

TEST(BoardTests, SanFromMoveGeneratesCorrectSan) {
    Board board;
    Move move = board.moveFromSan("e4");
    std::string san = board.sanFromMove(move);
    ASSERT_EQ(san, "e4");

    Move move2 = board.moveFromSan("Nf3");
    std::string san2 = board.sanFromMove(move2);
    ASSERT_EQ(san2, "Nf3");
}

TEST(BoardTests, GetPseudoLegalMovesReturnsPlausibleMoves) {
    Board board;
    auto moves = board.getPseudoLegalMoves();
    // In starting position, white has 20 pseudo-legal moves
    ASSERT_GE(moves.size(), 20);
}

TEST(BoardTests, GetLegalMovesReturnsOnlyLegalMoves) {
    Board board;
    auto moves = board.getLegalMoves();
    // In starting position, white has 20 legal moves
    ASSERT_EQ(moves.size(), 20);
}

TEST(BoardTests, GetAttackersFindsAttackersCorrectly) {
    Board board;
    // e4 is empty, so no attackers
    ASSERT_TRUE(board.getAttackers(28).empty());
    // e2 is a white pawn, so black has no attackers
    ASSERT_TRUE(board.getAttackers(12).empty());
    // After e2e4, black pawn on d7 can attack e4 if it moves
    auto move = board.moveFromSan("e4");
    board.doMove(move);
    auto attackers = board.getAttackers(28);
    // Should be empty in this position, but test call works
    ASSERT_EQ(attackers.size(), 0);
    board.undoMove();
}

TEST(BoardTests, GetOccupancyAndGetPieceBoardWork) {
    Board board;
    u64 occ = board.getOccupancy();
    ASSERT_TRUE(occ & BB::set_bit(0));
    ASSERT_TRUE(occ & BB::set_bit(63));
    u64 pawns = board.getPieceBoard(ePawn);
    ASSERT_TRUE(pawns & BB::set_bit(8));
    ASSERT_TRUE(pawns & BB::set_bit(55));
}

TEST(BoardTests, PrintBoardAndPrintBitBoardsDoNotThrow) {
    Board board;
    ASSERT_NO_THROW(board.printBoard());
    ASSERT_NO_THROW(board.printBitBoards());
}
