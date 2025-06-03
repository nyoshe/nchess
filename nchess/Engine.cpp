#include "Engine.h"

#include <algorithm>

void Engine::perftSearch(int d) {

	if (!d) return;
	std::vector<Move> legal_moves;
	b.genPseudoLegalMoves(legal_moves);
	b.filterToLegal(legal_moves);

	if (legal_moves.size() == 0) {
		perf_values[max_depth - d].checkmates ++;
		return;
	}
	for (auto& move : legal_moves) {
		int index = max_depth - d;
		
		if (max_depth != d){
			pos_count++;
		}
		perf_values[index].depth = index + 1;
		perf_values[index].nodes++;
		perf_values[index].captures += static_cast<bool>(move.captured());
		perf_values[index].en_passant += move.isEnPassant();
		perf_values[index].castles += move.isCastle();
		perf_values[index].promotions += static_cast<bool>(move.promotion());

		b.doMove(move);

		perf_values[index].checks += b.isCheck();

		perftSearch(d - 1);

		b.undoMove();

		if (max_depth == d) {
			std::cout << move.toUci() << ": " << pos_count << ", ";
			pos_count = 0;
		}
	}
}

Move Engine::search(int depth) {
	for (auto& i : history_table) {
		for (auto& j : i) {
			for (auto& k : j) {
				k = 0;
			}
		}
	}
	for (int i = 0; i < MAX_PLY; i++) {
		pv_length[i] = 0;
	}
	
	start_time = std::clock();
	start_ply = b.ply;
	cleanupTT();

	std::vector<std::pair<int, Move>> out;
	Move best_move;
	std::vector<Move> legal_moves;
	b.genPseudoLegalMoves(legal_moves);
	b.filterToLegal(legal_moves);
	sortMoves(legal_moves);
	calcTime();

	for (auto move : legal_moves) {
		out.push_back({ -100000, move });
	}

	max_depth = 1;
	int score = alphaBeta(-100000, 100000, 1);


	for (max_depth = 2 ; max_depth < 20; max_depth++) {

		// Start with narrow aspiration window
		int alpha = score - 50;
		int beta = score + 50;
		int delta = 50;

		// Keep searching until we get a score within our window
		while (true) {
			score = alphaBeta(alpha, beta, max_depth);

			// If we run out of time, stop searching
			if (checkTime()) break;

			// If the score is within our window, break
			if (score > alpha && score < beta)
				break;

			// If we fail low, widen the window downward
			if (score <= alpha) {
				alpha = max(-100000, alpha - delta);
				delta *= 2; // Exponentially increase window size
			}
			// If we fail high, widen the window upward 
			else if (score >= beta) {
				beta = min(100000, beta + delta);
				delta *= 2; // Exponentially increase window size
			}

			// If we've fully widened the window, no need to retry
			if (alpha <= -100000 && beta >= 100000)
				break;
		}

		int index = 0;

		for (auto& move : out) {

			b.doMove(move.second);
			score = -alphaBeta(-beta, -alpha, max_depth);
			b.undoMove();
			if (checkTime()) break;

			move.first = score;

			if (score > alpha) {
                alpha = score;
				best_move = move.second;
				move.first = score;
                

				if (score >= beta) {
					score = beta;
				}
			}
			index++;
		}

		std::sort(out.begin(), out.end(), [](const auto& a, const auto& b) { return a.first > b.first;  });
		if (checkTime()) break;
		printPV(best_move, out[0].first);
		std::cout << std::endl;
	}
	
	std::cout << std::endl;
	//storeTTEntry(b.getHash(), score, b.ply);
	nodes = 0;
	if (best_move.from() == 0 && best_move.to() == 0) {
		best_move = out[0].second;
	}
	return best_move;
}

