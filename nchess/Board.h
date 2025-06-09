#pragma once
#include "Misc.h"
#include "BitBoard.h"
#include "Move.h"
#include "Tables.h"
#include <iostream>
#include <iomanip>
#include <io.h>
#include <fcntl.h>
#define NOMINMAX
#include <Windows.h>
#include <stdio.h>
#include <vector>
#include <array>
#include <cassert>
#include <sstream>
#include <ranges>

#include "Memory.h"
#include "robin_hood.h"

inline u64 rnd64()
{
    static u64 i = 69;
    return (i = (164603309694725029ull * i) % 14738995463583502973ull);
}


struct BoardState {
    u64 hash = 0;
    i8 ep_square = -1;
    u8 castle_flags = 0b1111; // 0bKQkq
    Move move;
    int eval = 0;
    u16 half_move;

    BoardState(int ep_square, u8 castle_flags, Move move, int eval, u64 hash, u16 half_move)
        : ep_square(ep_square), castle_flags(castle_flags), move(move), eval(eval), hash(hash), half_move(half_move) {
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
    int eval = 0;
    u64 hash = 0;
    u8 castle_flags = 0b1111;
    int ep_square = -1; // -1 means no en passant square, ep square represents piece taken
public:
    std::vector<BoardState> state_stack;
    bool us = eWhite;
    int ply = 0;
    u16 half_move = 0;
    Board();
    // Copy constructor
    Board(const Board& other) = default;

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
    void genPseudoLegalCaptures(StaticVector<Move>& moves);
    void serializeMoves(Piece piece, StaticVector<Move>& moves, bool quiet);

    void genPseudoLegalMoves(StaticVector<Move>& moves);
    void filterToLegal(StaticVector<Move>& pseudo_moves);
    bool isLegal(Move move);


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
    [[nodiscard]] int evalUpdate(Move move) ;

    [[nodiscard]] int getEval()  {
        eval = evalFullUpdate();
	    return us == eWhite ? eval : -eval;
    };


    void runSanityChecks() const;
    void printMoves() const;
    void reset();

    std::vector<Move> getLastMoves(int n_moves) const;

    u64 getHash() const;
    bool is3fold();

    [[nodiscard]] u64 calcHash() const;

    void updateZobrist(Move move);

    int getMobility(bool side) const;

    int evalFullUpdate() {
        int out = 0;
        int16_t game_phase = 24 -
            BB::popcnt(boards[eWhite][eKnight]) -
            BB::popcnt(boards[eWhite][eBishop]) -
            BB::popcnt(boards[eWhite][eRook]) * 2 -
            BB::popcnt(boards[eWhite][eQueen]) * 4 -
            BB::popcnt(boards[eBlack][eKnight]) -
            BB::popcnt(boards[eBlack][eBishop]) -
            BB::popcnt(boards[eBlack][eRook]) * 2 -
            BB::popcnt(boards[eBlack][eQueen]) * 4
            ;

        int mg_val = 0;
        int eg_val = 0;
        u64 white_squares = boards[eWhite][0];
        unsigned long at;
        while (white_squares) {
            BB::bitscan_reset(at, white_squares);
            u8 p = piece_board[at];
            u8 sq = at ^ 56;
            mg_val += mg_table[p][sq];
            eg_val += eg_table[p][sq];
        }

        u64 black_squares = boards[eBlack][0];
        at = 0;
        while (black_squares) {
            BB::bitscan_reset(at, black_squares);
            u8 p = piece_board[at];
            u8 sq = at;
            mg_val -= mg_table[p][sq];
            eg_val -= eg_table[p][sq];
        }

        out += (mg_val + (game_phase * eg_val - mg_val) / 24);
        /*
        //count doubled pawns
        //count isolated and doubled
        for (int file = 0; file < 8; file++) {
            if (boards[eWhite][ePawn] & BB::files[file])
                out -= (boards[eWhite][ePawn] & BB::neighbor_files[file]) ? 0 : 10;
            if (boards[eBlack][ePawn] & BB::files[file])
                out += (boards[eBlack][ePawn] & BB::neighbor_files[file]) ? 0 : 10;

            out -= 30 * (BB::popcnt(boards[eWhite][ePawn] & BB::files[file]) >= 2);
            out += 30 * (BB::popcnt(boards[eBlack][ePawn] & BB::files[file]) >= 2);
        }

        //count defenders
        u64 w_east_defenders = BB::get_pawn_attacks(eEast, eWhite, boards[eWhite][ePawn], boards[eWhite][ePawn]);
        u64 w_west_defenders = BB::get_pawn_attacks(eWest, eWhite, boards[eWhite][ePawn], boards[eWhite][ePawn]);
        u64 b_east_defenders = BB::get_pawn_attacks(eEast, eBlack, boards[eBlack][ePawn], boards[eBlack][ePawn]);
        u64 b_west_defenders = BB::get_pawn_attacks(eWest, eBlack, boards[eBlack][ePawn], boards[eBlack][ePawn]);

        //single defenders
        out += 3 * (BB::popcnt(w_east_defenders | w_west_defenders) - BB::popcnt(b_east_defenders | b_west_defenders));
        //out += 4 * (getMobility(eWhite) - getMobility(eBlack));
        //double defenders
        //out += var * (BB::popcnt(w_east_defenders & w_west_defenders) - BB::popcnt(b_east_defenders & b_west_defenders));
        out = us == eWhite ? out : -out;
		*/
        return out;
    }
};

