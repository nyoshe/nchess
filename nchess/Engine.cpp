#include "Engine.h"

#include <algorithm>

void Engine::perftSearch(int d) {

	if (!d) return;
	StaticVector<Move> legal_moves;
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
/**/
Move Engine::search(int depth) {
	for (auto& i : pv_table) {
		for (auto& j : i) {
			j = Move();
		}
	}

	for (int i = 0; i < MAX_PLY; i++) {
		pv_length[i] = 0;
	}
	pv_moves.clear();
	nodes = 0;
	start_time = std::clock();
	start_ply = b.ply;

	StaticVector<Move> legal_moves;
	b.genPseudoLegalMoves(legal_moves);
	b.filterToLegal(legal_moves);

	calcTime();
	Move best_move = legal_moves.front();
	std::vector<std::pair<int, Move>> sorted_moves;
	for (auto move : legal_moves) {
		sorted_moves.push_back({ -100000, move });
	}

	max_depth = 1;
	int score = alphaBeta(-100000, 100000, max_depth, false);

	for (max_depth = 1; max_depth < MAX_PLY; max_depth++) {

		// Start with narrow aspiration window
		int alpha = score - 50;
		int beta = score + 50;
		int delta = 50;

		// Keep searching until we get a score within our window
		while (true) {
			score = alphaBeta(alpha, beta, max_depth, false);
			if (checkTime()) break;
			if (score > alpha && score < beta) break;
			if (score <= alpha) {
				alpha = std::max(-100000, alpha - delta);
				delta *= 2; // Exponentially increase window size
			}
			else if (score >= beta) {
				beta = std::min(100000, beta + delta);
				delta *= 2; // Exponentially increase window size
			}
			if (alpha <= -100000 && beta >= 100000) break;
		}

		pv_length[0] = 0;
		int index = 0;

		for (auto& move : sorted_moves) {

			b.doMove(move.second);
			if (index == 0) {
				score = -alphaBeta(-beta, -alpha, max_depth, true);
			}
			else {
				score = -alphaBeta(-alpha - 1, -alpha, max_depth, false);
				if (alpha < score && score < beta) {
					score = -alphaBeta(-beta, -alpha, max_depth, true);
				}
			}
			b.undoMove();
			if (checkTime()) break;

			move.first = score;


			if (score > alpha) {
				alpha = score;
				best_move = move.second;
				move.first = score;

				b.doMove(best_move);
				if (!b.is3fold()) updatePV(b.ply - start_ply - 1, best_move);
				b.undoMove();
				if (score >= beta) {
					score = beta;
				}
			}
			index++;
		}

		std::sort(sorted_moves.begin(), sorted_moves.end(), [](const auto& a, const auto& b) { return a.first > b.first;  });
		if (checkTime()) break;
		printPV(alpha);

	}
	if (best_move.from() == 0 && best_move.to() == 0) {
		best_move = pv_table[0][0];
	}

	return best_move;
}
/*
Move Engine::search(int depth) {
	for (auto& i : history_table) {
		for (auto& j : i) {
			for (auto& k : j) {
				k = 0;
			}
		}
	}
	for (auto& i : pv_table) {
		for (auto& j : i) {
			j = Move();
		}
	}

	for (int i = 0; i < MAX_PLY; i++) {
		pv_length[i] = 0;
	}
	pv_moves.clear();
	nodes = 0;
	start_time = std::clock();
	start_ply = b.ply;


	
	std::vector<Move> legal_moves;
	b.genPseudoLegalMoves(legal_moves);
	b.filterToLegal(legal_moves);
	sortMoves(legal_moves);
	calcTime();
	Move best_move = legal_moves.front();
	std::vector<std::pair<int, Move>> sorted_moves;
	for (auto move : legal_moves) {
		sorted_moves.push_back({ -100000, move });
	}

	max_depth = 1;
	//int score = alphaBeta(-100000, 100000, max_depth, false);
	int score = -100000;
	
	int delta = 50;
	for (max_depth = 1 ; max_depth < 32; max_depth++) {
		//pv_length[0] = 0;
		// Start with narrow aspiration window
		int alpha = -100000;
		int beta = 100000;
		
		if (max_depth >= 3) {
			alpha = std::max(-100000, score - delta);
			beta = std::min(100000, score + delta);
		}

		// Keep searching until we get a score within our window
		while (true) {
			score = alphaBeta(alpha, beta, max_depth, true);
			if (checkTime()) break;
			if (score > alpha && score < beta) break;
			if (score <= alpha) {
				alpha = std::max(-100000, alpha - delta);
				delta *= 2; // Exponentially increase window size
			}
			if (score >= beta) {
				beta = std::min(100000, beta + delta);
				delta *= 2; // Exponentially increase window size
			}
			if (alpha <= -100000 && beta >= 100000) break;
		}

		
		int index = 0;


		//score = alphaBeta(alpha, beta, max_depth, true);

		index++;

		/*
		for (auto& move : sorted_moves) {

			b.doMove(move.second);
			if (index == 0) {
				score = -alphaBeta(-beta, -alpha, max_depth, true);
			}
			else {
				score = -alphaBeta(-alpha - 1, -alpha, max_depth, false);
				if (alpha < score && score < beta) {
					score = -alphaBeta(-beta, -alpha, max_depth, true);
				}
			}
			b.undoMove();
			if (checkTime()) break;

			move.first = score;


			if (score > alpha) {
                alpha = score;
				best_move = move.second;
				move.first = score;
                
				b.doMove(best_move);
				if (!b.is3fold()) updatePV(b.ply - start_ply - 1, best_move);
				b.undoMove();
				if (score >= beta) {
					score = beta;
				}
			}
			index++;
		}
		
		//std::sort(sorted_moves.begin(), sorted_moves.end(), [](const auto& a, const auto& b) { return a.first > b.first;  });
		if (checkTime()) break;
		printPV(alpha);

	}
	if (best_move.from() == 0 && best_move.to() == 0) {
		best_move = pv_table[0][0];
	}
	return best_move;
}
*/


int Engine::alphaBeta(int alpha, int beta, int depth_left, bool is_pv) {
	const int search_ply = b.ply - start_ply;
	pv_length[search_ply] = 0;
	if (checkTime()) return -100000;
	if (b.is3fold() || b.half_move == 100) return 0;
	bool in_check = b.isCheck();
	if (depth_left == 0 && in_check) depth_left++;
	nodes++;
	if (depth_left <= 0) return quiesce(alpha, beta);
	
	//check first for hash hits
	/**/
	u64 hash_key = b.getHash();
	TTEntry entry = probeTT(hash_key);
	
	if (entry && entry.search_depth >= max_depth && entry.depth_left >= depth_left) {
		if (entry.type == TType::BETA && entry.eval >= beta) return entry.eval;
		if (entry.type == TType::ALPHA && entry.eval <= alpha) return entry.eval;
	}
	
	
	int best = -100000;
	Move best_move;
	bool can_apply_futility = !in_check && depth_left <= 3 && !is_pv;

	
	if (search_ply >= MAX_PLY - 1) return b.getEval();

	//null move pruning
	if (!is_pv && depth_left >= 3 && !in_check && (b.getEval() + 50) > beta) {
		b.doMove(Move(0,0));
		const int R = 2 + (depth_left / 6);
		int null_score = -alphaBeta(-beta, -beta + 1, depth_left - 1 - R, false);
		b.undoMove();
		if (null_score >= beta) return beta;  
	}

	// Futility margins increasing by depth
	const int futility_margins[4] = { 0, 100, 300, 500 };
	bool futility_prune = false;
	int futility_margin = 0;

	if (can_apply_futility) {
		futility_margin = b.getEval() + futility_margins[depth_left];
		futility_prune = (futility_margin <= alpha);
	}

	g_move_list[search_ply].clear();
	b.genPseudoLegalMoves(g_move_list[search_ply]);
	b.filterToLegal(g_move_list[search_ply]);
	MoveGen movegen;
	// Check for mate/stalemate
	if (g_move_list[search_ply].empty()) {
		return in_check ? -99999 + search_ply : 0;
	}
	
	//sortMoves(legal_moves);
	int i = 0;
	Move move = movegen.getNext(*this, b, g_move_list[search_ply]);
	search_calls++;
	while (move.raw()) {
		moves_inspected++;
		int score = 0;

		bool can_reduce =
			i >= 3 &&   
			!move.captured() &&
			!move.promotion() &&
			!in_check &&  
			!is_pv &&  
			depth_left >= 3;

		b.doMove(move);
		//futility pruning
		
		if (futility_prune &&
			!move.captured() &&
			!move.promotion() &&
			!move.isEnPassant() &&
			!in_check) {
			b.undoMove();
			move = movegen.getNext(*this, b, g_move_list[search_ply]);
			i++;
			continue;
		}
		
		if (can_reduce) {
			int R = int(0.5 + std::log(depth_left) * std::log(i) / 3.0);
			score = -alphaBeta(-alpha - 1, -alpha, depth_left - 1 - R, false);
		}
		

		
		else if (i == 0) {
			score = -alphaBeta(-beta, -alpha, depth_left - 1, is_pv);
		}
		else {
			score = -alphaBeta(-alpha - 1, -alpha, depth_left - 1, false);
			if (alpha < score && score < beta) {
				score = -alphaBeta(-beta, -alpha, depth_left - 1, true);
			}
		}
		
		b.undoMove();

		i++;

		if (score > best) {
			best = score;
			best_move = move;
			if (score > alpha) {
				alpha = score;
				if (is_pv) {
					b.doMove(best_move);
					if (!b.is3fold()) updatePV(b.ply - start_ply - 1, best_move);
					storeTTEntry(b.getHash(), best, TType::EXACT, depth_left, best_move);
					b.undoMove();
				}
			}
		}

		if (score >= beta) {
			if (!move.captured()) {
				// Store as killer move
				killer_moves[search_ply][1] = killer_moves[search_ply][0];
				killer_moves[search_ply][0] = move;
			}

			if (!move.captured()) history_table[!b.us][move.from()][move.to()] += depth_left * depth_left;
			storeTTEntry(b.getHash(), beta, TType::BETA , depth_left, best_move);
			return beta;
		}
		move = movegen.getNext(*this, b, g_move_list[search_ply]);
	}
	if (best <= alpha) {
		// fail-low node - none of the moves improved alpha
		storeTTEntry(b.getHash(), alpha, TType::ALPHA, depth_left, best_move);
		return alpha;
	}
	// exact score node
	if (is_pv) {
		storeTTEntry(b.getHash(), best, TType::EXACT, depth_left, best_move);
	}
	return best;
}


std::vector<Move> Engine::getPrincipalVariation() const {
	std::vector<Move> pv;
	if (pv_length[0]) {
		for (int i = 0; i < pv_length[0]; i++) {
			pv.push_back(pv_table[0][i]);
		}
	}
	return pv;
}

void Engine::printPV(int score)  {
	std::vector<Move> pv = getPrincipalVariation();
	//if (!pv.empty()) {
	std::cout << "info score cp " << score << " depth " << max_depth
		<< " nodes " << nodes
		<< " nps " << static_cast<int>((1000.0 * nodes) / (1000.0 * (std::clock() - start_time) / CLOCKS_PER_SEC));
	std::cout << " pv ";

	for (auto& move : pv) {
		std::cout << move.toUci() << " ";
	}
	std::cout << std::endl;
	//}
	
}

std::string Engine::getPV() {
		
	std::vector<Move> pv = getPrincipalVariation();
	if (!pv.empty()) {
		std::string out = "depth " + std::to_string(max_depth)
			+ " nodes " + std::to_string(nodes)
			+ " nps " + std::to_string(static_cast<int>((1000.0 * nodes) / (1000.0 * (std::clock() - start_time) / CLOCKS_PER_SEC)))
			+ " pv ";

		for (auto& move : pv) {
			out += move.toUci() + " ";
		}
		return out;
	}
	return "";
	//}
}

void Engine::storeTTEntry(u64 hash_key, int score, TType type, u8 depth_left, Move best) {
	hash_key = hash_key & (1048576 - 1);

	if (tt[hash_key].ply <= start_ply || tt[hash_key].depth_left <= depth_left) { //replace
		tt[hash_key] = TTEntry{ score, u8(depth_left), u16(start_ply), u8(max_depth), type, best };

	}
}


bool Engine::checkTime() {
	if ((std::clock() - start_time) > max_time) return true;
	return false;
}

void Engine::calcTime() {
	if (tc.movetime) {
		max_time = tc.movetime;
		return;
	}
	StaticVector<Move> legal_moves;
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

void Engine::updatePV(int depth, Move move) {
	pv_table[depth][0] = move;

	for (int i = 0; i < pv_length[depth + 1]; i++) {
		pv_table[depth][i + 1] = pv_table[depth + 1][i];
	}
	pv_length[depth] = pv_length[depth + 1] + 1;
}

int Engine::quiesce(int alpha, int beta) {
	nodes++;
	
	if (checkTime()) return -100000;
	if (b.is3fold()) return 0;
	int search_ply = b.ply - start_ply;
	/*
	TTEntry* entry = probeTT(b.getHash());

	if (entry) {
		// Use the entry based on its bound type
		if (entry->type == TType::EXACT) return entry->eval;
		if (entry->type == TType::BETA && entry->eval >= beta) return entry->eval;  // Beta cutoff
		if (entry->type == TType::ALPHA && entry->eval <= alpha) return entry->eval;  // Alpha cutoff
	}*/
	
	int stand_pat = b.getEval();
	int best = stand_pat;

	//delta prune
	if (stand_pat < alpha - 950) return alpha;

	if (stand_pat >= beta) return beta;

	if (alpha < stand_pat) {
		alpha = stand_pat;
	}

	g_move_list[search_ply].clear();
	b.genPseudoLegalCaptures(g_move_list[search_ply]);
	b.filterToLegal(g_move_list[search_ply]);
	MoveGen move_gen;

	// Check for #M
	if (!g_move_list[search_ply].size() && b.isCheck()) {
		b.genPseudoLegalMoves(g_move_list[search_ply]);
		b.filterToLegal(g_move_list[search_ply]);
		if (!g_move_list[search_ply].size()) {
			return -99999 + b.ply - start_ply;
		}
	}
	Move best_move;

	Move move = move_gen.getNext(*this, b, g_move_list[search_ply]);
	search_calls++;
	while (move.raw()) {

		if (move.captured() == eKing) return 99999 - (b.ply - start_ply);


		b.doMove(move);
		int score = -quiesce(-beta, -alpha);
		b.undoMove();

		if (score >= beta) return score;

		if (score > best) {
			best = score;
		}
		if (score > alpha) {
			alpha = score;
		}
		move = move_gen.getNext(*this, b, g_move_list[search_ply]);
	}
	if (best <= alpha) return alpha; 
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
