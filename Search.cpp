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
#include <deque>
#include <iostream>
#include <limits.h>
#include <sstream>
#include <thread>

namespace gunborg {

const int MAX_CHECK_EXTENSION = 2;

Search::Search() {
	max_think_time_ms = 10000;
	node_count = 0;
	save_time = true;
}

inline bool Search::time_to_stop() {
	int time_elapsed = std::chrono::duration_cast < std::chrono::milliseconds > (clock.now() - start).count();
	return (time_elapsed > max_think_time_ms  && !pondering) || !should_run;
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
		bool null_move_not_allowed, Move (&killers)[32][2], uint64_t (&history)[64][64], int ply, int extension) {
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
		bool null_move_not_allowed, Move (&killers)[32][2], uint64_t (&history)[64][64], int ply, int extension) {
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
			if (beta - alpha > 1 && next_move != 0) {
				// we do not expect to find a better move
				// use a fast null window search to verify it!
				res = -null_window_search(!white_turn, depth - 1 - depth_reduction, -alpha, position, tt, null_move_not_allowed,
							killers, history, ply + 1, extension);
				if (res > alpha) {
					// score improved unexpected, we have to do a full window search
					res = -alpha_beta(!white_turn, depth - 1 - depth_reduction, -beta, -alpha, position, tt, null_move_not_allowed,
							killers, history, ply + 1, extension);
				}
			} else {
				res = -alpha_beta(!white_turn, depth - 1 - depth_reduction, -beta, -alpha, position, tt, null_move_not_allowed,
							killers, history, ply + 1, extension);
			}
			if (depth_reduction > 0 && res > alpha) {
				// score improved "unexpected" at reduced depth
				// re-search at normal depth
				res = -alpha_beta(!white_turn, depth - 1, -beta, -alpha, position, tt, null_move_not_allowed, killers, history,
						ply + 1, extension);
			}
		}

