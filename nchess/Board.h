#pragma once
#include "Enums.h"
#include "BitBoard.h"
#include "Move.h"
#include <iostream>
#include <iomanip>
#include <io.h>
#include <fcntl.h>
#include <Windows.h>
#include <stdio.h>
#include <vector>
#include <cassert>
struct BoardState {
    int ep_square = -1;
    uint8_t castle_flags = 0b1111; // 0bKQkq
    Move move;
    BoardState(int ep_square, uint8_t castle_flags, Move move)
        : ep_square(ep_square), castle_flags(castle_flags), move(move) {
    };
};

class Board
{
private:
	// boards[side][0] = occupancy
    u64 boards[2][7];

    uint8_t piece_board[64] = {
        eRook,   eKnight, eBishop, eQueen,  eKing,   eBishop, eKnight, eRook,
        ePawn,   ePawn,   ePawn,   ePawn,   ePawn,   ePawn,   ePawn,   ePawn,
        eNone,   eNone,   eNone,   eNone,   eNone,   eNone,   eNone,   eNone,
        eNone,   eNone,   eNone,   eNone,   eNone,   eNone,   eNone,   eNone,
        eNone,   eNone,   eNone,   eNone,   eNone,   eNone,   eNone,   eNone,
        eNone,   eNone,   eNone,   eNone,   eNone,   eNone,   eNone,   eNone,
        ePawn,   ePawn,   ePawn,   ePawn,   ePawn,   ePawn,   ePawn,   ePawn,
        eRook,   eKnight, eBishop, eQueen,  eKing,   eBishop, eKnight, eRook
    };



    int ply = 0;
    bool side_to_move = eWhite;
    uint8_t castle_flags = 0b1111;
	std::vector<BoardState> state_stack;
	int ep_square = -1; // -1 means no en passant square, ep square represents piece taken

public:

    void setOccupancy();

	Board() {
        boards[eWhite][ePawn] = 0x000000000000FF00;
        boards[eBlack][ePawn] = 0x00FF000000000000;

        boards[eWhite][eKnight] = 0b01000010;
        boards[eBlack][eKnight] = (u64(0b01000010) << 56);

        boards[eWhite][eBishop] = 0b00100100;
        boards[eBlack][eBishop] = (u64(0b00100100) << 56);
        
        boards[eWhite][eRook] = 0b10000001;
        boards[eBlack][eRook] = (u64(0b10000001) << 56);

        boards[eWhite][eQueen] = 0b00001000;
        boards[eBlack][eQueen] = (u64(0b00001000) << 56);

        boards[eWhite][eKing] = 0b00010000;
        boards[eBlack][eKing] = (u64(0b00010000) << 56);
        
        setOccupancy();
	};

    void printBoard() const;
	void printBitBoards() const;
    [[nodiscard]] std::string boardString() const; //outputs board string in standard ascii

    void doMove(Move move);
    void undoMove();
    void movePiece(uint8_t from, uint8_t to);
    void setPiece(uint8_t square, uint8_t color, uint8_t piece);
    void removePiece(uint8_t square);

    [[nodiscard]] Side getSide(int square) const;

    [[nodiscard]] Move moveFromSan(const std::string& san);
    [[nodiscard]] std::string sanFromMove(Move move) ;

	[[nodiscard]] std::vector<Move> getPseudoLegalMoves() const;
    [[nodiscard]] std::vector<Move> getLegalMoves() ;

	//get index of all attackers of a square
    [[nodiscard]] std::vector<int> getAttackers(int square) const;
	[[nodiscard]] std::vector<int> getAttackers(int square, bool side) const;

	[[nodiscard]] u64 getOccupancy() const {
		return boards[eWhite][0] | boards[eBlack][0];
	};

    [[nodiscard]] u64 getPieceBoard(Piece piece) const {
        return boards[eWhite][piece] | boards[eBlack][piece];
    }


};

