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

const int MAX_CHECK_EXTENSION = 2;

Search::Search() {
	max_think_time_ms = 10000;
	node_count = 0;
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
		return white_turn ? -10000 : 10000;
	} else if (board.b[BLACK][KING] == 0) {
		return white_turn ? 10000 : -10000;
	}

	int static_eval = nega_evaluate(board, white_turn);
	if (static_eval > alpha) {
		return static_eval;
	}

	MoveList capture_moves = get_captures(board, white_turn);
	if (capture_moves.empty()) {
		// the end point of the quiescence search
		return static_eval;
	}
	bool has_legal_capture = false;
	for (unsigned int i = 0; i < capture_moves.size(); ++i) {
		pick_next_move(capture_moves, i);
		Move move = capture_moves[i];
		bool legal_move = make_move(board, move);
		if (!legal_move) {
			unmake_move(board, move);
			continue;
		}
		has_legal_capture = true;
		int res = -capture_quiescence_eval_search(!white_turn, -beta, -alpha, board);
		unmake_move(board, move);
		if (res >= beta || time_to_stop()) {
			return beta;
		}
		if (res > alpha) {
			alpha = res;
		}
	}
	if (!has_legal_capture) {
		return static_eval;
	}
	return alpha;
}

int Search::null_window_search(bool white_turn, int depth, int beta, Board& board, Transposition *tt,
		bool null_move_not_allowed, Move (&killers)[32][2], int (&history)[64][64], int ply, int extension) {
	int alpha = beta - 1;
	return alpha_beta(white_turn, depth, alpha, beta, board, tt, null_move_not_allowed, killers, history, ply, extension);
}

inline bool should_prune(int depth, bool white_turn, Board& board, int alpha, int beta) {
	if (depth > 3) {
		return false;
	}
	if (depth == 1) {
		int static_eval = nega_evaluate(board, white_turn);
		// futility pruning. we do not hope to improve a position more than 300 in one move
		if (static_eval + 300 < alpha) {
			return true;
		}
	}
	if (depth == 2) {
		// extended futility pruning. we do not hope to improve a position more than 520 in two plies
		int static_eval = nega_evaluate(board, white_turn);
		if (static_eval + 520 < alpha) {
			return true;
		}
	}
	if (depth == 3) {
		// extended futility pruning. we do not hope to improve a position more than 900 in three plies
		int static_eval = nega_evaluate(board, white_turn);
		if (static_eval + 900 < alpha) {
			return true;
		}
	}
	return false;
}

