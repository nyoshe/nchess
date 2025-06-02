#pragma once

#include "Board.h"
#include <ctime>
#include <algorithm>
#include <unordered_map>
struct TTEntry {
	int16_t eval = 0;
	uint16_t depth = 0;
	uint16_t age = 0;
	uint16_t search_depth = 0;
};
typedef struct LINE {
	int cmove;              // Number of moves in the line.
	Move argmove[256];  // The line.
}   LINE;

struct TimeControl {
	int wtime = 0;
	int btime = 0;
	int winc = 0;
	int binc = 0;
	int movetime = 0;
};

class Engine
{
private:
	Move best_move;
	static constexpr int MAX_PLY = 64;
	std::array<std::array<Move, MAX_PLY>, MAX_PLY> pv_table;
	std::array<int, MAX_PLY> pv_length;
	std::vector<Move> best_pv;
	// Engine state variables
	int start_ply = 0;
	uint16_t max_depth = 0;
	int nodes = 0;
	int current_age = 0;
    // Timer variables
    std::clock_t start_time = 0;
	int max_time = 0;
	bool is_running = false;

	
	std::vector<PerfT> perf_values;
	int pos_count = 0;
	void perftSearch(int depth);
	int alphaBeta(int alpha, int beta, int depthleft);
	int quiesce(int alpha, int beta);
	std::unordered_map<u64, TTEntry> tt;

public:
	Board b;
	TimeControl tc;
	std::vector<PerfT>  doPerftSearch(int depth);
	void setBoardFEN(std::istringstream& fen);
	void setBoardUCI(std::istringstream& uci);
	std::vector<PerfT>  doPerftSearch(std::string position, int depth);
	Move search(int depth);
	std::vector<std::pair<int, Move>> sortMovesByEval(std::vector<Move>& moves);
	std::vector<Move> getPrincipalVariation() const;
	void printPV(Move root_move, int score) const;
	void pruneTT(size_t max_size);

	void storeTTEntry(u64 hash_key, int16_t eval, uint8_t depth);

	void updateTTAge();
	bool checkTime() {
		if ((1000.0 * (std::clock() - start_time) / CLOCKS_PER_SEC) > max_time) return true;
		return false;
	}
	void calcTime() {
		if (tc.movetime) {
			max_time = tc.movetime;
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
};