int Engine::alphaBeta(int alpha, int beta, int depthleft) {
	pv_length[b.ply - start_ply] = 0;
	if (checkTime()) return -100000;
	if (b.is3fold()) return 0;
	if (b.half_move == 50) return 0;
	if (depthleft <= 0) return quiesce(alpha, beta);
	nodes++;
	u64 hash_key = b.getHash();
	// Probe the transposition table
	TTEntry* entry = probeTT(hash_key);
	bool in_check = b.isCheck();
	if (in_check) depthleft++;
	const int search_ply = b.ply - start_ply;  // Calculate ply from search depth
	if (search_ply >= MAX_PLY - 1) return b.getEval(); // Prevent array overflow
	

	// If we have a valid entry that's deep enough for our current search
	if (entry && entry->depth >= depthleft) {
		// Use the entry based on its bound type
		if (entry->type == TType::EXACT) return entry->eval;
		if (entry->type == TType::BETA && entry->eval >= beta) return entry->eval;  // Beta cutoff
		if (entry->type == TType::ALPHA && entry->eval <= alpha) return entry->eval;  // Alpha cutoff
	}
	

	if (!is_pv && depthleft >= 3 && !in_check && (b.getEval() + 50) > beta) {
		b.doMove(Move(0,0));
		const int R = 2 + (depthleft / 6);
		int null_score = -alphaBeta(-beta, -beta + 1, depthleft - 1 - R);
		b.undoMove();
		if (null_score >= beta) return beta;  // If the position is good enough even after giving opponent a free move, it's likely a beta cutoff
	}

	int score = 0;
	//do tt move first
	Move best_move;

	/*
	if (entry && entry->type == TType::BEST) {
		//this might be prone to hash fuckups
		best_move = entry->best_move;
		if (best_move.raw()) {
			b.doMove(best_move);
			score = -alphaBeta(-beta, -alpha, depthleft - 1);
			b.undoMove();
		}
	}
	*/
	bool can_apply_futility =
		!in_check &&
		depthleft <= 3 &&  // Apply to depths 1, 2, and 3
		!is_pv;            // Don't apply in PV nodes

	// Futility margins increasing by depth
	const int futility_margins[4] = { 0, 100, 300, 500 };
	bool futility_prune = false;
	int futility_margin = 0;

	if (can_apply_futility) {
		futility_margin = b.getEval() + futility_margins[depthleft];
		futility_prune = (futility_margin <= alpha);
	}

	int best = -100000;

	std::vector<Move> legal_moves;
	legal_moves.reserve(128);
	b.genPseudoLegalMoves(legal_moves);
	b.filterToLegal(legal_moves);

	// Check for mate/stalemate
	if (legal_moves.empty()) {
		return in_check ? -99999 + search_ply : 0;
	}
	
	sortMoves(legal_moves);
	int i = 0;
	for (auto& move : legal_moves) {

		bool can_reduce =
			i >= 3 &&                    // Not one of the first few moves
			!move.captured() &&          // Not a capture
			!move.promotion() &&         // Not a promotion
			!in_check &&                 // Not in check
			!is_pv &&                    // Not in PV
			depthleft >= 3;              // Depth is sufficient

		if (can_reduce) {
			// R increases with depth and move index
			int R = int(0.5 + std::log(depthleft) * std::log(i) / 3.0);

			// Reduced depth search
			b.doMove(move);
			score = -alphaBeta(-alpha - 1, -alpha, depthleft - 1 - R);

			// If the reduced search indicates this might be a good move,
			// re-search with full depth
			if (score > alpha) {
				score = -alphaBeta(-beta, -alpha, depthleft - 1);
			}
			b.undoMove();
		} else {
			b.doMove(move);
			static int piece_vals[7] = { 0, 100, 320, 330, 500, 900 , 99999 };
			//futility pruning
			if (futility_prune &&
				!move.captured() &&
				!move.promotion() &&
				!move.isEnPassant() &&
				!b.isCheck()) {
				b.undoMove();
				continue;
			}
			score = -alphaBeta(-beta, -alpha, depthleft - 1);
			b.undoMove();
		}


		i++;

		if (score > best) {
			best = score;
			best_move = move;
			if (score > alpha) {
				//score improved
				alpha = score;
				//if (!move.captured()) history_table[b.us][move.from()][move.to()] += (b.ply - start_ply);
				is_pv = true;
				storeTTEntry(b.getHash(), best, TType::EXACT, depthleft, best_move);
				updatePV(b.ply - start_ply, move);
			}
			
		}
		if (score >= beta) {
			is_pv = false;
			//beta cutoff, fail high, too good
			if (!move.captured()) history_table[!b.us][move.from()][move.to()] += (b.ply-start_ply) * (b.ply - start_ply);
			storeTTEntry(b.getHash(), beta, TType::BETA , depthleft, best_move);
			return beta;
		}
	}
	if (best_move.raw() && !best_move.captured()) history_table[b.us][best_move.from()][best_move.to()] += (b.ply - start_ply) * (b.ply - start_ply);
	if (best <= alpha) {
		is_pv = false;
		// fail-low node - none of the moves improved alpha
		storeTTEntry(b.getHash(), alpha, TType::ALPHA, depthleft, best_move);
		return alpha;
	}
	// exact score node
	storeTTEntry(b.getHash(), best, TType::EXACT, depthleft, best_move);
	return best;
}
/*
std::vector<std::pair<int, Move>> Engine::sortMoves(std::vector<Move>& moves) {
	std::vector<std::pair<int, Move>> eval_moves;
	eval_moves.reserve(moves.size());
	int piece_vals[7] = { 0, 100, 320, 330, 500, 900 , 99999 };
	for (auto& move : moves) {
		int eval = 0;
		if (move.captured()) {
			// MVV-LVA score: victim value * 10 - attacker value
			eval_moves.emplace_back(1000 + (10 * piece_vals[move.captured()] - piece_vals[move.piece()]), move);
			continue;
		}

		b.doMove(move);
		// Probe the transposition table
		TTEntry* entry = probeTT(b.getHash());
		// If we have a valid entry that's deep enough for our current search, and ensure it gets searched first
		if (entry) {
			eval = -entry->eval + 10000;
		}
		else {
			eval = -b.getEval();
		}
		b.undoMove();

		eval_moves.emplace_back(eval, move);
	}

	std::sort(eval_moves.begin(), eval_moves.end(), [](const auto& a, const auto& b) {
		return a.first > b.first; // Sort descending by eval
		});

	for (size_t i = 0; i < moves.size(); ++i) {
		moves[i] = eval_moves[i].second;
	}
	return eval_moves;
}
*/

