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
	save_time = true;
}

inline bool Search::time_to_stop() {
	int time_elapsed = std::chrono::duration_cast < std::chrono::milliseconds > (clock.now() - start).count();
	return time_elapsed > max_think_time_ms || !should_run;
}

bool is_equal(const Position& p1, const Position& p2) {
	if (p1.p[BLACK][PAWN] != p2.p[BLACK][PAWN]) {
		return false;
	}
	if (p1.p[BLACK][BISHOP] != p2.p[BLACK][BISHOP]) {
		return false;
	}
	if (p1.p[BLACK][KNIGHT] != p2.p[BLACK][KNIGHT]) {
		return false;
	}
	if (p1.p[BLACK][ROOK] != p2.p[BLACK][ROOK]) {
		return false;
	}
	if (p1.p[BLACK][QUEEN] != p2.p[BLACK][QUEEN]) {
		return false;
	}
	if (p1.p[BLACK][KING] != p2.p[BLACK][KING]) {
		return false;
	}
	if (p1.p[WHITE][PAWN] != p2.p[WHITE][PAWN]) {
		return false;
	}
	if (p1.p[WHITE][BISHOP] != p2.p[WHITE][BISHOP]) {
		return false;
	}
	if (p1.p[WHITE][KNIGHT] != p2.p[WHITE][KNIGHT]) {
		return false;
	}
	if (p1.p[WHITE][ROOK] != p2.p[WHITE][ROOK]) {
		return false;
	}
	if (p1.p[WHITE][QUEEN] != p2.p[WHITE][QUEEN]) {
		return false;
	}
	if (p1.p[WHITE][KING] != p2.p[WHITE][KING]) {
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

int Search::capture_quiescence_eval_search(bool white_turn, int alpha, int beta, Position& position) {
	if (position.p[WHITE][KING] == 0) {
		return white_turn ? -10000 : 10000;
	} else if (position.p[BLACK][KING] == 0) {
		return white_turn ? 10000 : -10000;
	}
	int static_eval = nega_evaluate(position, white_turn);
	if (static_eval > alpha) {
		alpha = static_eval;
	}
	if (static_eval >= beta) {
		return beta;
	}

	MoveList capture_moves = get_captures(position, white_turn);
	if (capture_moves.empty()) {
		// the end point of the quiescence search
		return static_eval;
	}
	bool has_legal_capture = false;
	for (unsigned int i = 0; i < capture_moves.size(); ++i) {
		pick_next_move(capture_moves, i);
		Move move = capture_moves[i];
		bool legal_move = make_move(position, move);
		if (!legal_move) {
			unmake_move(position, move);
			continue;
		}
		has_legal_capture = true;
		int res = -capture_quiescence_eval_search(!white_turn, -beta, -alpha, position);
		unmake_move(position, move);
		if (res >= beta) {
			return beta;
		}
		if (res > alpha) {
			alpha = res;
		}
		if (time_to_stop()) {
			return alpha;
		}
	}
	if (!has_legal_capture) {
		return static_eval;
	}
	return alpha;
}

int Search::null_window_search(bool white_turn, int depth, int beta, Position& position, Transposition *tt,
		bool null_move_not_allowed, Move (&killers)[32][2], int (&history)[64][64], int ply, int extension) {
	int alpha = beta - 1;
	return alpha_beta(white_turn, depth, alpha, beta, position, tt, null_move_not_allowed, killers, history, ply, extension);
}

inline bool should_prune(int depth, bool white_turn, Position& position, int alpha, int beta) {
	if (depth > 3) {
		return false;
	}
	if (depth == 1) {
		int static_eval = nega_evaluate(position, white_turn);
		// futility pruning. we do not hope to improve a position more than 300 in one move
		if (static_eval + 300 < alpha) {
			return true;
		}
	}
	if (depth == 2) {
		// extended futility pruning. we do not hope to improve a position more than 520 in two plies
		int static_eval = nega_evaluate(position, white_turn);
		if (static_eval + 520 < alpha) {
			return true;
		}
	}
	if (depth == 3) {
		// extended futility pruning. we do not hope to improve a position more than 900 in three plies
		int static_eval = nega_evaluate(position, white_turn);
		if (static_eval + 900 < alpha) {
			return true;
		}
	}
	return false;
}

int Search::alpha_beta(bool white_turn, int depth, int alpha, int beta, Position& position, Transposition *tt,
		bool null_move_not_allowed, Move (&killers)[32][2], int (&history)[64][64], int ply, int extension) {
	if (depth == 0) {
		return capture_quiescence_eval_search(white_turn, alpha, beta, position);
	}
	if (time_to_stop()) {
		return alpha;
	}
	if (should_prune(depth, white_turn, position, alpha, beta)) {
		return capture_quiescence_eval_search(white_turn, alpha, beta, position);
	}

	// null move heuristic
	if (!null_move_not_allowed && depth > 3) {
		// skip a turn and see if and see if we get a cut-off at shallower depth
		// it assumes:
		// 1. That the disadvantage of forfeiting one's turn is greater than the disadvantage of performing a shallower search.
		// 2. That the beta cut-offs prunes enough branches to be worth the time searching at reduced depth
		int R = 2; // depth reduction
		make_null_move(position);
		int res = -alpha_beta(!white_turn, depth - 1 - R, -beta, -alpha, position, tt, true, killers, history, ply + 1, extension);
		unmake_null_move(position);
		if (res >= beta) {
			return beta;
		}
	}

	// check for hit in transposition table
	Transposition tt_pv = tt[hash_index(position.hash_key) % hash_size];
	bool cache_hit = tt_pv.next_move != 0 && tt_pv.hash == hash_verification(position.hash_key);

	MoveList moves = get_moves(position, white_turn);
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
	t.hash = hash_verification(position.hash_key);
	int next_move = 0;
	bool has_legal_move = false;
	int static_eval = 0;
	for (unsigned int i = 0; i < moves.size(); ++i) {

		pick_next_move(moves, i);
		Move move = moves[i];
		node_count++;
		bool legal_move = make_move(position, move);
		if (!legal_move) {
			unmake_move(position, move);
			continue;
		}
		has_legal_move = true;

		int res;
		if (i < 5 && next_move == 0) {
			int depth_extention = 0;
			if (extension < MAX_CHECK_EXTENSION) {
				// if this is a checking move, extend the search one ply
				if (get_attacked_squares(position, white_turn) & position.p[white_turn ? BLACK : WHITE][KING]) {
					extension++;
					depth_extention = 1;
				}
			}
			res = -alpha_beta(!white_turn, depth - 1 + depth_extention, -beta, -alpha, position, tt, null_move_not_allowed,
				killers, history, ply + 1, extension);
		} else {
			// prune late moves that we do not expect to improve alpha
			if (i == 12 && depth <= 2) {
				static_eval = nega_evaluate(position, white_turn);
			}
			if (i >= 12 && depth <= 2 && static_eval + 100 < alpha) {
				unmake_move(position, move);
				break;
			}
			int depth_reduction = 0;
			// late move reduction.
			// we assume sort order is good enough to not search later moves as deep as the first
			if (depth > 2 && i > 5 && !is_capture(move.m)) {
				depth_reduction = depth > 5 && i > 20 ? 2 : 1;
			}
			// we do not expect to find a better move
			// use a fast null window search to verify it!
			res = -null_window_search(!white_turn, depth - 1 - depth_reduction, -alpha, position, tt, null_move_not_allowed,
										killers, history, ply + 1, extension);
			if (res > alpha) {
				// score improved unexpected, we have to do a full window search
				res = -alpha_beta(!white_turn, depth - 1 - depth_reduction, -beta, -alpha, position, tt, null_move_not_allowed,
							killers, history, ply + 1, extension);
			}
			if (depth_reduction > 0 && res > alpha && res < beta) {
				// score improved "unexpected" at reduced depth
				// re-search at normal depth
				res = -alpha_beta(!white_turn, depth - 1, -beta, -alpha, position, tt, null_move_not_allowed, killers, history,
						ply + 1, extension);
			}
		}

		unmake_move(position, move);
		if (time_to_stop()) {
			return alpha;
		}
		if (res >= beta) {
			if (!is_capture(move.m)) {
				if (killers[ply - 1][0].m != move.m) {
					killers[ply - 1][1] = killers[ply - 1][0];
					killers[ply - 1][0] = move;
				}
			}
			next_move = move.m;
			t.next_move = next_move;
			tt[hash_index(position.hash_key) % hash_size] = t;
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
		bool in_check = get_attacked_squares(position, !white_turn) & position.p[white_turn ? WHITE : BLACK][KING];
		if (in_check) {
			return -10000;
		} else {
			return 0; // stalemate
		}
	}
	if (next_move != 0) {
		t.next_move = next_move;
		tt[hash_index(position.hash_key) % hash_size] = t;
	}
	return alpha;
}

void Search::print_uci_info(int pv[], int depth, int score) {
	std::string pvstring = pvstring_from_stack(pv, depth);

	int time_elapsed_last_depth_ms = std::chrono::duration_cast < std::chrono::milliseconds
			> (clock.now() - start).count();

	// uci info with score from engine's perspective
	std::cout << "info score cp " << score << " depth " << depth << " time " << time_elapsed_last_depth_ms << " nodes "
			<< node_count << " pv " << pvstring << "\n" << std::flush;
}

void Search::search_best_move(const Position& position, const bool white_turn, const list history, Transposition * tt) {
	start = clock.now();

	Position p = position;
	MoveList root_moves = get_moves(p, white_turn);

	for (auto it = root_moves.begin(); it != root_moves.end(); ++it) {
		make_move(p, *it);
		it->sort_score = nega_evaluate(p, white_turn);
		unmake_move(p, *it);
	}

	int alpha = INT_MIN;
	int beta = INT_MAX;
	int START_WINDOW_SIZE = 25;
	Move killers2[32][2];
	int quites_history[64][64] = { };

	uint64_t attacked_squares_by_opponent = get_attacked_squares(p, !white_turn);
	bool in_check = attacked_squares_by_opponent & (white_turn ? p.p[WHITE][KING] : p.p[BLACK][KING]);

	for (int depth = 1; depth <= max_depth;) {

		int score = alpha;
		int a = alpha;
		int b = beta;
		MoveList next_iteration_root_moves;
		int pv[depth];
		for (unsigned int i = 0; i < root_moves.size(); i++) {
			pick_next_move(root_moves, i);
			Move root_move = root_moves[i];
			if (a < b) {
				node_count++;
				bool legal_move = make_move(p, root_move);
				if (!legal_move) {
					unmake_move(p, root_move);
					continue;
				}
				int res = -1;
				bool history_white_turn = true;
				for (auto hit = history.begin(); hit != history.end(); ++hit) {
					if (is_equal(p, *hit) && white_turn != history_white_turn) {
						// draw by repetition
						res = 0;
					}
					history_white_turn = !history_white_turn;
				}
				if (res == -1) {
					// check if stale mate
					bool opponent_in_check = get_attacked_squares(p, white_turn)
												& (white_turn ? p.p[BLACK][KING] : p.p[WHITE][KING]);
					if (!opponent_in_check) {
						bool opponent_has_legal_move = false;
						MoveList oppenent_moves = get_moves(p, !white_turn);
						for (auto it = oppenent_moves.begin(); it != oppenent_moves.end(); ++it) {
							bool legal_move = make_move(p, *it);
							if (legal_move) {
								opponent_has_legal_move = true;
								unmake_move(p,*it);
								break;
							}
							unmake_move(p,*it);
						}
						if (!opponent_has_legal_move) {
							res = 0;
						}
					}
				}
				if (res == -1) {
					// for all moves except the first, search with a very narrow window to see if a full window search is necessary
					if (i > 0 && depth > 1) {
						res = -null_window_search(!white_turn, depth - 1, -a, p, tt, in_check, killers2, quites_history, 1, 0);
						if (res > a) {
							// full window is necessary
							res = -alpha_beta(!white_turn, depth - 1, -b, -a, p, tt, in_check, killers2, quites_history, 1, 0);
						} else {
							res = a - i*500; // keep sort order
						}
					} else {
						res = -alpha_beta(!white_turn, depth - 1, -b, -a, p, tt, in_check, killers2, quites_history, 1, 0);
					}
				}
				unmake_move(p, root_move);
				if (res > a && (!time_to_stop() || i == 0)) {
					score = res;
					a = res;
					for (int p = 1; p < depth; p++) {
						pv[p] = 0;
					}
					pv[0] = root_move.m;
					uint32_t next_pv_move = root_move.m;
					uint64_t hash = p.hash_key;
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
					if (score > alpha && score < beta) {
						print_uci_info(pv, depth, score);
						std::string pvstring = pvstring_from_stack(pv, depth);
						std::stringstream ss(pvstring);
						getline(ss, best_move, ' ');
					}
					root_move.sort_score = res;
				} else {
					root_move.sort_score = a - i*500;
				}
			} else {
				// beta fail
				// we just know that the rest of the moves are worse than the best move, or is this un-reachable code?
				root_move.sort_score = a - i*500; // keep sort order
			}
			next_iteration_root_moves.push_back(root_move);
		}
		int time_elapsed_last_depth_ms = std::chrono::duration_cast < std::chrono::milliseconds
				> (clock.now() - start).count();
		if (score > alpha && score < beta) {
			print_uci_info(pv, depth, score);
			std::string pvstring = pvstring_from_stack(pv, depth);
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
		if (save_time && (4 * time_elapsed_last_depth_ms) > max_think_time_ms) {
			break;
		}
		depth++;
	}
	std::cout << "bestmove " << best_move << std::endl << std::flush;
	return;
}

Search::~Search() {
}

} /* namespace gunborg */
