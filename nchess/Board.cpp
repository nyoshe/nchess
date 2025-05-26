#include "Board.h"


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

void Board::doMove(Move move)  
{
    movePiece(move.from(), move.to());

	// Add move to stack, along with previous en passant square and castle flags
    state_stack.emplace_back(ep_square, castle_flags, move);

    uint8_t us = side_to_move;
	// Handle promotion  
	if (move.promotion() != eNone) {  
		boards[us][move.piece()] &= ~BB::set_bit(move.to());  
		boards[us][move.promotion()] |= BB::set_bit(move.to());
		piece_board[move.to()] = move.promotion();  
	}  

	// Handle en passant  
	if (move.isEnPassant()) {  
		uint8_t ep_capture_square = move.to() + (us == eWhite ? -8 : 8);
		boards[!us][ePawn] &= ~BB::set_bit(ep_capture_square);
		boards[!us][0] &= ~BB::set_bit(ep_capture_square);
		piece_board[ep_capture_square] = eSideNone;
	}

    if (move.piece() == eKing) {
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
    	castle_flags &= (us == eWhite) ? ~(wShortCastleMask | wLongCastleMask) : ~(bShortCastleMask | bLongCastleMask);
	}

	//modify castle flags for rook move
    if (move.piece() == eRook) {
        switch (move.from()) {
            case h1: castle_flags &= ~wShortCastleMask; break; // White king-side rook
            case a1: castle_flags &= ~wLongCastleMask; break; // White queen-side rook
            case h8: castle_flags &= ~bShortCastleMask; break; // Black king-side rook
            case a8: castle_flags &= ~bLongCastleMask; break; // Black queen-side rook
			default: break;
		}
	}

    // Update en passant square
    if (move.piece() == ePawn && std::abs((int)move.to() - (int)move.from()) == 16) {
        // Set ep_square to the square behind the pawn after a double push
        ep_square = (move.from() + move.to()) / 2;
    }
    else {
        ep_square = eNone;
    }

	// Update side to move  
	side_to_move ^= 1;  

	// Increment ply count  
	ply++;


#ifndef NDEBUG
    if (boards[eWhite][0] != (boards[eWhite][ePawn] | boards[eWhite][eKnight] | boards[eWhite][eBishop] | boards[eWhite][eRook] | boards[eWhite][eQueen] | boards[eWhite][eKing]))
    {
        printBitBoards();
        throw std::logic_error("white bitboard mismatch");
    }
    if (boards[eBlack][0] != (boards[eBlack][ePawn] | boards[eBlack][eKnight] | boards[eBlack][eBishop] | boards[eBlack][eRook] | boards[eBlack][eQueen] | boards[eBlack][eKing]))
    {
        printBitBoards();
        throw std::logic_error("black bitboard mismatch");
    }
	if (piece_board[move.from()] != eNone)
    {
        printBitBoards();
        throw std::logic_error("no piece moved!");
    }
#endif
    
}

void Board::undoMove() {
    if (state_stack.empty()) return;

    // Pop the last move
    Move move = state_stack.back().move;
    ep_square = state_stack.back().ep_square;
    castle_flags = state_stack.back().castle_flags;
    state_stack.pop_back();

    // Switch side to move back
    side_to_move ^= 1;
    uint8_t us = side_to_move;

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
    }

    // Restore captured piece (if any)
    if (captured != eNone) {
        setPiece(to, !us, captured);
    }

    // Handle en passant undo
    if (move.isEnPassant()) {
        uint8_t ep_capture_square = to + (us == eWhite ? -8 : 8);
        setPiece(ep_capture_square, !us, ePawn);
        removePiece(to); // Remove pawn that landed on the en passant square
    }
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
    boards[side_to_move][piece_board[from]] &= ~BB::set_bit(from);
    boards[side_to_move][0] &= ~BB::set_bit(from);
    boards[side_to_move][piece_board[from]] |= BB::set_bit(to);
    boards[side_to_move][0] |= BB::set_bit(to);
    //clear enemy bit
    boards[!side_to_move][piece_board[to]] &= ~BB::set_bit(to);
    boards[!side_to_move][0] &= ~BB::set_bit(to);
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
	Piece piece = Piece(piece_board[square]);
	boards[color][piece] &= ~BB::set_bit(square);
	boards[color][0] &= ~BB::set_bit(square);
	piece_board[square] = eNone;
}

