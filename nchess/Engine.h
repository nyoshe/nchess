#pragma once

#include "Board.h"
#include <ctime>
#include <algorithm>
#include <unordered_map>

#include "Memory.h"
#include "robin_hood.h"
enum class TType : u8 {
	INVALID,
	EXACT,
	ALPHA,
	BETA,
	BEST
};

struct TTEntry {
	int eval = 0;
	u8 depth_left = 0;
	u16 ply = 0;
	u8 search_depth = 0;
	TType type = TType::INVALID;
	Move best_move;
	TTEntry& operator=(const TTEntry& other) {
		if (this != &other) {
			eval = other.eval;
			depth_left = other.depth_left;
			ply = other.ply;
			search_depth = other.search_depth;
			type = other.type;
			best_move = other.best_move;
		}
		return *this;
	}
	[[nodiscard]] explicit constexpr operator bool() const {
		return type != TType::INVALID;
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
	int alphaBeta(int alpha, int beta, int depth_left, bool is_pv);
	int quiesce(int alpha, int beta);
public:
	std::array<StaticVector<int>, 64> eval_vec;
	int start_ply = 0;
	Board b;
	TimeControl tc;
	Engine() {
		tt.resize(1048576);
		//for (auto& entry : tt) {
		//	entry = TTEntry();
		//}
	}
	std::array<StaticVector<Move>, 64> move_vec;


	std::vector<PerfT> doPerftSearch(int depth);
	std::vector<PerfT> doPerftSearch(std::string position, int depth);

	void setBoardFEN(std::istringstream& fen);
	void setBoardUCI(std::istringstream& uci);

	Move search(int depth);
	std::vector<Move> getPrincipalVariation() const;

	std::string getPV();
	void printPV(int score);

	void storeTTEntry(u64 hash_key, int score, TType type, u8 depth_left, Move best);

	TTEntry probeTT(u64 hash_key) const {
		hash_key = hash_key & (1048576 - 1);
		return tt[hash_key];
	}

	bool checkTime();
	void calcTime();

	void updatePV(int depth, Move move);


};

enum class MoveStage {
	ttMove,
	captures,
	evals
};
class MoveGen {
	std::vector<std::pair<int, Move>> fallback_moves;
	MoveStage stage = MoveStage::ttMove;
	bool init = false;
	
public:

	Move getNext(Engine& e, Board& b, StaticVector<Move>& moves) {
		Move out;
		TTEntry entry = e.probeTT(e.b.getHash());
		if (stage == MoveStage::ttMove) {
			auto pos_best = std::find(moves.begin(), moves.end(), entry.best_move);
			if (pos_best != moves.end()) {
				out = *pos_best;
				*pos_best = moves.back();
				moves.pop_back();

				stage = MoveStage::captures;
				return out;
			}
		}


		if (stage == MoveStage::captures && moves.end() != std::find_if(moves.begin(), moves.end(), [](const auto& m) {return m.captured();})) {
			auto mvv_lva = [](const auto& a, const auto& b) {
				return piece_vals[a.captured()] * 10 - piece_vals[a.piece()] < piece_vals[b.captured()] * 10 - piece_vals[b.piece()];
				};
			auto pos_best = std::ranges::max_element(moves.begin(), moves.end(), mvv_lva);
			out = *pos_best;
			*pos_best = moves.back();
			moves.pop_back();
			return out;
		}
		else {
			stage = MoveStage::evals;
		}
		
		
		if (stage == MoveStage::evals && !init) {
			init = true;
			int eval_ply = b.ply - e.start_ply;
			e.eval_vec[eval_ply].clear();
			for (auto& move : moves) {
				b.doMove(move);
				e.eval_vec[eval_ply].emplace_back(-b.getEval());
				b.undoMove();
			}
		}
		
		if (stage == MoveStage::evals && !moves.empty()) {
			int eval_ply = b.ply - e.start_ply;
			auto pos_best = std::ranges::max_element(e.eval_vec[eval_ply].begin(), e.eval_vec[eval_ply].end());

			int pos = std::distance(e.eval_vec[eval_ply].begin(), pos_best);
			out = moves[pos];
			moves[pos] = moves.back();
			moves.pop_back();
			*pos_best = e.eval_vec[eval_ply].back();
			e.eval_vec[eval_ply].pop_back();

			return out;
		}
		return Move();
	}
};