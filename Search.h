/*
 * Gunborg - UCI chess engine
 * Copyright (C) 2013-2015 Torbjörn Nilsson
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Search.h
 *
 *  Created on: Jan 11, 2014
 *      Author: Torbjörn Nilsson
 */

#ifndef SEARCH_H_
#define SEARCH_H_

#include "board.h"
#include "Cache.h"
#include <atomic>
#include <chrono>
#include <string>

namespace gunborg {

class Search {

private:
	std::chrono::high_resolution_clock clock;
	std::chrono::high_resolution_clock::time_point start;
	bool pondering = false;
	static const int WINDOW_SIZE = 56;
	static const int START_WINDOW_SIZE = 30;
	static const int DELTA_PRUNING_MARGIN = 200;

	int alpha_beta(bool white_turn, int depth, int alpha, int beta, Position& position, Transposition *tt,
			bool null_move_disabled, Move (&killers)[32][2], uint64_t (&history)[64][64], int ply, int extension);
	int null_window_search(bool white_turn, int depth, int beta, Position& position, Transposition *tt,
			bool null_move_disabled, Move (&killers)[32][2], uint64_t (&history)[64][64], int ply, int extension);
	int capture_quiescence_eval_search(bool white_turn, int alpha, int beta, Position& position);
	int aspiration_window_search(bool white_turn, int depth, int alpha, int beta, Position& pos, Transposition *tt,
			bool in_check, Move (&killers)[32][2], uint64_t (&history)[64][64]);

	bool time_to_stop();
	void print_uci_info(int pv[], int depth, int score, Transposition *tt);
	void init_sort_score(const bool white_turn, MoveList& root_moves, Position& p, Transposition *tt);
	bool is_draw_by_repetition(
			const list& history, const Position& pos, const bool white_turn);
	bool is_stale_mate(const bool white_turn, Position& pos);

public:
	Search();
	std::atomic_bool should_run;
	int max_think_time_ms;
	int max_depth = 30;
	int node_count;
	bool save_time;
	uint8_t generation = 0;

	void search_best_move(const Position& position, const bool white_turn, list history, Transposition * tt);

	void ponder();
	void ponder_hit();

	virtual ~Search();
};

} /* namespace gunborg */
#endif /* SEARCH_H_ */
