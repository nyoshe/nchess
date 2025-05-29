#include "Board.h"


Board::Board() {
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


	//legal_moves.reserve(256);
	setOccupancy();
}

Board::Board(const Board& other) {
	for (int side = 0; side < 2; ++side) {
		for (int piece = 0; piece < 7; ++piece) {
			boards[side][piece] = other.boards[side][piece];
		}
	}
	for (int i = 0; i < 8; ++i) {
		reinterpret_cast<u64*>(&piece_board)[i] = reinterpret_cast<const u64*>(&other.piece_board)[i];
	}
	ply = other.ply;
	us = other.us;
	castle_flags = other.castle_flags;
	state_stack = other.state_stack;
	ep_square = other.ep_square;
}

bool Board::operator==(const Board& other) const {
	for (int side = 0; side < 2; ++side) {
		for (int piece = 0; piece < 7; ++piece) {
			if (boards[side][piece] != other.boards[side][piece])
				return false;
		}
	}
	//kinda cursed for quick comparison
	for (int i = 0; i < 8; ++i) {
		if (reinterpret_cast<const u64*>(&piece_board)[i] != reinterpret_cast<const u64*>(&piece_board)[i])
			return false;
	}
	if (ply != other.ply ||
		us != other.us ||
		castle_flags != other.castle_flags ||
		ep_square != other.ep_square ||
		state_stack != other.state_stack) {
		return false;
	}
		
	return true;
}

void Board::setOccupancy() {
	boards[eBlack][0] = 0;

	for (int p = 1; p < 7; p++) {
		boards[eBlack][0] |= boards[eBlack][p];
	}

	boards[eWhite][0] = 0;

	for (int p = 1; p < 7; p++) {
		boards[eWhite][0] |= boards[eWhite][p];
	}
}

void Board::doMove(Move move) {
	movePiece(move.from(), move.to());
	uint8_t p = move.piece();

	// Add move to stack, along with previous en passant square and castle flags
	state_stack.emplace_back(ep_square, castle_flags, move, eval);

	// Handle promotion  
	if (move.promotion() != eNone) {
		boards[us][p] &= ~BB::set_bit(move.to());
		boards[us][move.promotion()] |= BB::set_bit(move.to());
		piece_board[move.to()] = move.promotion();
	}

	// Handle en passant  
	if (move.isEnPassant()) {
		uint8_t ep_capture_square = move.to() + (us == eWhite ? -8 : 8);
		boards[!us][ePawn] &= ~BB::set_bit(ep_capture_square);
		boards[!us][0] &= ~BB::set_bit(ep_capture_square);
		piece_board[ep_capture_square] = eNone;
	}

	if (p == eKing) {
		// Move the rook if castling
		if (std::abs((int)move.to() - (int)move.from()) == 2) {
			switch (move.to()) {
			case g1: movePiece(h1, f1); break; // King-side castling for white
			case c1: movePiece(a1, d1); break; // Queen-side castling for white
			case g8: movePiece(h8, f8); break; // King-side castling for black
			case c8: movePiece(a8, d8); break; // Queen-side castling for black
			default: throw std::invalid_argument("Invalid castling move");
			}
		}
		//modify castle flags for king move
		castle_flags &= (us == eWhite) ? ~(wShortCastleFlag | wLongCastleFlag) : ~(bShortCastleFlag | bLongCastleFlag);
	}

	//modify castle flags for rook move
	if (p == eRook) {
		switch (move.from()) {
		case h1: castle_flags &= ~wShortCastleFlag; break; // White king-side rook
		case a1: castle_flags &= ~wLongCastleFlag; break; // White queen-side rook
		case h8: castle_flags &= ~bShortCastleFlag; break; // Black king-side rook
		case a8: castle_flags &= ~bLongCastleFlag; break; // Black queen-side rook
		default: break;
		}
	}

	//there really has to be a better way
	switch (move.to()) {
		case h1: castle_flags &= ~wShortCastleFlag; break; // White king-side rook
		case a1: castle_flags &= ~wLongCastleFlag; break; // White queen-side rook
		case h8: castle_flags &= ~bShortCastleFlag; break; // Black king-side rook
		case a8: castle_flags &= ~bLongCastleFlag; break; // Black queen-side rook
		default: break;
	}


	// Update en passant square
	if (p == ePawn && std::abs((int)move.to() - (int)move.from()) == 16) {
		// Set ep_square to the square behind the pawn after a double push
		ep_square = (move.from() + move.to()) / 2;
	}
	else {
		ep_square = -1;
	}

	eval = evalUpdate();
	// Update side to move  
	us ^= 1;

	// Increment ply count  
	ply++;


#ifndef NDEBUG
	runSanityChecks();
#endif
}

