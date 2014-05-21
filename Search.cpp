/*
 * Gunborg - UCI chess engine
 * Copyright (C) 2013-2014 Torbjörn Nilsson
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
 * Search.cpp
 *
 *  Created on: Jan 11, 2014
 *      Author: Torbjörn Nilsson
 */

#include "Search.h"

#include "board.h"
#include "Cache.h"
#include "eval.h"
#include "moves.h"
#include "util.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <limits.h>
#include <deque>
#include <sstream>

namespace gunborg {

Search::Search() {
	max_think_time_ms = 10000;
	node_count = 0;
	quiescence_node_count = 0;
	total_generated_moves = 0;
}

inline bool Search::time_to_stop() {
	int time_elapsed = std::chrono::duration_cast < std::chrono::milliseconds > (clock.now() - start).count();
	return time_elapsed > max_think_time_ms || !should_run;
}

bool is_equal(const Board& b1, const Board& b2) {
	if (b1.b[BLACK][PAWN] != b2.b[BLACK][PAWN]) {
		return false;
	}
	if (b1.b[BLACK][BISHOP] != b2.b[BLACK][BISHOP]) {
		return false;
	}
	if (b1.b[BLACK][KNIGHT] != b2.b[BLACK][KNIGHT]) {
		return false;
	}
	if (b1.b[BLACK][ROOK] != b2.b[BLACK][ROOK]) {
		return false;
	}
	if (b1.b[BLACK][QUEEN] != b2.b[BLACK][QUEEN]) {
		return false;
	}
	if (b1.b[BLACK][KING] != b2.b[BLACK][KING]) {
		return false;
	}
	if (b1.b[WHITE][PAWN] != b2.b[WHITE][PAWN]) {
		return false;
	}
	if (b1.b[WHITE][BISHOP] != b2.b[WHITE][BISHOP]) {
		return false;
	}
	if (b1.b[WHITE][KNIGHT] != b2.b[WHITE][KNIGHT]) {
		return false;
	}
	if (b1.b[WHITE][ROOK] != b2.b[WHITE][ROOK]) {
		return false;
	}
	if (b1.b[WHITE][QUEEN] != b2.b[WHITE][QUEEN]) {
		return false;
	}
	if (b1.b[WHITE][KING] != b2.b[WHITE][KING]) {
		return false;
	}
	return true;
}

/**
 * selection sort algorithm
 *
 * The algorithm divides the input list into two parts: the sublist of items already sorted,
 * which is built up from left to right at the front (left) of the list,
 * and the sublist of items remaining to be sorted that occupy the rest of the list.
 *
 * swaps the best, non-sorted, move to next index
 */
void pick_next_move(MoveList& moves, const int no_sorted_moves) {
	int max_sort_score = INT_MIN;
	int max_index = no_sorted_moves;
	int i = no_sorted_moves;
	for (auto it = moves.begin() + no_sorted_moves; it != moves.end(); ++it) {
		if (it->sort_score >= max_sort_score) {
			max_index = i;
			max_sort_score = it->sort_score;
		}
		i++;
	}
	std::swap(moves[no_sorted_moves], moves[max_index]);
}

int Search::capture_quiescence_eval_search(bool white_turn, int alpha, int beta, Board& board) {
	if (board.b[WHITE][KING] == 0) {
		return -10000;
	} else if (board.b[BLACK][KING] == 0) {
		return 10000;
	}

	int static_eval = evaluate(board);
	if (static_eval > alpha && white_turn) {
		return static_eval;
	}
	if (static_eval < beta && !white_turn) {
		return static_eval;
	}

	MoveList capture_moves = get_captures(board, white_turn);
	if (capture_moves.empty()) {
		// the end point of the quiescence search
		return static_eval;
	}
	for (unsigned int i = 0; i < capture_moves.size(); ++i) {
		pick_next_move(capture_moves, i);
		Move move = capture_moves[i];
		quiescence_node_count++;
		make_move(board, move);
		int res = capture_quiescence_eval_search(!white_turn, alpha, beta, board);
		unmake_move(board, move);
		if (res > alpha && white_turn) {
			alpha = res;
		}
		if (res < beta && !white_turn) {
			beta = res;
		}
		if (beta <= alpha || time_to_stop()) {
			break;
		}
	}
	if (white_turn) {
		return alpha;
	}
	return beta;
}

int Search::alphaBeta(bool white_turn, int depth, int alpha, int beta, Board& board, Transposition *tt,
		bool null_move_in_branch, Move (&killers)[32][2], int (&history)[64][64], int ply) {

	// If, mate we do not need search at greater depths
	if (board.b[WHITE][KING] == 0) {
		return -10000;
	} else if (board.b[BLACK][KING] == 0) {
		return 10000;
	}
	if (depth == 0) {
		return capture_quiescence_eval_search(white_turn, alpha, beta, board);
	}
	if (depth == 1) {
		// futility pruning. we do not hope for improving a position more than 300 in one move...
		int static_eval = evaluate(board);
		if (white_turn && (static_eval + 300) < alpha) {
			return capture_quiescence_eval_search(white_turn, alpha, beta, board);
		}
		if (!white_turn && (static_eval - 300) > beta) {
			return capture_quiescence_eval_search(white_turn, alpha, beta, board);
		}
	}
	if (depth == 2) {
		// extended futility pruning. we do not hope for improving a position more than 500 in two plies...
		// not really proven to +ELO but does not worse performance at least
		int static_eval = evaluate(board);
		if ((white_turn && (static_eval + 500) < alpha) || (!white_turn && (static_eval - 500) > beta)) {
			return capture_quiescence_eval_search(white_turn, alpha, beta, board);
		}
	}

	// null move heuristic - we do this despite in check..
	if (!null_move_in_branch && depth > 3) {
		// skip a turn and see if and see if we get a cut-off at shallower depth
		// it assumes:
		// 1. That the disadvantage of forfeiting one's turn is greater than the disadvantage of performing a shallower search.
		// 2. That the beta cut-offs prunes enough branches to be worth the time searching at reduced depth
		int R = 2; // depth reduction
		int res = alphaBeta(!white_turn, depth - 1 - R, alpha, beta, board, tt, true, killers, history, ply + 1);
		if (white_turn && res > alpha) {
			alpha = res;
		} else if (!white_turn && res < beta) {
			beta = res;
		}
		if (beta <= alpha) {
			if (white_turn) {
				return alpha;
			}
			return beta;
		}
	}
	MoveList moves = get_children(board, white_turn);
	if (moves.empty()) {
		return 0;
	}
	Transposition tt_pv = tt[board.hash_key % hash_size];
	for (auto it = moves.begin(); it != moves.end(); ++it) {
		// sort pv moves first
		if (tt_pv.next_move != 0 && tt_pv.hash == board.hash_key && tt_pv.next_move == it->m) {
			it->sort_score += 1100000;
		}
		// ...then captures in MVVLVA order
		if (!is_capture(it->m)) {
			// killer moves are quite moves that has previously led to a cut-off
			if (killers[ply - 1][0].m == it->m) {
				it->sort_score += 999999;
			} else if (killers[ply - 1][1].m == it->m) {
				it->sort_score += 899999;
			} else {
				// "history heuristics"
				// the rest of the quite moves are sorted based on how often they increase score in the search tree
				it->sort_score += history[from_square(it->m)][to_square(it->m)];
			}
		}
	}
	total_generated_moves += moves.size();

	Transposition t;
	t.hash = board.hash_key;
	int next_move = 0;
	for (unsigned int i = 0; i < moves.size(); ++i) {
		// late move reduction.
		// we assume sort order is good enough to not search later moves as deep as the first 4
		if (depth > 5 && i == 10) {
			depth -= 2;
		}

		pick_next_move(moves, i);
		Move child = moves[i];
		node_count++;
		make_move(board, child);

		int res = alphaBeta(!white_turn, depth - 1, alpha, beta, board, tt, null_move_in_branch, killers, history,
				ply + 1);
		unmake_move(board, child);
		if (res > alpha && res < beta) {
			// only cache exact scores

		}

		if (res > alpha && white_turn) {
			next_move = child.m;
			alpha = res;
			//history heuristics
			if (!is_capture(child.m)) {
				history[from_square(child.m)][to_square(child.m)] += depth;
			}
			// update pv
		}
		if (res < beta && !white_turn) {
			next_move = child.m;
			beta = res;
			//history heuristics
			if (!is_capture(child.m)) {
				history[from_square(child.m)][to_square(child.m)] += depth;
			}
			// update pv
		}
		if (beta <= alpha || time_to_stop()) {
			if (!is_capture(child.m)) {
				if (killers[ply - 1][0].m != child.m) {
					killers[ply - 1][1] = killers[ply - 1][0];
					killers[ply - 1][0] = child;
				}
			}
			break;
		}
	}
	t.next_move = next_move;
	tt[board.hash_key % hash_size] = t;
	if (white_turn) {
		return alpha;
	}
	return beta;
}

void Search::search_best_move(const Board& board, const bool white_turn, list history) {
	start = clock.now();

	Board b2 = board;
	MoveList root_moves = get_children(b2, white_turn);

	for (auto it = root_moves.begin(); it != root_moves.end(); ++it) {
		make_move(b2, *it);
		it->sort_score = evaluate(b2);
		unmake_move(b2, *it);
	}

	int alpha = INT_MIN;
	int beta = INT_MAX;
	int START_WINDOW_SIZE = 32;
	Move killers2[32][2];
	int quites_history[64][64] = { };
	Transposition * tt = new Transposition[hash_size];
	b2.hash_key = 0;
	for (int depth = 1; depth < 30;) {
		if (!white_turn) {
			for (auto it = root_moves.begin(); it != root_moves.end(); ++it) {
				it->sort_score = -it->sort_score;
			}
		}

		int score = white_turn ? alpha : beta;
		int a = alpha;
		int b = beta;
		MoveList next_iteration_root_moves;
		int pv[depth];
		for (unsigned int i = 0; i < root_moves.size(); i++) {
			// TODO only search best move with alpha, alpha + 1, if fail high/low there is another better move and we have to search all moves
			pick_next_move(root_moves, i);
			Move root_move = root_moves[i];
			if (b > a) { //strict?
				if (is_castling(root_move.m)) {
					Transposition * empty_tt = new Transposition[1000];
					int empty_quites_history[64][64] = { };
					int one_depth_score = alphaBeta(!white_turn, 1, INT_MIN / 2, INT_MAX / 2, b2, empty_tt,
					true, killers2, empty_quites_history, 1);
					delete empty_tt;
					if ((white_turn && one_depth_score < -5000) || (!white_turn && one_depth_score > 5000)) {
						// we are in check and castling is illegal. The move is not copied to the next iteration.
						root_move.sort_score = one_depth_score;
						continue;
					}
				}
				node_count++;
				make_move(b2, root_move);
				int res = -1; // TODO clean up code
				for (auto hit = history.begin(); hit != history.end(); ++hit) {
					if (is_equal(b2, *hit)) {
						// draw by repetition
						res = 0;
					}
				}
				if (res == -1) {
					res = alphaBeta(!white_turn, depth - 1, a, b, b2, tt, false, killers2, quites_history, 1);
				}
				unmake_move(b2, root_move);
				if (res > a && white_turn) {
					score = res;
					a = res;
					// TODO extract method get_pv
					for (int p = 1; p < depth; p++) {
						pv[p] = 0;
					}
					pv[0] = root_move.m;
					int next_pv_move = root_move.m;
					uint32_t hash = b2.hash_key;
					for (int p = 1; p < depth - 1; p++) {
						hash ^= next_pv_move;
						Transposition next = tt[hash % hash_size];
						if (next.hash == hash && next.next_move != 0) {
							pv[p] = next.next_move;
							next_pv_move = next.next_move;
						} else {
							break;
						}
					}
				}
				if (res < b && !white_turn) {
					score = res;
					b = res;
					for (int p = 1; p < depth; p++) {
						pv[p] = 0;
					}
					pv[0] = root_move.m;
					int next_pv_move = root_move.m;
					uint32_t hash = b2.hash_key;
					for (int p = 1; p < depth - 1; p++) {
						hash ^= next_pv_move;
						Transposition next = tt[hash % hash_size];
						if (next.hash == hash && next.next_move != 0) {
							pv[p] = next.next_move;
							next_pv_move = next.next_move;
						} else {
							break;
						}
					}
				}
				root_move.sort_score = res;
			} else {
				// beta fail
				// we just know that the rest of the moves are worse than the best move, or is this un-reachable code?
				root_move.sort_score = white_turn ? a - i : b + i; // keep sort order
			}
			next_iteration_root_moves.push_back(root_move);
		}
		int time_elapsed_last_depth_ms = std::chrono::duration_cast < std::chrono::milliseconds
				> (clock.now() - start).count();
		if (score > alpha && score < beta) {
			std::string pvstring = pvstring_from_stack(pv, depth);
			// uci score is from engine perspective
			int engine_score = white_turn ? score : -score;
			std::cout << "info score cp " << engine_score << " depth " << depth << " time "
					<< time_elapsed_last_depth_ms << " nodes " << node_count << " pv " << pvstring << "\n"
					<< std::flush;

			std::stringstream ss(pvstring);
			getline(ss, best_move, ' ');
		}
		if (time_to_stop()) {
			break;
		}
		// aspiration window based on previous score - use delta to increase if no moves are found...
		int window_size = beta - alpha;
		if (score <= alpha) {
			// failed low, search again at same depth
			alpha = alpha - window_size;
			continue;
		}
		if (score >= beta) {
			// failed high, search again at same depth
			beta = beta + window_size;
			continue;
		}

		root_moves = next_iteration_root_moves;

		// aspiration window size
		alpha = score - START_WINDOW_SIZE / 2;
		beta = score + START_WINDOW_SIZE / 2;

		// if mate is found at this depth, just stop searching for better moves. Cause there are none.. The best move at the last depth will prolong the inevitably as long as possible.
		if ((white_turn && score < -5000) || (!white_turn && score > 5000)) {
			// we are mated...
			break;
		}
		if (abs(score) > 5000) {
			// deliver mate
			break;
		}

		// save some time for next move
		if ((4 * time_elapsed_last_depth_ms) > max_think_time_ms) {
			break;
		}
		depth++;
	}
	std::cout << "bestmove " << best_move << std::endl << std::flush;
	delete tt;
	return;
}

Search::~Search() {
}

} /* namespace gunborg */
