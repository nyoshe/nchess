#include "Engine.h"
void Engine::perftSearch(int d) {
	if (!d) return;
	std::vector<Move> legal_moves;
	b.genPseudoLegalMoves(legal_moves);
	b.genLegalMoves(legal_moves);
	for (auto& move : legal_moves) {
		int index = max_depth - d;
		if (do_perft) {
			if (max_depth != d){
				pos_count++;
				//std::cout<< " - "<< move.toUci() << "\n";
				//pos_count = 0;
			}
			perf_values[index].depth = index + 1;
			perf_values[index].nodes++;
			perf_values[index].captures += static_cast<bool>(move.captured());
			perf_values[index].en_passant += move.isEnPassant();
			perf_values[index].castles += move.isCastle();
		}

		b.doMove(move);

		if (do_perft) perf_values[index].checks += b.isCheck();

		perftSearch(d - 1);

		b.undoMove();

		if (max_depth == d) {
			std::cout << move.toUci() << ": " << pos_count << ", ";
			pos_count = 0;
		}
	}
}

void Engine::search(int depth) {
	alphaBeta( -100000,  100000, 5);
}

int Engine::alphaBeta(int alpha, int beta, int depthleft) {
	if (depthleft == 0) return quiesce(alpha, beta);

	std::vector<Move> legal_moves;
	b.genPseudoLegalMoves(legal_moves);
	b.genLegalMoves(legal_moves);

	int bestValue = -1000000;
	for (auto& move : legal_moves) {
		int score = -alphaBeta(-beta, -alpha, depthleft - 1);
		if (score > bestValue)
		{
			bestValue = score;
			if (score > alpha)
				alpha = score; // alpha acts like max in MiniMax
		}
		if (score >= beta)
			return bestValue;   //  fail soft beta-cutoff, existing the loop here is also fine
	}
	return bestValue;
}

int Engine::quiesce(int alpha, int beta) {
	/*
	int static_eval = b.getEval();

	// Stand Pat
	int best_value = static_eval;
	if (best_value >= beta)
		return best_value;
	if (best_value > alpha)
		alpha = best_value;

	until(every_capture_has_been_examined) {
		MakeCapture();
		score = -Quiesce(-beta, -alpha);
		TakeBackMove();

		if (score >= beta)
			return score;
		if (score > best_value)
			best_value = score;
		if (score > alpha)
			alpha = score;
	}

	return best_value;
	*/

	return 12;
}

void Engine::startSearch(int d) {
	perf_values.clear();
	perf_values.resize(d);
	max_depth = d;
	perftSearch(d);
}

std::vector<PerfT> Engine::doPerftSearch(int depth) {

	start_time = std::clock();

	do_perft = true;
	startSearch(depth);
	do_perft = false;

	end_time = std::clock();


	// Returns elapsed time in milliseconds
	std::cout << "search time: " << static_cast<int>(1000.0 * (end_time - start_time) / CLOCKS_PER_SEC) << "ms\n\n";

	return perf_values;
}

std::vector<PerfT> Engine::doPerftSearch(std::string position, int depth) {
	b.loadFen(position);
	return doPerftSearch(depth);
}