void Board::undoMove() {
	if (state_stack.empty()) return;
	if (BB::popcnt(boards[eBlack][ePawn]) > 8) {
		printBitBoards();
		throw std::logic_error("too many pawns!");
	}
	// Pop the last move
	Move move = state_stack.back().move;
	ep_square = state_stack.back().ep_square;
	castle_flags = state_stack.back().castle_flags;
	eval = state_stack.back().eval;
	state_stack.pop_back();

	// Switch side to move back
	us ^= 1;

	// Decrement ply count
	ply--;

	uint8_t from = move.from();
	uint8_t to = move.to();
	uint8_t piece = move.piece();
	uint8_t captured = move.captured();
	uint8_t promotion = move.promotion();
	// Handle castling undo (move rook back)
	if (piece == eKing && std::abs((int)to - (int)from) == 2) {
		if (us == eWhite) {
			if (to == 6) movePiece(5, 7); // King-side
			else if (to == 2) movePiece(3, 0); // Queen-side
		}
		else {
			if (to == 62) movePiece(61, 63); // King-side
			else if (to == 58) movePiece(59, 56); // Queen-side
		}
	}

	// Handle promotion reversal
	if (promotion != eNone) {
		removePiece(to); // Remove promoted piece
		setPiece(from, us, ePawn); // Restore pawn to 'from'
	}
	else {
		removePiece(to);
		setPiece(from, us, piece);
		//movePiece(to, from);
	}

	// Handle en passant undo
	if (move.isEnPassant()) {
		
		uint8_t ep_capture_square = to + (us == eWhite ? -8 : 8);
		setPiece(ep_capture_square, !us, ePawn);
	} else {
		// Restore captured piece (if any)
		if (captured != eNone) {
			setPiece(to, !us, captured);
		}
	}

#ifndef NDEBUG
	runSanityChecks();
#endif
}

void Board::printBoard() const {
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	bool color = true;

	for (int rank = 7; rank >= 0; rank--) {
		for (int file = 0; file <= 7; file++) {
			// Set background color for the square  
			std::string bgColor = "\x1b[48;2;" + std::to_string(color ? 252 : 230) + ";" +
				std::to_string(color ? 197 : 151) + ";" +
				std::to_string(color ? 142 : 55) + "m";
			printf(bgColor.c_str());

			u64 mask_pos = (1ULL << (file | (rank << 3)));

			// Check if no piece is present  
			if (!((boards[eWhite][0] & mask_pos) || (boards[eBlack][0] & mask_pos))) {
				std::cout << "  ";
				color = !color;
				continue;
			}

			// Print the piece if found  
			for (int side = 0; side < 2; side++) {
				// Set text color based on side  
				std::string textColor = (side == eBlack)
					                        ? "\x1b[38;2;0;0;0m"
					                        : "\x1b[38;2;255;255;255m";
				printf(textColor.c_str());

				wchar_t* pieceChar = nullptr;
				if (boards[side][eQueen] & mask_pos) pieceChar = const_cast<wchar_t*>(L"\u265B ");
				else if (boards[side][eKing] & mask_pos) pieceChar = const_cast<wchar_t*>(L"\u265A ");
				else if (boards[side][eRook] & mask_pos) pieceChar = const_cast<wchar_t*>(L"\u265C ");
				else if (boards[side][eBishop] & mask_pos) pieceChar = const_cast<wchar_t*>(L"\u265D ");
				else if (boards[side][eKnight] & mask_pos) pieceChar = const_cast<wchar_t*>(L"\u265E ");
				else if (boards[side][ePawn] & mask_pos) pieceChar = const_cast<wchar_t*>(L"\u265F ");

				if (pieceChar) {
					WriteConsoleW(hOut, pieceChar, wcslen(pieceChar), nullptr, nullptr);
					break;
				}
			}

			color = !color;
		}

		// Reset background and text color for the row  
		printf("\x1b[48;2;0;0;0m");
		printf("\x1b[38;2;255;255;255m");

		// Print rank number  
		std::cout << " " << static_cast<char>('1' + rank) << " \n";
		color = !color;
	}

	// Print file labels  
	std::cout << "a b c d e f g h \n\n";
}

