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
	TTEntry& operator=(const TTEntry& other) {
		if (this != &other) {
			eval = other.eval;
			depth = other.depth;
			ply = other.ply;
			search_depth = other.search_depth;
			type = other.type;
			best_move = other.best_move;
		}
		return *this;
	}
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
	std::vector<Move> pv_moves;
	std::vector<TTEntry> tt;

	
	// Engine state variables
	int start_ply = 0;
	u16 max_depth = 0;
	int nodes = 0;
	int current_age = 0;

	float search_calls = 0;
	float moves_inspected = 0;

    // Timer variables
    std::clock_t start_time = 0;
	int max_time = 0;
	std::vector<PerfT> perf_values;
	int pos_count = 0;


	void perftSearch(int depth);
	int alphaBeta(int alpha, int beta, int depthleft, bool is_pv);
	int quiesce(int alpha, int beta);
public:
	Board b;
	TimeControl tc;
	Engine() {
		tt.resize(1048576);
		//for (auto& entry : tt) {
		//	entry = TTEntry();
		//}
	}

	std::vector<PerfT> doPerftSearch(int depth);
	std::vector<PerfT> doPerftSearch(std::string position, int depth);

	void setBoardFEN(std::istringstream& fen);
	void setBoardUCI(std::istringstream& uci);

	Move search(int depth);
	void sortMoves(std::vector<Move>& moves);
	std::vector<Move> getPrincipalVariation() const;

	void printPV(int score) ;

	void storeTTEntry(u64 hash_key, int score, TType type, u8 depth_left, Move best);

	TTEntry probeTT(u64 hash_key) {
		hash_key = hash_key & (1048576 - 1);
		return tt[hash_key];
	}

	bool checkTime();
	void calcTime();

	void updatePV(int depth, Move move);


};

