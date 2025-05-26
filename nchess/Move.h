#pragma once
#include <stdexcept>
#include <stdint.h>
#include <string>
#include "Enums.h"

class Move
{
private:
	uint32_t data = 0;
	//bit 0-5: from
	//bits 6-11: to
	//bits 12-14: piece type
	//bits 15-17: captured piece type
	//bits 18-20: promotion 
	//bit 21: ep flag
public:
	constexpr Move() = default;
	constexpr Move(std::string uci) {
		if (uci.size() < 4 || uci.size() > 5) {
			throw std::invalid_argument("Invalid UCI move format");
		}
		uint8_t from_file = uci[0] - 'a';
		uint8_t from_rank = uci[1] - '1';
		uint8_t to_file = uci[2] - 'a';
		uint8_t to_rank = uci[3] - '1';
		if (from_file > 7 || from_rank > 7 || to_file > 7 || to_rank > 7) {
			throw std::out_of_range("UCI move out of bounds");
		}
		data |= (from_file & 0x7) | ((from_rank & 0x7) << 3);
		data |= ((to_file & 0x7) | ((to_rank & 0x7) << 3)) << 6;
		if (uci.size() == 5) {
			switch (uci[4]) {
			case 'n': data |= eKnight << 12; break;
			case 'b': data |= eBishop << 12; break;
			case 'r': data |= eRook << 12; break;
			case 'q': data |= eQueen << 12; break;
			default: throw std::invalid_argument("Invalid promotion piece");
			}
		}
	}
	constexpr Move(uint8_t from, uint8_t to) {
		data |= from & 0x3F;
		data |= (to & 0x3F) << 6;
	};
	constexpr Move(uint8_t from, uint8_t to, uint8_t moved) : Move(from, to) {
		data |= (moved & 0x7) << 12;
	};

	constexpr Move(uint8_t from, uint8_t to, uint8_t moved, uint8_t capture) : Move(from, to, moved) {
		data |= (capture & 0x7) << 15;
	};
	
	constexpr Move(uint8_t from, uint8_t to, uint8_t moved, uint8_t capture, uint8_t promotion) : Move(from, to, moved, capture) {
		data |= (promotion & 0x7) << 18;
	};

	constexpr Move(uint8_t from, uint8_t to, uint8_t moved, uint8_t capture, uint8_t promotion, bool ep_flag) : Move(from, to, moved, capture, promotion) {
		data |= static_cast<uint32_t>(ep_flag) << 21;
	};
	
	[[nodiscard]] uint8_t from() const { return data & 0x3F; }
	[[nodiscard]] uint8_t to() const { return (data >> 6) & 0x3F; }
	[[nodiscard]] uint8_t piece() const { return (data >> 12) & 0x7; }
	void setMoved(uint8_t piece) { data |= (piece & 0x7) << 12; }

	[[nodiscard]] uint8_t captured() const { return (data >> 15) & 0x7; }
	void setCaptured(uint8_t piece) { data |= (piece & 0x7) << 15; }

	[[nodiscard]] uint8_t promotion() const { return (data >> 18) & 0x7; }
	void setPromotion(uint8_t piece) { data |= (piece & 0x7) << 18; }

	// Removed side() and setSide()

	[[nodiscard]] bool isEnPassant() const { return data & (1 << 21); }
	void setEnPassant(bool ep_flag) { data |= static_cast<uint32_t>(ep_flag) << 21; }

	[[nodiscard]] std::string toUci() const {
		std::string out;
		out += static_cast<char>('a' + (from() & 0x7));
		out += static_cast<char>('1' + ((from() & 0x38) >> 3));
		out += static_cast<char>('a' + (to() & 0x7));
		out += static_cast<char>('1' + ((to() & 0x38) >> 3));
		switch (promotion()) {
			case eKnight: out += "n"; break;
			case eBishop: out += "b"; break;
			case eRook: out += "r"; break;
			case eQueen: out += "q"; break;
			default: break; // No promotion
		}
		return out;
	}

	

	[[nodiscard]] uint32_t raw() const { return data; }
};