void Board::printBitBoards() const {
	const std::string names[] = {
		"occupancy", "pawn", "knight", "bishop",
		"rook", "queen", "king"
	};

	for (int side = eWhite; side <= eBlack; side++) {
		std::cout << std::left;
		for (const auto& name : names) {
			std::cout << std::setw(18) << (side == eWhite ? "white " + name : "black " + name);
		}
		std::cout << "\n";

		for (int rank = 7; rank >= 0; rank--) {
			for (int i = 0; i < 7; i++) {
				std::cout << BB::rank_to_string(boards[side][i], rank) + "  ";
			}
			std::cout << "\n";
		}
		std::cout << "\n";
	}
}

std::string Board::boardString() const {
	std::string out;
	out += "   a  b  c  d  e  f  g  h\n"; // Add file labels at the top  
	for (int rank = 7; rank >= 0; rank--) {
		out += std::to_string(rank + 1) + " "; // Add rank label at the start of each row  
		for (int file = 0; file <= 7; file++) {
			u64 mask_pos = (1ULL << (file | (rank << 3)));
			if (!(boards[eWhite][0] & mask_pos) && !(boards[eBlack][0] & mask_pos)) {
				out += " . ";
			}
			else {
				if (getSide(file | (rank << 3)) == eWhite) out += " w";
				else out += " b";

				for (int side = 0; side < 2; side++) {
					if (boards[side][eQueen] & mask_pos) out += "Q";
					else if (boards[side][eKing] & mask_pos) out += "K";
					else if (boards[side][eRook] & mask_pos) out += "R";
					else if (boards[side][eBishop] & mask_pos) out += "B";
					else if (boards[side][eKnight] & mask_pos) out += "N";
					else if (boards[side][ePawn] & mask_pos) out += "P";
				}
			}
		}
		out += " " + std::to_string(rank + 1) + "\n"; // Add rank label at the end of each row  
	}
	out += "   a  b  c  d  e  f  g  h\n"; // Add file labels at the bottom  
	return out;
}

void Board::movePiece(uint8_t from, uint8_t to) {
	//clear and set our piece
	Side color = getSide(from);
	if (color == eSideNone) {
		printBitBoards();
		std::cout << int(from) << " " << int(to) << "\n" << boardString();
		throw std::logic_error("weird");
		
	}
	boards[color][piece_board[from]] ^= BB::set_bit(from);
	boards[color][0] ^= BB::set_bit(from);
	boards[color][piece_board[from]] |= BB::set_bit(to);
	boards[color][0] |= BB::set_bit(to);
	//clear enemy bit
	boards[!color][piece_board[to]] &= ~BB::set_bit(to);
	boards[!color][0] &= ~BB::set_bit(to);

	piece_board[to] = piece_board[from];
	piece_board[from] = eNone;


}