int Search::alpha_beta(bool white_turn, int depth, int alpha, int beta, Board& board, Transposition *tt,
		bool null_move_not_allowed, Move (&killers)[32][2], int (&history)[64][64], int ply, int extension) {
	if (depth == 0) {
		return capture_quiescence_eval_search(white_turn, alpha, beta, board);
	}
	if (should_prune(depth, white_turn, board, alpha, beta)) {
		return capture_quiescence_eval_search(white_turn, alpha, beta, board);
	}
	// check for cached score in transposition table
	Transposition tt_pv = tt[hash_index(board.hash_key) % hash_size];
	bool cache_hit = tt_pv.next_move != 0 && tt_pv.hash == hash_verification(board.hash_key);
	if (cache_hit && tt_pv.depth == depth) {
		int cached_score = tt_pv.score;
		if (cached_score >= alpha && cached_score <= beta) {
			return cached_score;
		}
	}

	// null move heuristic
	if (!null_move_not_allowed && depth > 3) {
		// skip a turn and see if and see if we get a cut-off at shallower depth
		// it assumes:
		// 1. That the disadvantage of forfeiting one's turn is greater than the disadvantage of performing a shallower search.
		// 2. That the beta cut-offs prunes enough branches to be worth the time searching at reduced depth
		int R = 2; // depth reduction
		int res = -alpha_beta(!white_turn, depth - 1 - R, -beta, -alpha, board, tt, true, killers, history, ply + 1, extension);
		if (res >= beta) {
			return beta;
		}
	}

	MoveList moves = get_moves(board, white_turn);
	for (auto it = moves.begin(); it != moves.end(); ++it) {
		// sort pv moves first
		if (cache_hit && tt_pv.next_move == it->m) {
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

	Transposition t;
	t.hash = hash_verification(board.hash_key);
	int next_move = 0;
	bool has_legal_move = false;
	for (unsigned int i = 0; i < moves.size(); ++i) {

		pick_next_move(moves, i);
		Move move = moves[i];
		node_count++;
		bool legal_move = make_move(board, move);
		if (!legal_move) {
			unmake_move(board, move);
			continue;
		}
		has_legal_move = true;

		// late move reduction.
		// we assume sort order is good enough to not search later moves as deep as the first
		int depth_reduction = 0;
		if (depth > 2 && i > 5 && !is_capture(move.m)) {
			depth_reduction = 1;
		}
		if (extension < MAX_CHECK_EXTENSION) {
			// if this is a checking move, extend the search one ply
			if (get_attacked_squares(board, white_turn) & board.b[white_turn ? BLACK : WHITE][KING]) {
				extension++;
				depth_reduction = -1;
			}
		}
		int res;
		if (next_move != 0) {
			// we do not expect to find a better move
			// use a fast null window search to verify it!
			res = -null_window_search(!white_turn, depth - 1 - depth_reduction, -alpha, board, tt, null_move_not_allowed,
										killers, history, ply + 1, extension);
			if (res > alpha) {
				// score improved unexpected, we have to do a full window search
				res = -alpha_beta(!white_turn, depth - 1 - depth_reduction, -beta, -alpha, board, tt, null_move_not_allowed,
							killers, history, ply + 1, extension);
			}
		} else {
			res = -alpha_beta(!white_turn, depth - 1 - depth_reduction, -beta, -alpha, board, tt, null_move_not_allowed,
				killers, history, ply + 1, extension);
		}
		if (depth_reduction > 0 && res > alpha && res < beta) {
			// score improved "unexpected" at reduced depth
			// re-search at normal depth
			res = -alpha_beta(!white_turn, depth - 1, -beta, -alpha, board, tt, null_move_not_allowed, killers, history,
					ply + 1, extension);
		}


		unmake_move(board, move);

		if (res >= beta || time_to_stop()) {
			if (!is_capture(move.m)) {
				if (killers[ply - 1][0].m != move.m) {
					killers[ply - 1][1] = killers[ply - 1][0];
					killers[ply - 1][0] = move;
				}
			}
			next_move = move.m;
			t.next_move = next_move;
			t.score = beta;
			t.depth = 0;
			tt[hash_index(board.hash_key) % hash_size] = t;
			return beta;
		}

		if (res > alpha) {
			next_move = move.m;
			alpha = res;
			//history heuristics
			if (!is_capture(move.m)) {
				history[from_square(move.m)][to_square(move.m)] += depth;
			}
		}
	}
	if (!has_legal_move) {
		bool in_check = get_attacked_squares(board, !white_turn) & board.b[white_turn ? WHITE : BLACK][KING];
		if (in_check) {
			return -10000;
		} else {
			return 0; // stalemate
		}
	}
	if (next_move != 0) {
		t.next_move = next_move;
		t.score = alpha;
		t.depth = depth;
		tt[hash_index(board.hash_key) % hash_size] = t;
	}
	return alpha;
}

void Search::search_best_move(const Board& board, const bool white_turn, const list history) {
	start = clock.now();

	Board b2 = board;
	MoveList root_moves = get_moves(b2, white_turn);

	for (auto it = root_moves.begin(); it != root_moves.end(); ++it) {
		make_move(b2, *it);
		it->sort_score = nega_evaluate(b2, white_turn);
		unmake_move(b2, *it);
	}

	int alpha = INT_MIN;
	int beta = INT_MAX;
	int START_WINDOW_SIZE = 25;
	Move killers2[32][2];
	int quites_history[64][64] = { };
	Transposition * tt = new Transposition[hash_size];
	b2.hash_key = 0;

	uint64_t attacked_squares_by_opponent = get_attacked_squares(b2, !white_turn);
	bool in_check = attacked_squares_by_opponent & (white_turn ? b2.b[WHITE][KING] : b2.b[BLACK][KING]);

	for (int depth = 1; depth < 30;) {

		int score = alpha;
		int a = alpha;
		int b = beta;
		MoveList next_iteration_root_moves;
		int pv[depth];
		for (unsigned int i = 0; i < root_moves.size(); i++) {
			pick_next_move(root_moves, i);
			Move root_move = root_moves[i];
			if (b > a) { //strict?
				node_count++;
				bool legal_move = make_move(b2, root_move);
				if (!legal_move) {
					unmake_move(b2, root_move);
					continue;
				}
				int res = -1;
				for (auto hit = history.begin(); hit != history.end(); ++hit) {
					if (is_equal(b2, *hit)) {
						// draw by repetition
						res = 0;
					}
				}
				if (res == -1) {
					// check if stale mate
					bool opponent_in_check = get_attacked_squares(b2, white_turn)
												& (white_turn ? b2.b[BLACK][KING] : b2.b[WHITE][KING]);
					if (!opponent_in_check) {
						bool opponent_has_legal_move = false;
						MoveList oppenent_moves = get_moves(b2, !white_turn);
						for (auto it = oppenent_moves.begin(); it != oppenent_moves.end(); ++it) {
							bool legal_move = make_move(b2, *it);
							if (legal_move) {
								opponent_has_legal_move = true;
								unmake_move(b2,*it);
								break;
							}
							unmake_move(b2,*it);
						}
						if (!opponent_has_legal_move) {
							res = 0;
						}
					}
				}
				if (res == -1) {
					// for all moves except the first, search with a very narrow window to see if a full window search is necessary
					if (i > 0 && depth > 1) {
						res = -null_window_search(!white_turn, depth - 1, -a, b2, tt, in_check, killers2, quites_history, 1, 0);
						if (res > a) {
							// full window is necessary
							res = -alpha_beta(!white_turn, depth - 1, -b, -a, b2, tt, in_check, killers2, quites_history, 1, 0);
						} else {
							res = a - i; // keep sort order
						}
					} else {
						res = -alpha_beta(!white_turn, depth - 1, -b, -a, b2, tt, in_check, killers2, quites_history, 1, 0);
					}
				}
				unmake_move(b2, root_move);
				if (res > a && (!time_to_stop() || i == 0)) {
					score = res;
					a = res;
					for (int p = 1; p < depth; p++) {
						pv[p] = 0;
					}
					pv[0] = root_move.m;
					uint32_t next_pv_move = root_move.m;
					uint64_t hash = b2.hash_key;
					for (int p = 1; p < depth - 1; p++) {
						hash ^= move_hash(next_pv_move);
						Transposition next = tt[hash_index(hash) % hash_size];
						if (next.hash == hash_verification(hash) && next.next_move != 0) {
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
				root_move.sort_score = a - i; // keep sort order
			}
			next_iteration_root_moves.push_back(root_move);
		}
		int time_elapsed_last_depth_ms = std::chrono::duration_cast < std::chrono::milliseconds
				> (clock.now() - start).count();
		if (score > alpha && score < beta) {
			std::string pvstring = pvstring_from_stack(pv, depth);

			// uci info with score from engine's perspective
			std::cout << "info score cp " << score << " depth " << depth << " time "
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

		// if mate is found at this depth, just stop searching for better moves.
		// Cause there are none.. The best move at the last depth will prolong the inevitably as long as possible or deliver mate.
		if (abs(score) > 5000) {
			// deliver mate or be mated
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
