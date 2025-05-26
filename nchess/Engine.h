#pragma once
class Engine
{
private:
	// Engine state variables
	int depth = 0;
	int nodes = 0;
	int time_limit = 0; // in milliseconds
	bool is_running = false;
};