		unmake_move(position, move);
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

void Search::init_sort_score(const bool white_turn, MoveList& root_moves, Position& p, Transposition *tt) {
	// check for hit in transposition table
	Transposition tt_pv = tt[hash_index(p.hash_key) % hash_size];
	bool cache_hit = tt_pv.next_move != 0 && tt_pv.hash == hash_verification(p.hash_key);

	for (auto it = root_moves.begin(); it != root_moves.end(); ++it) {
		make_move(p, *it);
		it->sort_score = nega_evaluate(p, white_turn);
		if (cache_hit && it->m == tt_pv.next_move) {
			it->sort_score += 1000;
		}
		unmake_move(p, *it);
	}
}

bool Search::is_draw_by_repetition(const list& history, const Position& pos, const bool white_turn) {
	bool history_white_turn = true;
	for (auto hit = history.begin(); hit != history.end(); ++hit) {
		if (is_equal(pos, *hit) && white_turn != history_white_turn) {
			return true;
		}
		history_white_turn = !history_white_turn;
	}
	return false;
}

bool Search::is_stale_mate(const bool white_turn, Position& pos) {
	bool opponent_in_check = get_attacked_squares(pos, white_turn)
			& (white_turn ? pos.p[BLACK][KING] : pos.p[WHITE][KING]);
	if (opponent_in_check) {
		return false;
	}
	MoveList oppenent_moves = get_moves(pos, !white_turn);
	for (auto it = oppenent_moves.begin(); it != oppenent_moves.end(); ++it) {
		bool legal_move = make_move(pos, *it);
		if (legal_move) {
			unmake_move(pos, *it);
			return false;
		}
		unmake_move(pos, *it);
	}
	return true;
}

/*
 * search using alpha beta but with increasing aspiration window
 *
 */
int Search::aspiration_window_search(bool white_turn, int depth, int alpha, int beta, Position& pos, Transposition *tt,
		bool in_check, Move (&killers)[32][2], uint64_t (&history)[64][64]) {
	while (!time_to_stop()) {
		int move_score = -alpha_beta(!white_turn, depth - 1, -beta, -alpha, pos, tt, in_check, killers, history, 1, 0);
		if (move_score > alpha && move_score < beta) {
			return move_score;
		} else {
			int window_size = beta - alpha;
			if (move_score <= alpha) {
				// failed low, search again at same depth
				alpha = alpha - window_size;
			}
			if (move_score >= beta) {
				// failed high, search again at same depth
				beta = beta + window_size;
			}
		}
	}
	return alpha;
}

void Search::ponder() {
	pondering = true;
}

void Search::ponder_hit() {
	int time_elapsed = std::chrono::duration_cast < std::chrono::milliseconds > (clock.now() - start).count();
	max_think_time_ms += time_elapsed;
	pondering = false;
}

void Search::search_best_move(const Position& position, const bool white_turn, const list history, Transposition * tt) {
	start = clock.now();
	std::string best_move;
	std::string ponder_move = "";

	Position pos = position;
	MoveList root_moves = get_moves(pos, white_turn);

	init_sort_score(white_turn, root_moves, pos, tt);

	int alpha = INT_MIN;
	Move killers[32][2] = {};
	uint64_t quites_history[64][64] = {};

	uint64_t attacked_squares_by_opponent = get_attacked_squares(pos, !white_turn);
	bool in_check = attacked_squares_by_opponent & (white_turn ? pos.p[WHITE][KING] : pos.p[BLACK][KING]);

	for (int depth = 1; depth <= max_depth; depth++) {
		// moves sorted for the next depth
		MoveList next_iteration_root_moves;
		int pv[depth];

		for (unsigned int i = 0; i < root_moves.size(); i++) {
			pick_next_move(root_moves, i);
			Move root_move = root_moves[i];
			node_count++;
			bool legal_move = make_move(pos, root_move);
			if (!legal_move) {
				unmake_move(pos, root_move);
				continue;
			}
			int move_score;
			if (is_draw_by_repetition(history, pos, white_turn) || is_stale_mate(white_turn, pos)) {
				move_score = 0;
			} else {
				// for all moves except the first, search with a very narrow window to see if a full window search is necessary
				if (i > 0 && depth > 1) {
					move_score = -null_window_search(!white_turn, depth - 1, -alpha, pos, tt, in_check, killers, quites_history, 1, 0);
					if (move_score > alpha) {
						// full window search is necessary
						move_score = aspiration_window_search(white_turn, depth, alpha, alpha + WINDOW_SIZE, pos, tt,
								in_check, killers, quites_history);
					} else {
						move_score = alpha - i*500; // keep sort order
					}
				} else {
					if (alpha == INT_MIN) {
						move_score = aspiration_window_search(white_turn, depth, -START_WINDOW_SIZE, START_WINDOW_SIZE, pos, tt, in_check, killers, quites_history);
					} else {
						move_score = aspiration_window_search(white_turn, depth, alpha - START_WINDOW_SIZE,
								alpha + START_WINDOW_SIZE, pos, tt, in_check, killers, quites_history);
					}
				}
			}
			unmake_move(pos, root_move);
			if (time_to_stop()) {
				break;
			}
			root_move.sort_score = move_score;
			next_iteration_root_moves.push_back(root_move);

			if (move_score > alpha || i == 0) {
				alpha = move_score;
				for (int p = 1; p < depth; p++) {
					pv[p] = 0;
				}
				pv[0] = root_move.m;
				uint32_t next_pv_move = root_move.m;
				uint64_t hash = pos.hash_key;
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
				print_uci_info(pv, depth, move_score);
				std::string pvstring = pvstring_from_stack(pv, depth);
				std::stringstream ss(pvstring);
				getline(ss, best_move, ' ');
				if (depth > 1 && pv[1] != 0) {
					int pv_ponder = pv[1];
					ponder_move = long_algebraic_notation(1ULL << from_square(pv_ponder)) + long_algebraic_notation(1ULL << to_square(pv_ponder));
					if (is_promotion(pv_ponder)) {
						ponder_move += "q";
					}
				}
			}
		}
		if (time_to_stop()) {
			break;
		}
		print_uci_info(pv, depth, alpha);
		int time_elapsed_last_depth_ms = std::chrono::duration_cast < std::chrono::milliseconds
						> (clock.now() - start).count();
		if (!pondering && save_time && (4 * time_elapsed_last_depth_ms) > max_think_time_ms) {
			break;
		}
		// if mate is found at this depth, just stop searching for better moves.
		// Cause there are none.. The best move at the last depth will prolong the inevitably as long as possible or deliver mate.
		if (abs(alpha) > 5000) {
			// deliver mate or be mated
			break;
		}
		root_moves = next_iteration_root_moves;
	}
	while(pondering && should_run) {
		// wait for ponderhit (or stop)
		std::this_thread::sleep_for(std::chrono::milliseconds(3));
	}
	std::cout << "bestmove " << best_move;
	if (!ponder_move.empty()) {
		std::cout << " ponder " << ponder_move;
	}
	std::cout << std::endl << std::flush;
	return;
}

Search::~Search() {
}

} /* namespace gunborg */
