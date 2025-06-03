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
	start_time = std::clock();
	start_ply = b.ply;
	tt.clear();

	std::vector<std::pair<int, Move>> out;
	best_move = Move();
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


	for (max_depth = 2 ; max_depth < 10; max_depth++) {

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
				printPV(move.second, score);
				move.first = score;
                std::cout << std::endl;

				if (score >= beta) {
					score = beta;
				}
			}
			index++;
		}

		std::sort(out.begin(), out.end(), [](const auto& a, const auto& b) { return a.first > b.first;  });
		if (checkTime()) break;

	}
	printPV(best_move, out[0].first);
	std::cout << std::endl;
	//storeTTEntry(b.getHash(), score, b.ply);
	nodes = 0;
	if (best_move.from() == 0 && best_move.to() == 0) {
		best_move = out[0].second;
	}
	best_pv.clear();
	return best_move;
}

int Engine::alphaBeta(int alpha, int beta, int depthleft) {
	if (checkTime()) return -100000;
	nodes++;
	u64 hash_key = b.getHash();
	// Probe the transposition table
	TTEntry* entry = probeTT(hash_key);

	if (b.isCheck()) depthleft++;
	const int search_ply = b.ply - start_ply;  // Calculate ply from search depth
	if (search_ply >= MAX_PLY - 1) return b.getEval(); // Prevent array overflow
	if (depthleft <= 0) return quiesce(alpha, beta);

	// If we have a valid entry that's deep enough for our current search
	if (entry && entry->depth >= depthleft) {
		// Use the entry based on its bound type
		if (entry->type == TType::EXACT) return entry->eval;
		if (entry->type == TType::BETA && entry->eval >= beta) return entry->eval;  // Beta cutoff
		if (entry->type == TType::ALPHA && entry->eval <= alpha) return entry->eval;  // Alpha cutoff
	}

	if (b.is3fold()) return 0;
	if (b.half_move == 50) return 0;


	if (!is_pv && depthleft >= 3 && !b.isCheck()) {
		b.doMove(Move(0,0));
		const int R = 2 + (depthleft / 6);
		int null_score = -alphaBeta(-beta, -beta + 1, depthleft - 1 - R);
		b.undoMove();
		if (null_score >= beta) return beta;  // If the position is good enough even after giving opponent a free move, it's likely a beta cutoff
	}

	int best = -100000;

	// Clear this node's PV
	pv_length[search_ply] = 0;
	Move best_move;
	std::vector<Move> legal_moves;
	legal_moves.reserve(128);
	b.genPseudoLegalMoves(legal_moves);
	b.filterToLegal(legal_moves);

	// Check for mate/stalemate
	if (legal_moves.empty()) {
		return b.isCheck() ? -99999 + search_ply : 0;
	}
	
	sortMoves(legal_moves);
	int score = 0;
	for (auto& move : legal_moves) {

		b.doMove(move);
		score = -alphaBeta(-beta, -alpha, depthleft - 1);
		b.undoMove();

		if (score > best) {
			best = score;
			if (score > alpha) {
				//score improved
				alpha = score;
				best_move = move;
				//if (!move.captured()) history_table[b.us][move.from()][move.to()] += (b.ply - start_ply);
				is_pv = true;
				best_pv = b.getLastMoves(search_ply);
				best_pv_state = b.state_stack;
			}
		}
		if (score >= beta) {
			is_pv = false;
			//beta cutoff, fail high, too good
			if (!move.captured()) history_table[!b.us][move.from()][move.to()] += (b.ply-start_ply);
			storeTTEntry(b.getHash(), beta, TType::BETA , depthleft);
			return beta;
		}
	}
	if (best <= alpha) {
		is_pv = false;
		// fail-low node - none of the moves improved alpha
		storeTTEntry(b.getHash(), alpha, TType::ALPHA, depthleft);
		return alpha;
	}

	// exact score node
	storeTTEntry(b.getHash(), best, TType::EXACT, depthleft);
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
    std::vector<std::pair<int, Move>> captures;
	std::vector<std::pair<int, Move>> history_moves;
	std::vector<std::pair<int, Move>> fallback_moves;
	int piece_vals[7] = {0, 100, 320, 330, 500, 900 , 99999};
    for (auto& move : moves) {
		int eval = 0;
		if (move.captured()) {
			// MVV-LVA score: victim value * 10 - attacker value
			captures.emplace_back(1000 + (10 * piece_vals[move.captured()] - piece_vals[move.piece()]), move);
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

    std::ranges::sort(captures, [](const auto& a, const auto& b) {
        return a.first > b.first; // Sort descending by eval
    });
	std::ranges::sort(hash_moves, [](const auto& a, const auto& b) {
		return a.first > b.first; // Sort descending by eval
		});
	std::ranges::sort(history_moves, [](const auto& a, const auto& b) {
		return a.first > b.first; // Sort descending by eval
		});
	std::ranges::sort(fallback_moves, [](const auto& a, const auto& b) {
		return a.first > b.first; // Sort descending by eval
		});

	std::vector<std::pair<int, Move>> eval_moves;
	eval_moves.reserve(moves.size());

	if (captures.size()) eval_moves.insert(eval_moves.end(), captures.begin(), captures.end());
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

void Engine::storeTTEntry(u64 hash_key, int score, TType type, u8 depth) {
	const size_t MAX_TT_SIZE = 512000;
		
	if (tt.contains(hash_key)) {
		if (tt[hash_key].depth < depth) {
			//replace
			tt[hash_key] = TTEntry{ score, u8(depth), u8(current_age), u8(max_depth), type };
		}
	} else {
		if (tt.size() <= MAX_TT_SIZE) {
			tt[hash_key] = TTEntry{ score, u8(depth), u8(current_age), u8(max_depth), type };
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
	TTEntry* entry = probeTT(b.getHash());

	if (entry) {
		// Use the entry based on its bound type
		if (entry->type == TType::EXACT) return entry->eval;
		if (entry->type == TType::BETA && entry->eval >= beta) return entry->eval;  // Beta cutoff
		if (entry->type == TType::ALPHA && entry->eval <= alpha) return entry->eval;  // Alpha cutoff
	}

	int stand_pat = b.getEval();
	int best = stand_pat;
	nodes++;

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
	for (auto& move : captures) {
		if (move.captured() == eKing) return 99999 - (b.ply - start_ply);


		b.doMove(move);
		int score = -quiesce(-beta, -alpha);
		b.undoMove();

		if (score >= beta) {
			//beta cutoff, fail high, too good 
			storeTTEntry(b.getHash(), score, TType::BETA, 0);
			return score;
		}
		if (score > best) {
			best = score;
		}
		if (score > alpha) {
			alpha = score;
		}
	}
	if (best <= alpha) {
		// fail-low node - none of the moves improved alpha
		storeTTEntry(b.getHash(), alpha, TType::ALPHA, 0);
		return alpha;
	}

	// exact score node
	storeTTEntry(b.getHash(), best, TType::EXACT, 0);
	return alpha;
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
