#pragma once
#include "Misc.h"
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
#include <unordered_map>
#include "robin_hood.h"

inline u64 rnd64()
{
    static u64 i = 69;
    return (i = (164603309694725029ull * i) % 14738995463583502973ull);
}


struct BoardState {
    i8 ep_square = -1;
    u8 castle_flags = 0b1111; // 0bKQkq
    Move move;
    int eval = 0;
    u64 hash = 0;
    BoardState(int ep_square, u8 castle_flags, Move move, int16_t eval, u64 hash)
        : ep_square(ep_square), castle_flags(castle_flags), move(move), eval(eval), hash(hash) {
    };

    auto operator<=>(const BoardState&) const = default;
};

struct Zobrist {
    std::array<u64, 12 * 64> piece_at;
    u64 side;
    std::array<u64, 16> castle_rights;
    std::array<u64, 8> ep_file;
};

class Board
{
private:
	// boards[side][0] = occupancy
    //256 seems big enough, right?
    //std::vector<Move> legal_moves;
    std::array < std::array<u64, 7>, 2 > boards;

    static Zobrist initZobristValues() {
        Zobrist z;

        for (auto& val : z.piece_at) val = rnd64();
        z.side = rnd64();
        for (auto& val : z.castle_rights) val = rnd64();
        for (auto& val : z.ep_file) val = rnd64();

        return z;
    }

    Zobrist z = initZobristValues();
    
    std::array<u8, 64> piece_board;

    int16_t eval = 0;
    u64 hash = 0;
    int half_move = 0;
    
    u8 castle_flags = 0b1111;
	
	int ep_square = -1; // -1 means no en passant square, ep square represents piece taken
    robin_hood::unordered_map < u64, int > pos_history;
public:
    std::vector<BoardState> state_stack;
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
    void movePiece(u8 from, u8 to);
    void setPiece(u8 square, u8 color, u8 piece);
    void removePiece(u8 square);

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
	bool is3fold();

    [[nodiscard]] u64 calcHash() {
        u64 out_hash = 0;
	    for (int sq = 0; sq < 64; sq++) {
            if (piece_board[sq] != eNone) {
                out_hash ^= z.piece_at[sq * 12 + (piece_board[sq] - 1) + (getSide(sq) * 6)];
            }
	    }
        out_hash ^= z.castle_rights[castle_flags];
        if (us) out_hash ^= z.side;
        if (ep_square != -1) out_hash ^= z.ep_file[ep_square & 0x7];
        return out_hash;
    }

    void updateZobrist(Move move) {
        u8 p = move.piece();
        hash ^= z.side;

        hash ^= z.piece_at[(move.from() * 12) + (move.piece() - 1) + (!us * 6)]; //invert from square hash

        if (move.promotion() != eNone) {
            hash ^= z.piece_at[(move.to() * 12) + (move.promotion() - 1) + (!us * 6)]; 
        } else {
            hash ^= z.piece_at[(move.to() * 12) + (move.piece() - 1) + (!us * 6)];
        }
        
        if (move.captured() != eNone) {
            if (move.isEnPassant()) {

                hash ^= z.piece_at[((state_stack.end() - 2)->move.to() * 12) + (ePawn - 1) + (us * 6)]; //add captured piece
            } else {
                hash ^= z.piece_at[(move.to() * 12) + (move.captured() - 1) + (us * 6)]; //add captured piece
            }
        }
        

        // Update side to move  


        if (state_stack.back().castle_flags != castle_flags) {
            hash ^= z.castle_rights[state_stack.back().castle_flags];
            hash ^= z.castle_rights[castle_flags];
        }

        if (state_stack.back().ep_square != -1) hash ^= z.ep_file[state_stack.back().ep_square & 0x7];
        if (ep_square != -1) hash ^= z.ep_file[ep_square & 0x7];


        if (p == eKing) {
            // Move the rook if castling
            if (std::abs((int)move.to() - (int)move.from()) == 2) {
                switch (move.to()) {
                case g1: 
                    hash ^= z.piece_at[(h1 * 12) + (eRook - 1) + (!us * 6)];
                    hash ^= z.piece_at[(f1 * 12) + (eRook - 1) + (!us * 6)];
                	break; // King-side castling for white
                case c1: 
                    hash ^= z.piece_at[(a1 * 12) + (eRook - 1) + (!us * 6)]; 
                    hash ^= z.piece_at[(d1 * 12) + (eRook - 1) + (!us * 6)];
                    break; // Queen-side castling for white
                case g8:
                    hash ^= z.piece_at[(h8 * 12) + (eRook - 1) + (!us * 6)];
                    hash ^= z.piece_at[(f8 * 12) + (eRook - 1) + (!us * 6)];
                    break;// King-side castling for black
                case c8:
                    hash ^= z.piece_at[(a8 * 12) + (eRook - 1) + (!us * 6)];
                    hash ^= z.piece_at[(d8 * 12) + (eRook - 1) + (!us * 6)];
                    break; // Queen-side castling for black
                default: throw std::invalid_argument("Invalid castling move");
                }
            }
        }
    }
};

