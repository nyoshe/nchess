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
#include <ranges>

inline uint64_t rnd64(uint64_t n)
{
    const uint64_t z = 0x9FB21C651E98DF25;

    n ^= ((n << 49) | (n >> 15)) ^ ((n << 24) | (n >> 40));
    n *= z;
    n ^= n >> 35;
    n *= z;
    n ^= n >> 28;

    return n;
}

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

    static std::array<u64, 793> initZobristValues() {
        uint64_t state = 69;
        std::array<u64, 793> values{};
        for (int i = 0; i < 793; i++) {
            values[i] = rnd64(state++);
        }
        return values;
    }

    inline static std::array<u64, 793> z_val = initZobristValues();
    
    std::array<uint8_t, 64> piece_board;

    int16_t eval = 0;
    u64 hash = 0;
    int half_move = 0;
    
    uint8_t castle_flags = 0b1111;
	std::vector<BoardState> state_stack;
	int ep_square = -1; // -1 means no en passant square, ep square represents piece taken

public:
    bool us = eWhite;
    int ply = 0;
	Board();
    // Copy constructor
    Board(const Board& other);

	// Equality operator
    bool operator==(const Board& other) const;

    void setOccupancy();

    //fancy print
	void printBoard() const;
	void printBitBoards() const;
    [[nodiscard]] std::string boardString() const; //outputs board string in standard ascii

    void doMove(Move move);
    void undoMove();
    void movePiece(uint8_t from, uint8_t to);
    void setPiece(uint8_t square, uint8_t color, uint8_t piece);
    void removePiece(uint8_t square);

    [[nodiscard]] Side getSide(int square) const;
    void loadFen(std::istringstream& fen_stream);

    // Returns a Move object corresponding to the given UCI string (e.g. "e2e4", "e7e8q").
    Move moveFromSan(const std::string& san);
    std::string sanFromMove(Move move);
    Move moveFromUCI(const std::string& uci);
    void loadUci(std::istringstream& uci);
	void genPseudoLegalCaptures(std::vector<Move>& moves);
    void serializeMoves(Piece piece, std::vector<Move>& moves, bool quiet);

	void genPseudoLegalMoves(std::vector<Move>& moves);
    void filterToLegal(std::vector<Move>& pseudo_moves);


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
    void printMoves() const;
	void reset();

    std::vector<Move> getLastMoves(int n_moves) {
        std::vector<Move> last_moves;
        const size_t available_moves = state_stack.size();
        const size_t moves_to_return = min(static_cast<size_t>(n_moves), available_moves);

        last_moves.reserve(moves_to_return);
        for (size_t i = 0; i < moves_to_return; ++i) {
            last_moves.push_back(state_stack[available_moves - 1 - i].move);
        }

        return last_moves;
    }

    u64 getHash();
};

