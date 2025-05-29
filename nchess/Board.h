#pragma once
#include "Enums.h"
#include "BitBoard.h"
#include "Move.h"
#include "Tables.h"
#include <iostream>
#include <iomanip>
#include <io.h>
#include <fcntl.h>
#include <Windows.h>
#include <stdio.h>
#include <vector>
#include <array>
#include <cassert>
#include <sstream>

struct BoardState {
    int ep_square = -1;
    uint8_t castle_flags = 0b1111; // 0bKQkq
    Move move;
    int16_t eval = 0;
    BoardState(int ep_square, uint8_t castle_flags, Move move, int16_t eval)
        : ep_square(ep_square), castle_flags(castle_flags), move(move), eval(eval) {
    };

    auto operator<=>(const BoardState&) const = default;
};

class Board
{
private:
	// boards[side][0] = occupancy
    //256 seems big enough, right?
    //std::vector<Move> legal_moves;
    std::array < std::array<u64, 7>, 2 > boards;

    std::array<uint8_t, 64> piece_board = {
        eRook,   eKnight, eBishop, eQueen,  eKing,   eBishop, eKnight, eRook,
        ePawn,   ePawn,   ePawn,   ePawn,   ePawn,   ePawn,   ePawn,   ePawn,
        eNone,   eNone,   eNone,   eNone,   eNone,   eNone,   eNone,   eNone,
        eNone,   eNone,   eNone,   eNone,   eNone,   eNone,   eNone,   eNone,
        eNone,   eNone,   eNone,   eNone,   eNone,   eNone,   eNone,   eNone,
        eNone,   eNone,   eNone,   eNone,   eNone,   eNone,   eNone,   eNone,
        ePawn,   ePawn,   ePawn,   ePawn,   ePawn,   ePawn,   ePawn,   ePawn,
        eRook,   eKnight, eBishop, eQueen,  eKing,   eBishop, eKnight, eRook
    };

    int16_t eval = 0;
    int ply = 0;
    bool us = eWhite;
    uint8_t castle_flags = 0b1111;
	std::vector<BoardState> state_stack;
	int ep_square = -1; // -1 means no en passant square, ep square represents piece taken

public:

	Board();
    // Copy constructor
    Board(const Board& other);

	// Equality operator
    bool operator==(const Board& other) const;

    void setOccupancy();

	void printBoard() const;
	void printBitBoards() const;
    [[nodiscard]] std::string boardString() const; //outputs board string in standard ascii

    void doMove(Move move);
    void undoMove();
    void movePiece(uint8_t from, uint8_t to);
    void setPiece(uint8_t square, uint8_t color, uint8_t piece);
    void removePiece(uint8_t square);

    [[nodiscard]] Side getSide(int square) const;
    void loadFen(const std::string& fen);

    [[nodiscard]] Move moveFromSan(const std::string& san);
    [[nodiscard]] std::string sanFromMove(Move move) ;
	void genPseudoLegalCaptures(std::vector<Move>& moves);

	void genPseudoLegalMoves(std::vector<Move>& moves);
    void genLegalMoves(std::vector<Move>& pseudo_moves);


	//get index of all attackers of a square
    [[nodiscard]] u64 getAttackers(int square) const;
	[[nodiscard]] u64 getAttackers(int square, bool side) const;
    [[nodiscard]] bool isCheck() const;

	[[nodiscard]] u64 getOccupancy() const {
		return boards[eWhite][0] | boards[eBlack][0];
	};

    [[nodiscard]] u64 getPieceBoard(Piece piece) const {
        return boards[eWhite][piece] | boards[eBlack][piece];
    }
    
    //incrementally update eval
    [[nodiscard]] int16_t evalUpdate() const;

    [[nodiscard]] int16_t getEval() const { return eval; };

    void runSanityChecks() const;
};

