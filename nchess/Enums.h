#pragma once
#include <cstdint>
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
	Pos(int f, int r) : f(f), r(r) {};
	Pos(uint8_t square) : f(square & 0x7), r(square >> 3) {};
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
