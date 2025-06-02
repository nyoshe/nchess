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
	start_time = std::clock();
	start_ply = b.ply;


	std::vector<std::pair<int, Move>> out;
	best_move = Move();
	std::vector<Move> legal_moves;
	b.genPseudoLegalMoves(legal_moves);
	b.filterToLegal(legal_moves);
	sortMovesByEval(legal_moves);
	calcTime();

	for (auto move : legal_moves) {
		out.push_back({ -100000, move });
	}

	max_depth = 1;
	int score = alphaBeta(-100000, 100000, 1);
	int alpha = score - 50;
	int beta = score + 50;

	for (max_depth = 2 ; max_depth < 10; max_depth++) {
		updateTTAge();
		alpha = score - 50;
		beta = score + 50;

		for (int i = 0; i < MAX_PLY; i++) pv_length[i] = 0;

		score = alphaBeta(alpha, beta, max_depth);

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
	if (b.isCheck()) depthleft++;
	const int search_ply = b.ply - start_ply;  // Calculate ply from search depth
	if (search_ply >= MAX_PLY - 1) return b.getEval(); // Prevent array overflow
	if (depthleft <= 0) return quiesce(alpha, beta);


	int best = -100000;

	// Clear this node's PV
	pv_length[search_ply] = 0;

	std::vector<Move> legal_moves;
	legal_moves.reserve(128);
	b.genPseudoLegalMoves(legal_moves);
	b.filterToLegal(legal_moves);

	// Check for mate/stalemate
	if (legal_moves.empty()) {
		return b.isCheck() ? -99999 + search_ply : 0;
	}

	sortMovesByEval(legal_moves);

	for (auto& move : legal_moves) {

		b.doMove(move);

		int score = -alphaBeta(-beta, -alpha, depthleft - 1);

		b.undoMove();

		if (score > best) {
			best = score;
			if (score > alpha) {
				if (max_depth >= best_pv.size()) {
					best_pv = b.getLastMoves(search_ply);
				}
				alpha = score;
			}
		}
		if (score >= beta) {
			return best;
		}
	}
	//storeTTEntry(b.getHash(), alpha, b.ply);
	return best;

}


std::vector<std::pair<int, Move>> Engine::sortMovesByEval(std::vector<Move>& moves) {
    // Store original board state to restore after each move
    std::vector<std::pair<int, Move>> eval_moves;
    eval_moves.reserve(moves.size());
	int piece_vals[7] = {0, 100, 320, 330, 500, 900 , 99999};
    for (auto& move : moves) {
		int eval = 0;
		if (move.captured()) {
			// MVV-LVA score: victim value * 10 - attacker value
			eval_moves.emplace_back(1000 + (10 * piece_vals[move.captured()] - piece_vals[move.piece()]), move);
			continue;
		}

		b.doMove(move);
		eval = -b.getEval();
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

std::vector<Move> Engine::getPrincipalVariation() const {
	std::vector<Move> pv;
	for (int i = 0; i < pv_length[0]; i++) {
		pv.push_back(pv_table[0][i]);
	}
	return pv;
}

void Engine::printPV(Move root_move, int score) const {
	// Print the principal variation from root position (depth 0)
	std::cout << "info score cp " << score << " depth " << best_pv.size() << " nodes " << nodes << " max_time " << max_time <<  " given_time " << (start_ply % 2 ? tc.btime : tc.wtime) << " time " <<
		(1000.0 * (std::clock() - start_time) / CLOCKS_PER_SEC) << " pv ";
	for (auto pv_move : std::views::reverse(best_pv)) {
		std::cout << pv_move.toUci() << " ";
	}
}

void Engine::pruneTT(size_t max_size) {
	if (tt.size() <= max_size) return;

	// Create temporary map for entries we want to keep
	std::unordered_map<u64, TTEntry> new_tt;
	new_tt.reserve(max_size);

	// Sort entries by importance (depth and age)
	std::vector<std::pair<u64, TTEntry>> entries;
	entries.reserve(tt.size());

	for (const auto& [key, entry] : tt) {
		entries.emplace_back(key, entry);
	}

	// Sort by depth (primary) and age (secondary)
	std::sort(entries.begin(), entries.end(),
	          [](const auto& a, const auto& b) {
		          if (a.second.depth != b.second.depth)
			          return a.second.depth > b.second.depth;
		          return a.second.age > b.second.age;
	          });

	// Keep only the most valuable entries
	for (size_t i = 0; i < max_size && i < entries.size(); ++i) {
		new_tt[entries[i].first] = entries[i].second;
	}

	tt = std::move(new_tt);
}

void Engine::storeTTEntry(u64 hash_key, int16_t eval, u8 depth) {
	const size_t MAX_TT_SIZE = 1000000; // Adjust based on your needs

	if (tt.size() >= MAX_TT_SIZE) {
		pruneTT(MAX_TT_SIZE * 0.75); // Reduce to 75% of max size
	}

	tt[hash_key] = TTEntry{eval, u16(b.ply), u16(current_age), max_depth /* current age */ };
}

void Engine::updateTTAge() {
	static u16 current_age = 0;
	current_age++;

	for (auto& [key, entry] : tt) {
		entry.age = current_age;
	}
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
	int stand_pat = b.getEval();
	int score = stand_pat;
	nodes++;

	//delta prune
	if (score < alpha - 950) return alpha;

	if (score >= beta) {
		return beta;
	}

	if (alpha < score) {
		alpha = score;
	}

	std::vector<Move> captures;
	captures.reserve(32);
	b.genPseudoLegalCaptures(captures);
	b.filterToLegal(captures);
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

		//if (tt.contains(b.getHash())) {
		//	score = tt[b.getHash()].eval;
		//} else {
		score = -quiesce(-beta, -alpha);
		//s}

		b.undoMove();

		if (score > alpha) {
			if (score >= beta)
				return beta;
			//storeTTEntry(b.getHash(), score, b.ply);
			alpha = score;
		}
	}
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