void Board::setPiece(uint8_t square, uint8_t color, uint8_t piece) {
	boards[color][piece] |= BB::set_bit(square);
	boards[color][0] |= BB::set_bit(square);
	piece_board[square] = piece;
}

void Board::removePiece(uint8_t square) {
	Side color = getSide(square);
	if (color != eSideNone) {
		boards[color][piece_board[square]] &= ~BB::set_bit(square);
		boards[color][0] &= ~BB::set_bit(square);
	}
	
	piece_board[square] = eNone;
}

Side Board::getSide(int square) const {
	return boards[eWhite][0] & BB::set_bit(square)
		       ? eWhite
		       : (boards[eBlack][0] & BB::set_bit(square) ? eBlack : eSideNone);
}

void Board::loadFen(const std::string& fen) {
	// Clear all bitboards and piece_board
	for (int side = 0; side < 2; ++side)
		for (int piece = 0; piece < 7; ++piece)
			boards[side][piece] = 0;
	for (int i = 0; i < 64; ++i)
		piece_board[i] = eNone;

	ply = 0;
	us = eWhite;
	castle_flags = 0;
	ep_square = -1;
	state_stack.clear();

	std::istringstream fen_stream(fen);
	std::string board_part, active_color, castling, ep, halfmove, fullmove;
	fen_stream >> board_part >> active_color >> castling >> ep >> halfmove >> fullmove;

	// Parse board
	int square = 56; // a8
	for (char c : board_part) {
		if (c == '/') {
			square -= 16; // move to next rank
		}
		else if (isdigit(c)) {
			square += c - '0';
		}
		else {
			int color = isupper(c) ? eWhite : eBlack;
			int piece = eNone;
			switch (tolower(c)) {
			case 'p': piece = ePawn; break;
			case 'n': piece = eKnight; break;
			case 'b': piece = eBishop; break;
			case 'r': piece = eRook; break;
			case 'q': piece = eQueen; break;
			case 'k': piece = eKing; break;
			}
			if (piece != eNone) {
				boards[color][piece] |= BB::set_bit(square);
				boards[color][0] |= BB::set_bit(square);
				piece_board[square] = piece;
			}
			++square;
		}
	}

	// Parse active color
	us = (active_color == "w") ? eWhite : eBlack;

	// Parse castling rights
	for (char c : castling) {
		switch (c) {
		case 'K': castle_flags |= wShortCastleFlag; break;
		case 'Q': castle_flags |= wLongCastleFlag; break;
		case 'k': castle_flags |= bShortCastleFlag; break;
		case 'q': castle_flags |= bLongCastleFlag; break;
		case '-': break;
		}
	}

	// Parse en passant square
	if (ep != "-" && ep.length() == 2) {
		int file = ep[0] - 'a';
		int rank = ep[1] - '1';
		ep_square = file + (rank << 3);
	}
	else {
		ep_square = -1;
	}

	// Parse ply (halfmove clock)
	if (!halfmove.empty())
		ply = std::stoi(halfmove);

	// Recompute occupancy
	setOccupancy();
}

