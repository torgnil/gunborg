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
 * eval.cpp
 *
 *  Created on: Jan 18, 2014
 *      Author: Torbjörn Nilsson
 */

#include "board.h"
#include "eval.h"
#include "moves.h"

// score in centipawns
int evaluate(const Board& board) {
	int score = 0;
	int white_bishops = 0;
	int black_bishops = 0;

	uint64_t white_pawn_protection_squares = ((board.b[WHITE][PAWN] & ~A_FILE) << 7)
			| ((board.b[WHITE][PAWN] & ~H_FILE) << 9);
	uint64_t black_pawn_protection_squares = ((board.b[BLACK][PAWN] & ~A_FILE) >> 9)
			| ((board.b[BLACK][PAWN] & ~H_FILE) >> 7);

	// The idea is if a white pawn is on any of these squares then it is not a passed pawn
	uint64_t black_pawn_blocking_squares = (board.b[BLACK][PAWN] >> 8) | black_pawn_protection_squares;
	black_pawn_blocking_squares = black_pawn_blocking_squares | (black_pawn_blocking_squares >> 8);
	black_pawn_blocking_squares = black_pawn_blocking_squares | (black_pawn_blocking_squares >> 8);
	black_pawn_blocking_squares = black_pawn_blocking_squares | (black_pawn_blocking_squares >> 8);
	black_pawn_blocking_squares = black_pawn_blocking_squares | (black_pawn_blocking_squares >> 8);

	uint64_t white_pawn_blocking_squares = (board.b[WHITE][PAWN] << 8) | white_pawn_protection_squares;
	white_pawn_blocking_squares = white_pawn_blocking_squares | (white_pawn_blocking_squares << 8);
	white_pawn_blocking_squares = white_pawn_blocking_squares | (white_pawn_blocking_squares << 8);
	white_pawn_blocking_squares = white_pawn_blocking_squares | (white_pawn_blocking_squares << 8);
	white_pawn_blocking_squares = white_pawn_blocking_squares | (white_pawn_blocking_squares << 8);

	uint64_t white_pawn_files = file_fill(board.b[WHITE][PAWN]);
	uint64_t black_pawn_files = file_fill(board.b[BLACK][PAWN]);

	uint64_t open_files = ~(white_pawn_files | black_pawn_files);

	uint64_t white_semi_open_files = ~white_pawn_files;
	uint64_t black_semi_open_files = ~black_pawn_files;

	uint64_t white_double_pawn_mask = north_fill(board.b[WHITE][PAWN] << 8);
	uint64_t black_double_pawn_mask = south_fill(board.b[BLACK][PAWN] >> 8);

	uint64_t white_pawns = board.b[WHITE][PAWN];
	bool end_game = (board.b[WHITE][QUEEN] == 0) && (board.b[BLACK][QUEEN] == 0);

	while (white_pawns) {
		int i = lsb_to_square(white_pawns);
		uint64_t square = lsb(white_pawns);
		score += WHITE_PAWN_SQUARE_TABLE[i];
		if ((black_pawn_blocking_squares & square) == 0) {
			score += PASSED_PAWN_BONUS;
		}
		if (square & white_pawn_protection_squares) {
			score += PROTECTED_PAWN_BONUS;
		}
		if (square & white_double_pawn_mask) {
			score -= DOUBLED_PAWN_PENALTY;
		}
		if (~(((square & ~A_FILE) >> 1) & white_pawn_files) && ~(((square & ~H_FILE) << 1) & white_pawn_files)) {
			score -= ISOLATED_PAWN_PENALTY;
		}
		white_pawns = reset_lsb(white_pawns);
	}
	uint64_t white_king = board.b[WHITE][KING];
	while (white_king) {
		uint64_t square = lsb(white_king);
		if (end_game) {
			int i = lsb_to_square(white_king);
			score += KING_SQUARE_TABLE_ENDGAME[i];
		} else {
			score += 10000;
			// king safety
			if (square == C1 || square == G1) {
				score += 25;
			}
			if (((white_king & ~ROW_8) << 8) & board.b[WHITE][PAWN]) {
				score += 10;
			}
			if (((white_king & ~NW_BORDER) << 7) & board.b[WHITE][PAWN]) {
				score += 10;
			}
			if (((white_king & ~NE_BORDER) << 9) & board.b[WHITE][PAWN]) {
				score += 10;
			}
		}
		white_king = reset_lsb(white_king);
	}
	uint64_t w_bishops = board.b[WHITE][BISHOP];
	while (w_bishops) {
		int i = lsb_to_square(w_bishops);
		score += BISHOP_SQUARE_TABLE[i];
		white_bishops++;
		w_bishops = reset_lsb(w_bishops);
	}
	uint64_t white_knights = board.b[WHITE][KNIGHT];
	while (white_knights) {
		int i = lsb_to_square(white_knights);
		score += KNIGHT_SQUARE_TABLE[i];
		white_knights = reset_lsb(white_knights);
	}
	uint64_t white_rooks = board.b[WHITE][ROOK];
	while (white_rooks) {
		int i = lsb_to_square(white_rooks);
		uint64_t square = lsb(white_rooks);
		score += WHITE_ROOK_SQUARE_TABLE[i];
		if (open_files & square) {
			score += OPEN_FILE_BONUS;
		} else if (white_semi_open_files & square) {
			score += SEMI_OPEN_FILE_BONUS;
		}
		white_rooks = reset_lsb(white_rooks);
	}
	uint64_t white_queens = board.b[WHITE][QUEEN];
	while (white_queens) {
		uint64_t square = lsb(white_queens);
		score += 900;
		if (open_files & square) {
			score += OPEN_FILE_BONUS;
		} else if (white_semi_open_files & square) {
			score += SEMI_OPEN_FILE_BONUS;
		}
		white_queens = reset_lsb(white_queens);
	}

	uint64_t black_pawns = board.b[BLACK][PAWN];
	while (black_pawns) {
		int i = lsb_to_square(black_pawns);
		uint64_t square = lsb(black_pawns);
		score -= BLACK_PAWN_SQUARE_TABLE[i];
		if ((white_pawn_blocking_squares & square) == 0) {
			score -= PASSED_PAWN_BONUS;
		}
		if (square & black_pawn_protection_squares) {
			score -= PROTECTED_PAWN_BONUS;
		}
		if (square & black_double_pawn_mask) {
			score += DOUBLED_PAWN_PENALTY;
		}
		if (~(((square & ~A_FILE) >> 1) & black_pawn_files) && ~(((square & ~H_FILE) << 1) & black_pawn_files)) {
			score += ISOLATED_PAWN_PENALTY;
		}
		black_pawns = reset_lsb(black_pawns);
	}
	uint64_t black_king = board.b[BLACK][KING];
	while (black_king) {
		uint64_t square = lsb(black_king);
		if (end_game) {
			int i = lsb_to_square(black_king);
			score -= KING_SQUARE_TABLE_ENDGAME[i];
		} else {
			score -= 10000;
			// king safety
			if (square == C8 || square == G8) {
				score -= 25; // king safety
			}
			if (((black_king & ~ROW_1) >> 8) & board.b[BLACK][PAWN]) {
				score -= 10;
			}
			if (((black_king & ~SE_BORDER) >> 7) & board.b[BLACK][PAWN]) {
				score -= 10;
			}
			if (((black_king & ~SW_BORDER) >> 9) & board.b[BLACK][PAWN]) {
				score -= 10;
			}
		}
		black_king = reset_lsb(black_king);
	}
	uint64_t b_bishops = board.b[BLACK][BISHOP];
	while (b_bishops) {
		int i = lsb_to_square(b_bishops);
		score -= BISHOP_SQUARE_TABLE[i];
		black_bishops++;
		b_bishops = reset_lsb(b_bishops);
	}
	uint64_t black_knights = board.b[BLACK][KNIGHT];
	while (black_knights) {
		int i = lsb_to_square(black_knights);
		score -= KNIGHT_SQUARE_TABLE[i];
		black_knights = reset_lsb(black_knights);
	}
	uint64_t black_rooks = board.b[BLACK][ROOK];
	while (black_rooks) {
		int i = lsb_to_square(black_rooks);
		uint64_t square = lsb(black_rooks);
		score -= BLACK_ROOK_SQUARE_TABLE[i];
		if (open_files & square) {
			score -= OPEN_FILE_BONUS;
		} else if (black_semi_open_files & square) {
			score -= SEMI_OPEN_FILE_BONUS;
		}
		black_rooks = reset_lsb(black_rooks);
	}
	uint64_t black_queens = board.b[BLACK][QUEEN];
	while (black_queens) {
		uint64_t square = lsb(black_queens);
		score -= 900;
		if (open_files & square) {
			score -= OPEN_FILE_BONUS;
		} else if (black_semi_open_files & square) {
			score -= SEMI_OPEN_FILE_BONUS;
		}
		black_queens = reset_lsb(black_queens);
	}

	if (white_bishops == 2) {
		score += 50;
	}
	if (black_bishops == 2) {
		score -= 50;
	}
	return score;
}