std::vector<std::pair<int, Move>> Engine::sortMoves(std::vector<Move>& moves) {
	std::vector<std::pair<int, Move>> hash_moves;
    std::vector<std::pair<int, Move>> good_captures;
	std::vector<std::pair<int, Move>> bad_captures;
	std::vector<std::pair<int, Move>> history_moves;
	std::vector<std::pair<int, Move>> fallback_moves;
	int piece_vals[7] = {0, 100, 320, 330, 500, 900 , 99999};
    for (auto& move : moves) {
		int eval = 0;
		if (move.captured()) {
			/*
			int mat = piece_vals[move.captured()] - piece_vals[move.piece()];
			//sse
			u64 attackers;
			bool side = b.us;
			while (attackers = b.getAttackers(move.to(), side)) {
				side = !side;
			}
			*/
			
			// MVV-LVA score: victim value * 10 - attacker value
			good_captures.emplace_back(piece_vals[move.captured()] * 10 - piece_vals[move.piece()], move);
			//d
			continue;
		}
		

		b.doMove(move);
		// Probe the transposition table
		TTEntry* entry = probeTT(b.getHash());
		// If we have a valid entry that's deep enough for our current search, and ensure it gets searched first
		if (entry) {
			hash_moves.emplace_back(-entry->eval, move);
		} else if (history_table[b.us][move.from()][move.to()]){
			history_moves.emplace_back(history_table[b.us][move.from()][move.to()], move);
			//fallback_moves.emplace_back(-b.getEval(), move);
		} else {
			fallback_moves.emplace_back(-b.getEval(), move);
		}
		b.undoMove();
    }
	auto func = [](const auto& a, const auto& b) {
		return a.first > b.first; // Sort descending by eval
		};

	std::ranges::sort(bad_captures, func);
    std::ranges::sort(good_captures, func);
	std::ranges::sort(hash_moves, func);
	std::ranges::sort(history_moves, func);
	std::ranges::sort(fallback_moves, func);

	std::vector<std::pair<int, Move>> eval_moves;
	eval_moves.reserve(moves.size());
	if (good_captures.size()) eval_moves.insert(eval_moves.end(), good_captures.begin(), good_captures.end());
	if (bad_captures.size()) eval_moves.insert(eval_moves.end(), bad_captures.begin(), bad_captures.end());
	if (hash_moves.size()) eval_moves.insert(eval_moves.end(), hash_moves.begin(), hash_moves.end());
	if (history_moves.size()) eval_moves.insert(eval_moves.end(), history_moves.begin(), history_moves.end());
	if (fallback_moves.size()) eval_moves.insert(eval_moves.end(), fallback_moves.begin(), fallback_moves.end());

	for (size_t i = 0; i < moves.size(); ++i) {
        moves[i] = eval_moves[i].second;
    }
	return eval_moves;
}

std::vector<Move> Engine::getPrincipalVariation() const {
	std::vector<Move> pv;
	for (int i = 0; i < pv_length[0]; i++) {
		pv.push_back(pv_table[0][i]);
	}
	return pv;
}

void Engine::printPV(Move root_move, int score) const {
	// Print the principal variation from root position (depth 0)
	std::vector<Move> pv = getPrincipalVariation();

	std::cout << "info score cp " << score << " depth " << max_depth
		<< " nodes " << nodes << " max_time " << max_time
		<< " given_time " << (start_ply % 2 ? tc.btime : tc.wtime)
		<< " time " << (1000.0 * (std::clock() - start_time) / CLOCKS_PER_SEC)
		<< " hash_hits: " << hash_hits
		<< " hash_miss: " << hash_miss
		<< " pv ";
	/*
	for (const auto& pv_move : std::views::reverse(best_pv)) {
		std::cout << pv_move.toUci() << " ";
	}
	*/

	for (const auto& pv_move : pv) {
		std::cout << pv_move.toUci() << " ";
	}
}

