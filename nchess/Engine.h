#pragma once

#include "Board.h"
#include <ctime>
#include <algorithm>
#include <unordered_map>
#include "robin_hood.h"
enum class TType : u8 {
	EXACT,
	ALPHA,
	BETA,
	BEST
};

struct TTEntry {
	int eval = 0;
	u8 depth = 0;
	u16 ply = 0;
	u8 search_depth = 0;
	TType type = TType::EXACT;
	Move best_move;
};

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
	int hash_hits;
	int hash_miss;
	//Move best_move;
	static constexpr int MAX_PLY = 64;
	std::array<std::array<Move, MAX_PLY>, MAX_PLY> pv_table;
	std::array<std::array<std::array<int, 64>, 64>, 2> history_table;
	std::array<int, MAX_PLY> pv_length;
	std::array<std::array<Move, 10>, MAX_PLY> killer_moves;
	// Engine state variables
	int start_ply = 0;
	u16 max_depth = 0;
	int nodes = 0;
	int current_age = 0;
	bool is_pv = false;

    // Timer variables
    std::clock_t start_time = 0;
	int max_time = 0;
	std::vector<PerfT> perf_values;
	int pos_count = 0;
	robin_hood::unordered_map<u64, TTEntry> tt;

	void perftSearch(int depth);
	int alphaBeta(int alpha, int beta, int depthleft);
	int quiesce(int alpha, int beta);
public:
	Board b;
	TimeControl tc;

	std::vector<PerfT> doPerftSearch(int depth);
	std::vector<PerfT> doPerftSearch(std::string position, int depth);

	void setBoardFEN(std::istringstream& fen);
	void setBoardUCI(std::istringstream& uci);

	Move search(int depth);
	std::vector<std::pair<int, Move>> sortMoves(std::vector<Move>& moves);
	std::vector<Move> getPrincipalVariation() const;

	void printPV(Move root_move, int score) const;

	void storeTTEntry(u64 hash_key, int score, TType type, u8 depth_left, Move best);

	TTEntry* probeTT(u64 hash_key) {
		auto it = tt.find(hash_key);
		if (it != tt.end()) {
			hash_hits++;
			return &(it->second);
		}
		hash_miss++;
		return nullptr;
	}

	bool checkTime();
	void calcTime();

	void updatePV(int ply, Move move) {
		// Store the current move as the first move in the PV for this ply
		pv_table[ply][0] = move;

		// Copy the child PV into the current PV
		for (int next_ply = 0; next_ply < pv_length[ply + 1]; next_ply++) {
			pv_table[ply][next_ply + 1] = pv_table[ply + 1][next_ply];
		}

		// Update the PV length for this ply
		pv_length[ply] = pv_length[ply + 1] + 1;
	}
	void cleanupTT() {
		// Only call this occasionally, like after many searches
		// This is expensive but can prevent table from filling with ancient positions

		std::vector<u64> to_remove;

		for (const auto& [key, entry] : tt) {
			// Remove entries more than N searches old
			if (start_ply + 1  > entry.ply) {
				to_remove.push_back(key);
			}
		}

		for (auto key : to_remove) {
			tt.erase(key);
		}
	}

};

