#pragma once

#include "Board.h"
#include <ctime>

class Engine
{
private:
	// Engine state variables
	int max_depth = 0;
	int nodes = 0;
	int time_limit = 0; // in milliseconds

    // Timer variables
    std::clock_t start_time = 0;
    std::clock_t end_time = 0;

    
	bool is_running = false;
	bool do_perft = false;
	Board b;
	std::vector<PerfT> perf_values;
	int pos_count = 0;
	void perftSearch(int depth);
	void search(int depth);
	int alphaBeta(int alpha, int beta, int depthleft);
	int quiesce(int alpha, int beta);

public:
	void startSearch(int depth);
	std::vector<PerfT>  doPerftSearch(int depth);
	std::vector<PerfT>  doPerftSearch(std::string position, int depth);
};

