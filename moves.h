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
 * moves.h
 *
 *  Created on: Jun 16, 2013
 *      Author: Torbjörn Nilsson
 */

#ifndef MOVES_H_
#define MOVES_H_

#include "board.h"
#include <deque>
#include <string>

uint64_t south_fill(uint64_t l);

uint64_t north_fill(uint64_t l);

/*
 * returns mask with all files filled that are occupied with at least one bit of input
 */
inline uint64_t file_fill(uint64_t l) {
	return south_fill(l) | north_fill(l);
}

int see(const Position& position, const Move& capturing_move);

MoveList get_captures(const Position& position, const bool white_turn);

MoveList get_moves(const Position& position, const bool white_turn);

uint64_t get_attacked_squares(const Position& position, const bool white_turn);

bool is_illegal_castling_move(const Move& root_move, uint64_t attacked_squares_by_opponent);

/**
 * Returns true if legal move.
 */
bool make_move(Position& position, Move& move);

void unmake_move(Position& position, Move& move);

void init();

inline int piece_at_board(const Position& position, const uint64_t b, int color) {
	if (position.p[color][PAWN] & b) {
		return PAWN;
	}
	if (position.p[color][KNIGHT] & b) {
		return KNIGHT;
	}
	if (position.p[color][BISHOP] & b) {
		return BISHOP;
	}
	if (position.p[color][ROOK] & b) {
		return ROOK;
	}
	if (position.p[color][QUEEN] & b) {
		return QUEEN;
	}
	if (position.p[color][KING] & b) {
		return KING;
	}
	// if there is no piece at target square, then it must be an en passant capture
	return EN_PASSANT;
}

inline int piece_at_square(const Position& position, int square, int color) {
	return piece_at_board(position, (1ULL << square), color);
}

extern uint64_t from_randoms[64];
extern uint64_t to_randoms[64];


/**
 * To avoid hash collisions of similiar positions we xor move info with pre-generated 64-bit random numbers
 */
inline uint64_t move_hash(uint32_t move) {
	uint64_t move_64 = ((uint64_t) move) << 32 | move;
	return from_randoms[from_square(move)] ^ to_randoms[to_square(move)] ^ move_64;
}

inline void make_null_move(Position& position) {
	position.hash_key = position.hash_key ^ from_randoms[0];
}

inline void unmake_null_move(Position& position) {
	position.hash_key = position.hash_key ^ from_randoms[0];
}

#endif /* MOVES_H_ */