Move Board::moveFromSan(const std::string& san) {
	std::string move = san;

	std::vector<Move> moves;
	genPseudoLegalMoves(moves);
	genLegalMoves(moves);
	// Remove check/mate symbols
	if (!move.empty() && (move.back() == '+' || move.back() == '#'))
		move.pop_back();

	
	// Handle castling
	if (move == "O-O" || move == "0-0") {
		for (auto& m : moves) {
			if (piece_board[m.from()] == eKing && ((int)m.to() - (int)m.from()) == 2)
				return m;
		}
		throw std::invalid_argument("No legal king-side castling move found");
	}
	if (move == "O-O-O" || move == "0-0-0") {
		for (auto& m : moves) {
			if (piece_board[m.from()] == eKing && ((int)m.to() - (int)m.from()) == -2)
				return m;
		}
		throw std::invalid_argument("No legal queen-side castling move found");
	}

	// Parse promotion
	char promotion = '\0';
	size_t eq = move.find('=');
	if (eq != std::string::npos && eq + 1 < move.size()) {
		promotion = move[eq + 1];
		move = move.substr(0, eq);
	}

	// Identify piece
	int piece = ePawn;
	size_t idx = 0;
	if (move[0] >= 'A' && move[0] <= 'Z' && move[0] != 'O') {
		switch (move[0]) {
		case 'N': piece = eKnight;
			break;
		case 'B': piece = eBishop;
			break;
		case 'R': piece = eRook;
			break;
		case 'Q': piece = eQueen;
			break;
		case 'K': piece = eKing;
			break;
		default: throw std::invalid_argument("Unknown piece in SAN");
		}
		idx = 1;
	}

	// Find destination square (always last two characters)
	if (move.size() < idx + 2)
		throw std::invalid_argument("Invalid SAN: missing destination square");
	int to_file = move[move.size() - 2] - 'a';
	int to_rank = move[move.size() - 1] - '1';
	int to_square = to_file | (to_rank << 3);

	// Parse disambiguation and capture between idx and move.size()-2
	char disambig_file = 0, disambig_rank = 0;
	bool is_capture = false;
	for (size_t i = idx; i < move.size() - 2; ++i) {
		if (move[i] == 'x') {
			is_capture = true;
		}
		else if (move[i] >= 'a' && move[i] <= 'h') {
			disambig_file = move[i];
		}
		else if (move[i] >= '1' && move[i] <= '8') {
			disambig_rank = move[i];
		}
	}

	// Find matching legal move
	for (const auto& m : moves) {
		if (m.to() != to_square) continue;
		if (m.piece() != piece) continue;
		if (promotion) {
			int promo_piece = eNone;
			switch (promotion) {
			case 'N': promo_piece = eKnight;
				break;
			case 'B': promo_piece = eBishop;
				break;
			case 'R': promo_piece = eRook;
				break;
			case 'Q': promo_piece = eQueen;
				break;
			default: continue;
			}
			if (m.promotion() != promo_piece) continue;
		}
		if (is_capture && m.captured() == eNone) continue;
		if (!is_capture && m.captured() != eNone) continue;
		if (disambig_file && ((m.from() & 7) != (disambig_file - 'a'))) continue;
		if (disambig_rank && ((m.from() >> 3) != (disambig_rank - '1'))) continue;
		return m;
	}
	printBitBoards();
	std::cout << boardString() << "\n";
	std::cout << ep_square;
	throw std::invalid_argument("No matching legal move found for SAN: " + san);
}

std::string Board::sanFromMove(Move move) {
	std::string san;
	uint8_t from = move.from();
	uint8_t to = move.to();
	uint8_t piece = move.piece();
	uint8_t captured = move.captured();
	uint8_t promotion = move.promotion();

	// Handle castling  
	if (piece == eKing && abs((int)to - (int)from) == 2) {
		if (to > from) {
			return "O-O"; // King-side castling  
		}
		else {
			return "O-O-O"; // Queen-side castling  
		}
	}

	// Determine piece type  
	char piece_char = (piece == ePawn) ? '\0' : " PNBRQK"[piece];

	// Handle pawn moves  
	if (piece == ePawn) {
		if (captured != eNone) {
			san += static_cast<char>('a' + (from & 7)); // File of the pawn  
		}
	}
	else {
		san += piece_char;
	}

	// Handle disambiguation
	std::vector<Move> moves;
	genPseudoLegalMoves(moves);
	
	genLegalMoves(moves);
	bool fileAmbiguity = false, rankAmbiguity = false;
	for (const auto& m : moves) {
		if (m.to() == to && m.piece() == piece && m.from() != from) {
			if ((m.from() & 7) == (from & 7)) fileAmbiguity = true;
			if ((m.from() >> 3) == (from >> 3)) rankAmbiguity = true;
		}
	}
	if (fileAmbiguity && rankAmbiguity) {
		san += static_cast<char>('a' + (from & 7));
		san += static_cast<char>('1' + (from >> 3));
	}
	else if (fileAmbiguity) {
		san += static_cast<char>('a' + (from & 7));
	}
	else if (rankAmbiguity) {
		san += static_cast<char>('1' + (from >> 3));
	}

	// Handle captures  
	if (captured != eNone) {
		san += 'x';
	}

	// Add destination square  
	san += static_cast<char>('a' + (to & 7));
	san += static_cast<char>('1' + (to >> 3));

	// Handle promotion  
	if (promotion != eNone) {
		san += '=';
		san += "NBRQ"[promotion - eKnight];
	}

	// Handle check or checkmate  
	// Assuming a function `isCheck()` and `isCheckmate()` exist  
	/*  
	if (isCheckmate()) {  
	    san += '#';  
	} else if (isCheck()) {  
	    san += '+';  
	}  
	*/
	return san;
}