void Engine::storeTTEntry(u64 hash_key, int score, TType type, u8 depth_left, Move best) {
	const size_t MAX_TT_SIZE = 512000;
		
	if (tt.contains(hash_key)) {
		if (tt[hash_key].depth <= depth_left || tt[hash_key].ply < start_ply) {
			//replace
			tt[hash_key] = TTEntry{ score, u8(depth_left), u16(start_ply), u8(max_depth), type, best};
		}
	} else {
		if (tt.size() <= MAX_TT_SIZE) {
			tt[hash_key] = TTEntry{ score, u8(depth_left), u16(start_ply), u8(max_depth), type, best };
		}
	}
	/*
	for (auto [key, entry] : tt) {
		entry.age = current_age;
	}
	*/
}


bool Engine::checkTime() {
	if ((1000.0 * (std::clock() - start_time) / CLOCKS_PER_SEC) > max_time) return true;
	return false;
}

void Engine::calcTime() {
	if (tc.movetime) {
		max_time = tc.movetime;
		return;
	}
	std::vector<Move> legal_moves;
	b.genPseudoLegalMoves(legal_moves);
	b.filterToLegal(legal_moves);

	float num_moves = legal_moves.size();
	float factor = num_moves / 500.0;

	float total_time = b.us ? tc.btime : tc.wtime;
	float inc = b.us ? tc.binc : tc.winc;

	if (total_time < inc) {
		max_time = inc * 0.95;
	} else {
		max_time = (total_time * factor) + inc * 0.95;
	}
	//max_time = inc;
}

int Engine::quiesce(int alpha, int beta) {
	if (checkTime()) return -100000;
	if (b.is3fold()) return 0;
	if (b.half_move == 50) return 0;
	TTEntry* entry = probeTT(b.getHash());

	if (entry) {
		// Use the entry based on its bound type
		if (entry->type == TType::EXACT) return entry->eval;
		if (entry->type == TType::BETA && entry->eval >= beta) return entry->eval;  // Beta cutoff
		if (entry->type == TType::ALPHA && entry->eval <= alpha) return entry->eval;  // Alpha cutoff
	}

	int stand_pat = b.getEval();
	int best = stand_pat;


	//delta prune
	if (stand_pat < alpha - 950) return alpha;

	if (stand_pat >= beta) {
		return beta;
	}

	if (alpha < stand_pat) {
		alpha = stand_pat;
	}

	std::vector<Move> captures;
	captures.reserve(32);
	b.genPseudoLegalCaptures(captures);
	b.filterToLegal(captures);
	sortMoves(captures);

	// Check for #M
	if (!captures.size()) {
		if (b.isCheck()) {
			b.genPseudoLegalMoves(captures);
			b.filterToLegal(captures);
			if (!captures.size()) {
				return -99999 + b.ply - start_ply;
			}
		}
	}
	Move best_move;
	for (auto& move : captures) {
		if (move.captured() == eKing) return 99999 - (b.ply - start_ply);


		b.doMove(move);
		int score = -quiesce(-beta, -alpha);
		b.undoMove();

		if (score >= beta) {
			//beta cutoff, fail high, too good 
			//storeTTEntry(b.getHash(), score, TType::BETA, 0, best_move);
			return score;
		}
		if (score > best) {
			best = score;
			best_move = move;
		}
		if (score > alpha) {
			alpha = score;
		}
	}
	if (best <= alpha) {
		// fail-low node - none of the moves improved alpha
		//storeTTEntry(b.getHash(), alpha, TType::ALPHA, 0, best_move);
		return alpha;
	}

	// exact score node
	//storeTTEntry(b.getHash(), best, TType::EXACT, 0, best_move);
	return best;
}

std::vector<PerfT> Engine::doPerftSearch(int depth) {
	perf_values.clear();
	perf_values.resize(depth);
	start_time = std::clock();
	max_depth = depth;
	perftSearch(depth);
	// Returns elapsed time in milliseconds
	std::cout << "search time: " << static_cast<int>(1000.0 * (std::clock() - start_time) / CLOCKS_PER_SEC) << "ms\n\n";

	return perf_values;
}

void Engine::setBoardFEN(std::istringstream& fen) {
	b.loadFen(fen);
}

void Engine::setBoardUCI(std::istringstream& uci) {
	b.loadUci(uci);
}

std::vector<PerfT> Engine::doPerftSearch(std::string position, int depth) {
	std::istringstream iss(position);
	b.loadFen(iss);
	return doPerftSearch(depth);
}
