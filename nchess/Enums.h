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
	wShortCastleMask = 0b0001,
	wLongCastleMask = 0b0010,
	bShortCastleMask = 0b0100,
	bLongCastleMask = 0b1000
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
	uint8_t file; // 0-7
	uint8_t rank; // 0-7
	Pos(uint8_t f, uint8_t r) : file(f), rank(r) {};
	Pos(uint8_t square) : file(square & 0x7), rank(square >> 3) {};
	Pos() : file(0), rank(0) {};
	bool operator==(const Pos& other) const {
		return file == other.file && rank == other.rank;
	}
	Pos operator+(const Pos& other) const {
		return Pos(file + other.file, rank + other.rank);
	}
};