void Board::genPseudoLegalCaptures(std::vector<Move>& moves) {
	const int them = us ^ 1;

	u64 our_occ = boards[us][0];
	u64 their_occ = boards[them][0];
	u64 all_occ = our_occ | their_occ;

	// PAWNS
	u64 pawns = boards[us][ePawn];
	int forward = (us == eWhite) ? 8 : -8;
	int promo_rank = (us == eWhite) ? 6 : 1;

	// Single pushes
	u64 single_push = (us == eWhite) ? (pawns << 8) : (pawns >> 8);
	single_push &= ~all_occ;

	// Pawn captures
	u64 left_captures = (us == eWhite) ? ((pawns & ~BB::files[0]) << 7) : ((pawns & ~BB::files[0]) >> 9);
	u64 right_captures = (us == eWhite) ? ((pawns & ~BB::files[7]) << 9) : ((pawns & ~BB::files[7]) >> 7);

	left_captures &= their_occ;
	right_captures &= their_occ;

	while (left_captures) {
		unsigned long to;
		BB::bitscan_reset(to, left_captures);
		int from = to - ((us == eWhite) ? 7 : -9);
		if ((from >> 3) == promo_rank) {
			for (int promo = eKnight; promo <= eQueen; ++promo)
				moves.emplace_back(from, to, ePawn, piece_board[to], promo);
		}
		else {
			moves.emplace_back(from, to, ePawn, piece_board[to]);
		}
	}
	while (right_captures) {
		unsigned long to;
		BB::bitscan_reset(to, right_captures);
		int from = to - ((us == eWhite) ? 9 : -7);
		if ((from >> 3) == promo_rank) {
			for (int promo = eKnight; promo <= eQueen; ++promo)
				moves.emplace_back(from, to, ePawn, piece_board[to], promo);
		}
		else {
			moves.emplace_back(from, to, ePawn, piece_board[to]);
		}
	}
}

