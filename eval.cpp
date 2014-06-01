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

namespace {

const int MAX_MATERIAL = 3100;

}

// score in centipawns
int evaluate(const Board& board) {
	int score = 0;

	uint64_t white_pawn_protection_squares = ((board.b[WHITE][PAWN] & ~A_FILE) << 7)
			| ((board.b[WHITE][PAWN] & ~H_FILE) << 9);
	uint64_t black_pawn_protection_squares = ((board.b[BLACK][PAWN] & ~A_FILE) >> 9)
			| ((board.b[BLACK][PAWN] & ~H_FILE) >> 7);

	// The idea is if a white pawn is on any of these squares then it is not a passed pawn
	uint64_t black_pawn_blocking_squares = south_fill((board.b[BLACK][PAWN] >> 8) | black_pawn_protection_squares);
	uint64_t white_pawn_blocking_squares = north_fill((board.b[WHITE][PAWN] << 8) | white_pawn_protection_squares);

	uint64_t white_pawn_files = file_fill(board.b[WHITE][PAWN]);
	uint64_t black_pawn_files = file_fill(board.b[BLACK][PAWN]);

	uint64_t open_files = ~(white_pawn_files | black_pawn_files);

	uint64_t white_semi_open_files = ~white_pawn_files & black_pawn_files;
	uint64_t black_semi_open_files = ~black_pawn_files & white_pawn_files;

	uint64_t white_double_pawn_mask = north_fill(board.b[WHITE][PAWN] << 8);
	uint64_t black_double_pawn_mask = south_fill(board.b[BLACK][PAWN] >> 8);

	int white_piece_material = pop_count(board.b[WHITE][QUEEN]) * 900
			+ pop_count(board.b[WHITE][ROOK]) * 500
			+ pop_count(board.b[WHITE][BISHOP]) * 300
			+ pop_count(board.b[WHITE][KNIGHT]) * 300;

	int black_piece_material = pop_count(board.b[BLACK][QUEEN]) * 900
			+ pop_count(board.b[BLACK][ROOK]) * 500
			+ pop_count(board.b[BLACK][BISHOP]) * 300
			+ pop_count(board.b[BLACK][KNIGHT]) * 300;

	int total_material = white_piece_material + black_piece_material;
	if (total_material == 300 && pop_count(board.b[WHITE][PAWN] | board.b[BLACK][PAWN]) == 0) {
		return 0; // draw by insufficient mating material
	}

	uint64_t white_pawns = board.b[WHITE][PAWN];

	uint64_t white_passed_pawns = ~black_pawn_blocking_squares & white_pawns;
	score += pop_count(white_passed_pawns) * PASSED_PAWN_BONUS;

	uint64_t white_doubled_pawns = white_double_pawn_mask & white_pawns;
	score -= pop_count(white_doubled_pawns) * DOUBLED_PAWN_PENALTY;

	uint64_t white_isolated_pawns = white_pawns & ~file_fill(white_pawn_protection_squares);
	score -= pop_count(white_isolated_pawns) * ISOLATED_PAWN_PENALTY;

	// a backward pawn cannot advance without being taken by opponent's pawn
	uint64_t black_dominated_stop_squares = ~north_fill(white_pawn_protection_squares) & black_pawn_protection_squares;
	uint64_t white_backward_pawns =  south_fill(black_dominated_stop_squares) & white_pawns;
	score -= pop_count(white_backward_pawns) * BACKWARD_PAWN_PENALTY;

	while (white_pawns) {
		int i = lsb_to_square(white_pawns);
		score += WHITE_PAWN_SQUARE_TABLE_ENDGAME[i] - WHITE_PAWN_SQUARE_TABLE_ENDGAME[i] * total_material/MAX_MATERIAL;
		score += WHITE_PAWN_SQUARE_TABLE[i] * total_material/MAX_MATERIAL;
		white_pawns = reset_lsb(white_pawns);
	}

	uint64_t white_king = board.b[WHITE][KING];
	while (white_king) {
		int i = lsb_to_square(white_king);

		score += KING_SQUARE_TABLE_ENDGAME[i] - KING_SQUARE_TABLE_ENDGAME[i] * total_material/MAX_MATERIAL;
		score += KING_SQUARE_TABLE[i] * total_material/MAX_MATERIAL;

		// king safety
		uint64_t pawn_mask = (7ULL << (i + 7)) & ROW_2;

		int king_safety_penalty = 0;

		uint64_t open_files_around_king = open_files & pawn_mask;
		king_safety_penalty += pop_count(open_files_around_king) * UNSAFE_KING_PENALTY;

		// pawns in front of the king
		uint64_t pawn_missing_front_of_king = ~board.b[WHITE][PAWN] & pawn_mask;
		king_safety_penalty += pop_count(pawn_missing_front_of_king) * UNSAFE_KING_PENALTY;

		// no pawns on the two squares in front of the king
		uint64_t pawn_missing_two_squares_front_of_king = ~board.b[WHITE][PAWN] & (pawn_missing_front_of_king << 8);
		king_safety_penalty += pop_count(pawn_missing_two_squares_front_of_king) * UNSAFE_KING_PENALTY;

		// scale the penalty by opponent material
		// we want to exchange pieces when king is unsafe
		king_safety_penalty *= black_piece_material;
		king_safety_penalty /= MAX_MATERIAL;

		score -= king_safety_penalty;
		white_king = reset_lsb(white_king);
	}

	uint64_t white_bishops = board.b[WHITE][BISHOP];
	if (pop_count(white_bishops) == 2) {
		score += 50;
	}
	while (white_bishops) {
		int i = lsb_to_square(white_bishops);
		score += BISHOP_SQUARE_TABLE[i];
		white_bishops = reset_lsb(white_bishops);
	}

	uint64_t white_knights = board.b[WHITE][KNIGHT];
	while (white_knights) {
		int i = lsb_to_square(white_knights);
		score += KNIGHT_SQUARE_TABLE[i];
		white_knights = reset_lsb(white_knights);
	}
	uint64_t white_rooks = board.b[WHITE][ROOK];
	uint64_t white_queens = board.b[WHITE][QUEEN];

	uint64_t white_open_file_pieces = open_files & (white_rooks | white_queens);
	score += pop_count(white_open_file_pieces)*OPEN_FILE_BONUS;

	uint64_t white_semi_open_file_pieces = white_semi_open_files & (white_rooks | white_queens);
	score += pop_count(white_semi_open_file_pieces)*SEMI_OPEN_FILE_BONUS;

	while (white_rooks) {
		int i = lsb_to_square(white_rooks);
		score += WHITE_ROOK_SQUARE_TABLE[i];
		white_rooks = reset_lsb(white_rooks);
	}
	while (white_queens) {
		score += 900;
		white_queens = reset_lsb(white_queens);
	}

	uint64_t black_pawns = board.b[BLACK][PAWN];

	uint64_t black_passed_pawns = ~white_pawn_blocking_squares & black_pawns;
	score -= pop_count(black_passed_pawns) * PASSED_PAWN_BONUS;

	uint64_t black_doubled_pawns = black_double_pawn_mask & black_pawns;
	score += pop_count(black_doubled_pawns) * DOUBLED_PAWN_PENALTY;

	uint64_t black_isolated_pawns = black_pawns & ~file_fill(black_pawn_protection_squares);
	score += pop_count(black_isolated_pawns) * ISOLATED_PAWN_PENALTY;

	uint64_t white_dominated_stop_squares = ~south_fill(black_pawn_protection_squares) & white_pawn_protection_squares;
	uint64_t black_backward_pawns =  north_fill(white_dominated_stop_squares) & black_pawns;
	score += pop_count(black_backward_pawns) * BACKWARD_PAWN_PENALTY;

	while (black_pawns) {
		int i = lsb_to_square(black_pawns);
		score -= BLACK_PAWN_SQUARE_TABLE_ENDGAME[i]  - BLACK_PAWN_SQUARE_TABLE_ENDGAME[i] * total_material/MAX_MATERIAL;
		score -= BLACK_PAWN_SQUARE_TABLE[i] * total_material/MAX_MATERIAL;
		black_pawns = reset_lsb(black_pawns);
	}
	uint64_t black_king = board.b[BLACK][KING];

	while (black_king) {
		int i = lsb_to_square(black_king);
		score -= KING_SQUARE_TABLE_ENDGAME[i] - KING_SQUARE_TABLE_ENDGAME[i] * total_material/MAX_MATERIAL;
		score -= KING_SQUARE_TABLE[i] * total_material/MAX_MATERIAL;

		int king_safety_penalty = 0;

		// king safety
		uint64_t pawn_mask = (7ULL << (i - 9)) & ROW_7;

		uint64_t open_files_around_king = open_files & pawn_mask;
		king_safety_penalty += pop_count(open_files_around_king) * UNSAFE_KING_PENALTY;

		// pawns in front of the king
		uint64_t pawn_missing_front_of_king = ~board.b[BLACK][PAWN] & pawn_mask;
		king_safety_penalty += pop_count(pawn_missing_front_of_king) * UNSAFE_KING_PENALTY;

		// no pawns on the to squares in front of the king
		uint64_t pawn_missing_two_squares_front_of_king = ~board.b[BLACK][PAWN] & (pawn_missing_front_of_king >> 8);
		king_safety_penalty += pop_count(pawn_missing_two_squares_front_of_king) * UNSAFE_KING_PENALTY;

		// scale the penalty by opponent material
		// we want to exchange pieces when king is unsafe
		king_safety_penalty *= white_piece_material;
		king_safety_penalty /= MAX_MATERIAL;

		score += king_safety_penalty;
		black_king = reset_lsb(black_king);
	}

	uint64_t black_bishops = board.b[BLACK][BISHOP];
	if (pop_count(black_bishops) == 2) {
		score -= 50;
	}
	while (black_bishops) {
		int i = lsb_to_square(black_bishops);
		score -= BISHOP_SQUARE_TABLE[i];
		black_bishops = reset_lsb(black_bishops);
	}

	uint64_t black_knights = board.b[BLACK][KNIGHT];
	while (black_knights) {
		int i = lsb_to_square(black_knights);
		score -= KNIGHT_SQUARE_TABLE[i];
		black_knights = reset_lsb(black_knights);
	}
	uint64_t black_rooks = board.b[BLACK][ROOK];
	uint64_t black_queens = board.b[BLACK][QUEEN];

	uint64_t black_open_file_pieces = open_files & (black_rooks | black_queens);
	score -= pop_count(black_open_file_pieces)*OPEN_FILE_BONUS;

	uint64_t black_semi_open_file_pieces = black_semi_open_files & (black_rooks | black_queens);
	score -= pop_count(black_semi_open_file_pieces)*SEMI_OPEN_FILE_BONUS;

	while (black_rooks) {
		int i = lsb_to_square(black_rooks);
		score -= BLACK_ROOK_SQUARE_TABLE[i];
		black_rooks = reset_lsb(black_rooks);
	}
	while (black_queens) {
		score -= 900;
		black_queens = reset_lsb(black_queens);
	}

	return score;
}