Side Board::getSide(int square) const {
	return boards[eWhite][0] & BB::set_bit(square) ? eWhite : (boards[eBlack][0] & BB::set_bit(square) ? eBlack : eSideNone);
}

Move Board::moveFromSan(const std::string& san)  {
    std::string move = san;
	std::vector<Move> legal_moves = getLegalMoves();
    // Remove check/mate symbols
    if (!move.empty() && (move.back() == '+' || move.back() == '#'))
        move.pop_back();

    // Handle castling
    if (move == "O-O" || move == "0-0") {
        for (auto& m : legal_moves) {
            if (piece_board[m.from()] == eKing && abs((int)m.to() - (int)m.from()) == 2)
                return m;
        }
        throw std::invalid_argument("No legal king-side castling move found");
    }
    if (move == "O-O-O" || move == "0-0-0") {
        for (auto& m : legal_moves) {
            if (piece_board[m.from()] == eKing && abs((int)m.to() - (int)m.from()) == 2)
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
        case 'N': piece = eKnight; break;
        case 'B': piece = eBishop; break;
        case 'R': piece = eRook; break;
        case 'Q': piece = eQueen; break;
        case 'K': piece = eKing; break;
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
    bool isCapture = false;
    for (size_t i = idx; i < move.size() - 2; ++i) {
        if (move[i] == 'x') {
            isCapture = true;
        }
        else if (move[i] >= 'a' && move[i] <= 'h') {
            disambig_file = move[i];
        }
        else if (move[i] >= '1' && move[i] <= '8') {
            disambig_rank = move[i];
        }
    }

    // Find matching legal move
    for (const auto& m : legal_moves) {
        if (m.to() != to_square) continue;
        if (m.piece() != piece) continue;
        if (promotion) {
            int promo_piece = eNone;
            switch (promotion) {
            case 'N': promo_piece = eKnight; break;
            case 'B': promo_piece = eBishop; break;
            case 'R': promo_piece = eRook; break;
            case 'Q': promo_piece = eQueen; break;
            default: continue;
            }
            if (m.promotion() != promo_piece) continue;
        }
        if (isCapture && m.captured() == eNone) continue;
        if (!isCapture && m.captured() != eNone) continue;
        if (disambig_file && ((m.from() & 7) != (disambig_file - 'a'))) continue;
        if (disambig_rank && ((m.from() >> 3) != (disambig_rank - '1'))) continue;
        return m;
    }
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
       } else {  
           return "O-O-O"; // Queen-side castling  
       }  
   }  

   // Determine piece type  
   char pieceChar = (piece == ePawn) ? '\0' : " PNBRQK"[piece];  

   // Handle pawn moves  
   if (piece == ePawn) {  
       if (captured != eNone) {  
           san += static_cast<char>('a' + (from & 7)); // File of the pawn  
       }  
   } else {  
       san += pieceChar;  
   }  

   // Handle disambiguation  
   std::vector<Move> legalMoves = getLegalMoves();  
   bool fileAmbiguity = false, rankAmbiguity = false;  
   for (const auto& m : legalMoves) {  
       if (m.to() == to && m.piece() == piece && m.from() != from) {  
           if ((m.from() & 7) == (from & 7)) fileAmbiguity = true;  
           if ((m.from() >> 3) == (from >> 3)) rankAmbiguity = true;  
       }  
   }  
   if (fileAmbiguity && rankAmbiguity) {  
       san += static_cast<char>('a' + (from & 7));  
       san += static_cast<char>('1' + (from >> 3));  
   } else if (fileAmbiguity) {  
       san += static_cast<char>('a' + (from & 7));  
   } else if (rankAmbiguity) {  
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

std::vector<Move> Board::getPseudoLegalMoves() const {
    std::vector<Move> legalMoves;
    const int us = side_to_move;
    const int them = us ^ 1;

    u64 our_occ = boards[us][0];
    u64 their_occ = boards[them][0];
    u64 all_occ = our_occ | their_occ;

    // PAWNS
    u64 pawns = boards[us][ePawn];
    int forward = (us == eWhite) ? 8 : -8;
    int start_rank = (us == eWhite) ? 1 : 6;
    int promo_rank = (us == eWhite) ? 6 : 1;
    int ep_rank = (us == eWhite) ? 4 : 3;

    // Single pushes
    u64 single_push = (us == eWhite) ? (pawns << 8) : (pawns >> 8);
    single_push &= ~all_occ;
    for (int to : BB::get_set_bits(single_push)) {
        int from = to - forward;
        if ((from >> 3) == promo_rank) {
            // Promotions
            for (int promo = eKnight; promo <= eQueen; ++promo)
                legalMoves.emplace_back(from, to, ePawn, eNone, promo);
        } else {
            legalMoves.emplace_back(from, to, ePawn);
        }
    }

    // Double pushes
    u64 double_push = (us == eWhite ? (single_push << 8) : (single_push >> 8)) & ~all_occ;
    for (int to : BB::get_set_bits(double_push)) {
        int from = to + (us == eWhite ? -16 : 16);
        legalMoves.emplace_back(from, to, ePawn);
    }
	

    // Pawn captures
    u64 left_captures = (us == eWhite) ? ((pawns & ~BB::files[0]) << 7) : ((pawns & ~BB::files[7]) >> 9);
    u64 right_captures = (us == eWhite) ? ((pawns & ~BB::files[7]) << 9) : ((pawns & ~BB::files[0]) >> 7);

    left_captures &= their_occ;
    right_captures &= their_occ;

    for (int to : BB::get_set_bits(left_captures)) {
        int from = to - ((us == eWhite) ? 7 : -9);
        if ((from >> 3) == promo_rank) {
            for (int promo = eKnight; promo <= eQueen; ++promo)
                legalMoves.emplace_back(from, to, ePawn, piece_board[to], promo);
        } else {
            legalMoves.emplace_back(from, to, ePawn, piece_board[to]);
        }
    }
    for (int to : BB::get_set_bits(right_captures)) {
        int from = to - ((us == eWhite) ? 9 : -7);
        if ((from >> 3) == promo_rank) {
            for (int promo = eKnight; promo <= eQueen; ++promo)
                legalMoves.emplace_back(from, to, ePawn, piece_board[to], promo);
        } else {
            legalMoves.emplace_back(from, to, ePawn, piece_board[to]);
        }
    }

    // En passant (if you track ep square, add logic here)
    if (ep_square != eNone) {
        int ep_from = ep_square + (us == eWhite ? -8 : 8);
        if (piece_board[ep_square] == ePawn && (pawns & BB::set_bit(ep_from))) {
            // Left capture
            if ((ep_square & 7) > 0 && (pawns & BB::set_bit(ep_from - 1))) {
                legalMoves.emplace_back(ep_from, ep_square, ePawn, piece_board[ep_square], eNone, true);
            }
            // Right capture
            if ((ep_square & 7) < 7 && (pawns & BB::set_bit(ep_from + 1))) {
                legalMoves.emplace_back(ep_from, ep_square, ePawn, piece_board[ep_square], eNone, true);
            }
        }
    }

    // KNIGHTS
    for (int from : BB::get_set_bits(boards[us][eKnight])) {
        u64 attacks = BB::knight_attacks[from] & ~our_occ;
        for (int to : BB::get_set_bits(attacks)) {
            legalMoves.emplace_back(from, to, eKnight, piece_board[to]);
        }
    }

    // BISHOPS
    for (int from : BB::get_set_bits(boards[us][eBishop])) {
        u64 attacks = BB::get_bishop_attacks(from, all_occ) & ~our_occ;
        for (int to : BB::get_set_bits(attacks)) {
            legalMoves.emplace_back(from, to, eBishop, piece_board[to]);
        }
    }

    // ROOKS
    for (int from : BB::get_set_bits(boards[us][eRook])) {
        u64 attacks = BB::get_rook_attacks(from, all_occ) & ~our_occ;
        for (int to : BB::get_set_bits(attacks)) {
            legalMoves.emplace_back(from, to, eRook, piece_board[to]);
        }
    }

    // QUEENS
    for (int from : BB::get_set_bits(boards[us][eQueen])) {
        u64 attacks = BB::get_queen_attacks(from, all_occ) & ~our_occ;
        for (int to : BB::get_set_bits(attacks)) {
            legalMoves.emplace_back(from, to, eQueen, piece_board[to]);
        }
    }

    // KING

    int from = BB::get_first_bit(boards[us][eKing]);
    assert(from != -1 && "No king found for the side to move");

    u64 attacks = BB::king_attacks[from] & ~our_occ;
    for (int to : BB::get_set_bits(attacks)) {
        legalMoves.emplace_back(from, to, eKing, piece_board[to]);
    }
    // --- Castling logic ---
    // King and rook must be on their original squares, and squares between must be empty
    // e1 = 4, h1 = 7, a1 = 0 (White)
    // e8 = 60, h8 = 63, a8 = 56 (Black)
    if (us == eWhite) {
        // King-side castling
        if ((castle_flags & wShortCastleMask) && piece_board[f1] == eNone && piece_board[g1] == eNone) {
            legalMoves.emplace_back(e1, g1, eKing, eNone, eNone);
        }
        // Queen-side castling
        if ((castle_flags & wLongCastleMask) && piece_board[b1] == eNone && piece_board[c1] == eNone && piece_board[d1] == eNone) {
            legalMoves.emplace_back(e1, c1, eKing, eNone, eNone);
        }
    }
    else {
        // King-side castling
        if ((castle_flags & bShortCastleMask) && piece_board[f8] == eNone && piece_board[g8] == eNone) {
            legalMoves.emplace_back(e8, g8, eKing, eNone, eNone);
        }
        // Queen-side castling
        if ((castle_flags & bLongCastleMask) && piece_board[b8] == eNone && piece_board[c8] == eNone && piece_board[d8] == eNone) {
            legalMoves.emplace_back(e8, c8, eKing, eNone, eNone);
        }
    }

    return legalMoves;
}

std::vector<Move> Board::getLegalMoves() {
    std::vector<Move> legalMoves;
    std::vector<Move> pseudoMoves = getPseudoLegalMoves();

    for (const Move& move : pseudoMoves) {
        doMove(move);
        // Find our king's square after the move
        unsigned long king_square;
        _BitScanForward64(&king_square, boards[!side_to_move][eKing]);
        
        bool inCheck = false;

    	inCheck = !getAttackers(king_square, !side_to_move).empty();

        undoMove();
        if (!inCheck) {
            legalMoves.push_back(move);
        }
    }
    return legalMoves;
}

std::vector<int> Board::getAttackers(int square) const {
	return getAttackers(square, side_to_move);
}

std::vector<int> Board::getAttackers(int square, bool side) const {
	std::vector<int> attackers;  
	u64 square_mask = BB::set_bit(square);  
	u64 our_occ = boards[side][0];
	u64 their_occ = boards[!side][0];
	u64 all_occ = our_occ | their_occ;  

	// Check pawns  
	int forward = (side == eWhite) ? 8 : -8;  
	int left_capture = (side == eWhite) ? -7 : 9;  
	int right_capture = (side == eWhite) ? -9 : 7;  
	if ((square & 7) > 0 && (their_occ & BB::set_bit(square + left_capture)) && (square + left_capture >= 0 && square + left_capture < 64)) {  
	   attackers.push_back(square + left_capture);  
	}  
	if ((square & 7) < 7 && (their_occ & BB::set_bit(square + right_capture)) && (square + right_capture >= 0 && square + right_capture < 64)) {  
	   attackers.push_back(square + right_capture);  
	}  

	// Check knights  
	u64 knights = boards[!side][eKnight];  
	for (int from : BB::get_set_bits(knights)) {  
	   if (BB::knight_attacks[from] & square_mask) {  
	       attackers.push_back(from);  
	   }  
	}  

	// Check bishops and queens  
	u64 bishops = boards[!side][eBishop] | boards[!side][eQueen];  
	for (int from : BB::get_set_bits(bishops)) {  
	   if (BB::get_bishop_attacks(from, all_occ) & square_mask) {  
	       attackers.push_back(from);  
	   }  
	}  

	// Check rooks and queens  
	u64 rooks = boards[!side][eRook] | boards[!side][eQueen];  
	for (int from : BB::get_set_bits(rooks)) {  
	   if (BB::get_rook_attacks(from, all_occ) & square_mask) {  
	       attackers.push_back(from);  
	   }  
	}  

	// Check king  
	u64 king = boards[!side][eKing];  
	int king_square = BB::get_set_bits(king).empty() ? -1 : BB::get_set_bits(king)[0];  
	if (king_square != -1 && (BB::king_attacks[king_square] & square_mask)) {  
	   attackers.push_back(king_square);  
	}  

	return attackers;  

}