void Board::genPseudoLegalMoves(std::vector<Move>& moves)  {
	const int them = us ^ 1;

	u64 our_occ = boards[us][0];
	u64 their_occ = boards[them][0];
	u64 all_occ = our_occ | their_occ;

	// PAWNS
	u64 pawns = boards[us][ePawn];
	int forward = (us == eWhite) ? 8 : -8;
	int promo_rank = (us == eWhite) ? 6 : 1;

	// Single pushes
	u64 single_push = (us == eWhite) ? (pawns << 8) : (pawns >> 8);
	single_push &= ~all_occ;

	u64 attacks = single_push;

	genPseudoLegalCaptures(moves);

	while (attacks) {
		unsigned long to;
		BB::bitscan_reset(to, attacks);
		int from = to - forward;
		if ((from >> 3) == promo_rank) {
			// Promotions
			for (int promo = eKnight; promo <= eQueen; ++promo)
				moves.emplace_back(from, to, ePawn, eNone, promo);
		}
		else {
			moves.emplace_back(from, to, ePawn);
		}
	}

	// Double pushes
	u64 double_push = (us == eWhite ? (single_push << 8) : (single_push >> 8)) & ~all_occ;
	attacks = double_push;
	while (attacks) {
		unsigned long to;
		BB::bitscan_reset(to, attacks);
		int from = to + (us == eWhite ? -16 : 16);
		if ((us == eWhite && (from >> 3 == 1)) || (us == eBlack && (from >> 3) == 6)){
			moves.emplace_back(from, to, ePawn);
		}
	}

	
	if (ep_square != -1) {
		int ep_from = ep_square + (us == eWhite ? -8 : 8);
		// Left capture
		if ((ep_square & 7) > 0 && (pawns & BB::set_bit(ep_from - 1))) {
			moves.emplace_back(ep_from - 1, ep_square, ePawn, ePawn, eNone, true);
		}
		// Right capture
		if ((ep_square & 7) < 7 && (pawns & BB::set_bit(ep_from + 1))) {
			moves.emplace_back(ep_from + 1, ep_square, ePawn, ePawn, eNone, true);
		}
	}

	// KNIGHTS
	for (int from : BB::get_set_bits(boards[us][eKnight])) {
		attacks = BB::knight_attacks[from] & ~our_occ;
		unsigned long to;
		while (attacks) {
			BB::bitscan_reset(to, attacks);
			moves.emplace_back(from, to, eKnight, piece_board[to]);
		}
	}

	// BISHOPS
	for (int from : BB::get_set_bits(boards[us][eBishop])) {
		attacks = BB::get_bishop_attacks(from, all_occ) & ~our_occ;
		unsigned long to;
		while (attacks) {
			BB::bitscan_reset(to,attacks);
			moves.emplace_back(from, to, eBishop, piece_board[to]);
		}
	}

	// ROOKS
	for (int from : BB::get_set_bits(boards[us][eRook])) {
		attacks = BB::get_rook_attacks(from, all_occ) & ~our_occ;
		unsigned long to;
		while (attacks) {
			BB::bitscan_reset(to, attacks);
			moves.emplace_back(from, to, eRook, piece_board[to]);
		}
	}

	// QUEENS
	for (int from : BB::get_set_bits(boards[us][eQueen])) {
		attacks = BB::get_queen_attacks(from, all_occ) & ~our_occ;
		unsigned long to;
		while (attacks) {
			BB::bitscan_reset(to, attacks);
			moves.emplace_back(from, to, eQueen, piece_board[to]);
		}
	}

	// KING
	int from = BB::bitscan(boards[us][eKing]);
	assert(from != -1 && "No king found for the side to move");

	attacks = BB::king_attacks[from] & ~our_occ;
	unsigned long to;
	while (attacks) {
		BB::bitscan_reset(to, attacks);
		moves.emplace_back(from, to, eKing, piece_board[to]);
	}
	// --- Castling logic ---
	// King and rook must be on their original squares, and squares between must be empty
	// e1 = 4, h1 = 7, a1 = 0 (White)
	// e8 = 60, h8 = 63, a8 = 56 (Black)
	if (us == eWhite) {
		if ((castle_flags & wShortCastleFlag) && !(u64(0b01100000) & all_occ)) moves.emplace_back(e1, g1, eKing);
		if ((castle_flags & wLongCastleFlag) && !(u64(0b00001110) & all_occ)) moves.emplace_back(e1, c1, eKing);
	}
	else {
		if ((castle_flags & bShortCastleFlag) && !((u64(0b01100000) << 56) & all_occ)) moves.emplace_back(
			e8, g8, eKing);
		if ((castle_flags & bLongCastleFlag) && !((u64(0b00001110) << 56) & all_occ)) moves.emplace_back(
			e8, c8, eKing);
	}
}

