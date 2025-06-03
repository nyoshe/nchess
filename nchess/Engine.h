#pragma once

#include "Board.h"
#include <ctime>
#include <algorithm>
#include <unordered_map>
#include "robin_hood.h"
enum class TType {
	EXACT,
	ALPHA,
	BETA
};

struct TTEntry {
	int eval = 0;
	u8 depth = 0;
	u8 age = 0;
	u8 search_depth = 0;
	TType type = TType::EXACT;
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
	Move best_move;
	static constexpr int MAX_PLY = 64;
	std::array<std::array<Move, MAX_PLY>, MAX_PLY> pv_table;
	std::array<int, MAX_PLY> pv_length;
	std::vector<Move> best_pv;
	std::vector<BoardState> best_pv_state;
	// Engine state variables
	int start_ply = 0;
	u16 max_depth = 0;
	int nodes = 0;
	int current_age = 0;

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
	std::vector<std::pair<int, Move>> sortMovesByEval(std::vector<Move>& moves);
	std::vector<Move> getPrincipalVariation() const;

	void printPV(Move root_move, int score) const;

	void pruneTT(size_t max_size);
	void storeTTEntry(u64 hash_key, int score, TType type, u8 depth);
	void updateTTAge();
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
};

