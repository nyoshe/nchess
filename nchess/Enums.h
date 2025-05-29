#pragma once
#include <cstdint>
#include <string>

enum Piece : uint8_t {
	eNone, ePawn, eKnight, eBishop, eRook, eQueen, eKing
};

enum Side : uint8_t {
	eWhite, eBlack, eSideNone
};

enum SpecialMove : uint8_t {
	wShortCastle, wLongCastle, bShortCastle, bLongCastle, enPassant
};

enum CastleMask : uint8_t {
	wShortCastleFlag = 0b0001,
	wLongCastleFlag = 0b0010,
	bShortCastleFlag = 0b0100,
	bLongCastleFlag = 0b1000
};

enum Direction : uint8_t {
	eNorth, eSouth, eEast, eWest, eNorthEast, eNorthWest, eSouthEast, eSouthWest
};

enum Square : uint8_t {
	a1, b1, c1, d1, e1, f1, g1, h1,
	a2, b2, c2, d2, e2, f2, g2, h2,
	a3, b3, c3, d3, e3, f3, g3, h3,
	a4, b4, c4, d4, e4, f4, g4, h4,
	a5, b5, c5, d5, e5, f5, g5, h5,
	a6, b6, c6, d6, e6, f6, g6, h6,
	a7, b7, c7, d7, e7, f7, g7, h7,
	a8, b8, c8, d8, e8, f8, g8, h8
};

struct Pos {
	int8_t f; // 0-7
	int8_t r; // 0-7
	Pos(const int8_t f, const int8_t r) : f(f), r(r) {};
	Pos(const uint8_t square) : f(square & 0x7), r(square >> 3) {};
	Pos() : f(0), r(0) {};
	bool operator==(const Pos& other) const {
		return f == other.f && r == other.r;
	}
	Pos operator+(const Pos& other) const {
		return Pos(f + other.f, r + other.r);
	}

	Pos& operator+=(const Pos& other) {
		f += other.f;
		r += other.r;
		return *this;
	}

	uint8_t toSquare() const {
		return (r << 3) | f; // rank * 8 + file
	}
};


struct PerfT {
	int depth = 0;                // Search depth
	uint64_t nodes = 0;           // Total nodes
	uint64_t captures = 0;        // Number of captures
	uint64_t en_passant = 0;      // En passant moves
	uint64_t castles = 0;         // Castling moves
	uint64_t promotions = 0;      // Promotions
	uint64_t checks = 0;          // Checks
	uint64_t discovery_checks = 0;// Discovery checks
	uint64_t double_checks = 0;   // Double checks
	uint64_t checkmates = 0;      // Checkmates 
    bool operator==(const PerfT&) const = default;

    std::string to_string() {
    return
    "depth: " + std::to_string(depth) +
    ", nodes: " + std::to_string(nodes) +
    ", captures: " + std::to_string(captures) +
    ", en_passant: " + std::to_string(en_passant) +
    ", castles: " + std::to_string(castles) +
    ", promotions: " + std::to_string(promotions) +
    ", checks: " + std::to_string(checks) +
    ", discovery_checks: " + std::to_string(discovery_checks) +
    ", double_checks: " + std::to_string(double_checks) +
    ", checkmates: " + std::to_string(checkmates);
    };
};