void Board::genLegalMoves(std::vector<Move> &moves) {

	for (auto move = moves.begin(); move != moves.end();) {

		if (move->isCastle()) {
			if (getAttackers((move->to() + move->from()) / 2, us)) {
				moves.erase(move);
				continue;
			}
		}
		doMove(*move);
		// Find our king's square after the move

		us ^= 1;
		bool inCheck = isCheck();
		us ^= 1;

		undoMove();
		if (!inCheck) {
			++move;
			continue;
		} else {
			moves.erase(move);
		}
	}

}

u64 Board::getAttackers(int square) const {
	return getAttackers(square, us);
}

u64 Board::getAttackers(int square, bool side) const {
	u64 attackers = 0;
	u64 square_mask = BB::set_bit(square);
	u64 our_occ = boards[side][0];
	u64 their_occ = boards[!side][0];
	u64 all_occ = our_occ | their_occ;

	// Check pawns  
	int left_capture = (side == eBlack) ? -7 : 9;
	int right_capture = (side == eBlack) ? -9 : 7;


    if ((square % 8) != 7) 
		attackers |= BB::set_bit(square + left_capture) & boards[!side][ePawn];
    if ((square % 8) != 0)
		attackers |= BB::set_bit(square + right_capture) & boards[!side][ePawn];

	// Check knights  
	attackers |= BB::knight_attacks[square] & boards[!side][eKnight];
	//check bishops
	attackers |= BB::get_bishop_attacks(square, all_occ) & ~our_occ & boards[!side][eBishop];

	// Check rooks
	attackers |= BB::get_rook_attacks(square, all_occ) & ~our_occ & boards[!side][eRook];

	//check queen
	attackers |= BB::get_queen_attacks(square, all_occ) & ~our_occ & boards[!side][eQueen];

	// Check king  
	attackers |= BB::king_attacks[square] & boards[!side][eKing];

	return attackers;
}

bool Board::isCheck() const {
	return getAttackers(BB::bitscan(boards[us][eKing]), us);
}

int16_t Board::evalUpdate() const {
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
		uint8_t p = piece_board[at];
		uint8_t sq = at ^ 56;
		mg_val += mg_table[p][sq];
		eg_val += eg_table[p][sq];
	}

	u64 black_squares = boards[eBlack][0];
	at = 0;
	while (black_squares) {
		BB::bitscan_reset(at, black_squares);
		uint8_t p = piece_board[at];
		mg_val -= mg_table[p][at];
		eg_val -= eg_table[p][at];
	}

	out += (mg_val + (game_phase * eg_val - mg_val) / 24);

	return out;
}

void Board::runSanityChecks() const {
	if (BB::popcnt(boards[eBlack][ePawn]) > 8 || BB::popcnt(boards[eWhite][ePawn]) > 8) {
		printBitBoards();
		std::cout << boardString();
		throw std::logic_error("too many pawns!");
	}

	if (BB::popcnt(boards[eBlack][ePawn]) > 8) {
		printBitBoards();
		std::cout << boardString();
		throw std::logic_error("too many pawns!");
	}
	if (boards[eWhite][0] != (boards[eWhite][ePawn] | boards[eWhite][eKnight] | boards[eWhite][eBishop] | boards[eWhite]
		[eRook] | boards[eWhite][eQueen] | boards[eWhite][eKing])) {
		printBitBoards();
		std::cout << boardString();
		throw std::logic_error("white bitboard mismatch");
	}
	if (boards[eBlack][0] != (boards[eBlack][ePawn] | boards[eBlack][eKnight] | boards[eBlack][eBishop] | boards[eBlack]
		[eRook] | boards[eBlack][eQueen] | boards[eBlack][eKing])) {
		printBitBoards();
		std::cout << boardString();
		throw std::logic_error("black bitboard mismatch");
	}
}

