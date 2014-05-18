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

MoveList get_captures(const Board& board, const bool white_turn);

MoveList get_children(const Board& board, const bool white_turn);

void make_move(Board& board, Move move);

void unmake_move(Board& board, Move move);

void init();

inline int piece_at_board(const Board& board, const uint64_t b, int color) {
	if (board.b[color][PAWN] & b) {
		return PAWN;
	}
	if (board.b[color][KNIGHT] & b) {
		return KNIGHT;
	}
	if (board.b[color][BISHOP] & b) {
		return BISHOP;
	}
	if (board.b[color][ROOK] & b) {
		return ROOK;
	}
	if (board.b[color][QUEEN] & b) {
		return QUEEN;
	}
	if (board.b[color][KING] & b) {
		return KING;
	}
	// if there is no piece at target square, then it must be an en passant capture
	return EN_PASSANT;
}

inline int piece_at_square(const Board& board, int square, int color) {
	return piece_at_board(board, (1ULL << square), color);
}

#endif /* MOVES_H_ */
