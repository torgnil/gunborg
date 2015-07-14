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
 * eval.cpp
 *
 *  Created on: Jan 18, 2014
 *      Author: Torbjörn Nilsson
 */

#include "board.h"
#include "eval.h"
#include "magic.h"
#include "moves.h"

namespace {

const int MAX_MATERIAL = 3100;

}

int square_proximity[64][64];

// returns the score from the playing side's perspective
int nega_evaluate(const Position& position, bool white_turn) {
	return white_turn ? evaluate(position) : -evaluate(position);
}

// score in centipawns
int evaluate(const Position& position) {
	uint64_t black_king = position.p[BLACK][KING];
	uint64_t white_king = position.p[WHITE][KING];
	if (black_king == 0) {
		return 10000;
	} else if (white_king == 0) {
		return -10000;
	}
	int black_king_square = lsb_to_square(black_king);
	int white_king_square = lsb_to_square(white_king);

	uint64_t black_squares = position.p[BLACK][KING] | position.p[BLACK][PAWN] | position.p[BLACK][KNIGHT]
			| position.p[BLACK][BISHOP] | position.p[BLACK][ROOK] | position.p[BLACK][QUEEN];

	uint64_t white_squares = position.p[WHITE][KING] | position.p[WHITE][PAWN] | position.p[WHITE][KNIGHT]
			| position.p[WHITE][BISHOP] | position.p[WHITE][ROOK] | position.p[WHITE][QUEEN];

	uint64_t occupied_squares = black_squares | white_squares;

	int white_piece_material = pop_count(position.p[WHITE][QUEEN]) * 900
			+ pop_count(position.p[WHITE][ROOK]) * 500
			+ pop_count(position.p[WHITE][BISHOP]) * 300
			+ pop_count(position.p[WHITE][KNIGHT]) * 300;

	int black_piece_material = pop_count(position.p[BLACK][QUEEN]) * 900
			+ pop_count(position.p[BLACK][ROOK]) * 500
			+ pop_count(position.p[BLACK][BISHOP]) * 300
			+ pop_count(position.p[BLACK][KNIGHT]) * 300;

	int total_material = white_piece_material + black_piece_material;
	if (total_material <= 300 && (position.p[WHITE][PAWN] | position.p[BLACK][PAWN]) == 0) {
		return 0; // draw by insufficient mating material
	}
	int score = 0;

	uint64_t white_pawn_protection_squares = ((position.p[WHITE][PAWN] & ~A_FILE) << 7)
			| ((position.p[WHITE][PAWN] & ~H_FILE) << 9);
	uint64_t black_pawn_protection_squares = ((position.p[BLACK][PAWN] & ~A_FILE) >> 9)
			| ((position.p[BLACK][PAWN] & ~H_FILE) >> 7);

	// The idea is if a white pawn is on any of these squares then it is not a passed pawn
	uint64_t black_pawn_blocking_squares = south_fill((position.p[BLACK][PAWN] >> 8) | black_pawn_protection_squares);
	uint64_t white_pawn_blocking_squares = north_fill((position.p[WHITE][PAWN] << 8) | white_pawn_protection_squares);

	uint64_t white_pawn_files = file_fill(position.p[WHITE][PAWN]);
	uint64_t black_pawn_files = file_fill(position.p[BLACK][PAWN]);

	uint64_t open_files = ~(white_pawn_files | black_pawn_files);

	uint64_t white_semi_open_files = ~white_pawn_files & black_pawn_files;
	uint64_t black_semi_open_files = ~black_pawn_files & white_pawn_files;

	uint64_t white_double_pawn_mask = north_fill(position.p[WHITE][PAWN] << 8);
	uint64_t black_double_pawn_mask = south_fill(position.p[BLACK][PAWN] >> 8);

	uint64_t white_pawns = position.p[WHITE][PAWN];

	uint64_t white_passed_pawns = ~black_pawn_blocking_squares & white_pawns;
	score += pop_count(white_passed_pawns) * PASSED_PAWN_BONUS * (MAX_MATERIAL - total_material) / MAX_MATERIAL;

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
		score += PAWN_SQUARE_TABLE_ENDGAME[i] - PAWN_SQUARE_TABLE_ENDGAME[i] * total_material/MAX_MATERIAL;
		score += PAWN_SQUARE_TABLE[i] * total_material/MAX_MATERIAL;
		white_pawns = reset_lsb(white_pawns);
	}

	score += KING_SQUARE_TABLE_ENDGAME[white_king_square] - KING_SQUARE_TABLE_ENDGAME[white_king_square] * total_material/MAX_MATERIAL;
	score += KING_SQUARE_TABLE[white_king_square] * total_material/MAX_MATERIAL;

	// king safety
	uint64_t pawn_mask = (7ULL << (white_king_square + 7)) & ROW_2;

	int king_safety_penalty = 0;

	uint64_t open_files_around_king = open_files & pawn_mask;
	king_safety_penalty += pop_count(open_files_around_king) * UNSAFE_KING_PENALTY;

	// pawns in front of the king
	uint64_t pawn_missing_front_of_king = ~position.p[WHITE][PAWN] & pawn_mask;
	king_safety_penalty += pop_count(pawn_missing_front_of_king) * UNSAFE_KING_PENALTY;

	// no pawns on the two squares in front of the king
	uint64_t pawn_missing_two_squares_front_of_king = ~position.p[WHITE][PAWN] & (pawn_missing_front_of_king << 8);
	king_safety_penalty += pop_count(pawn_missing_two_squares_front_of_king) * UNSAFE_KING_PENALTY;

	// scale the penalty by opponent material
	// we want to exchange pieces when king is unsafe
	king_safety_penalty *= black_piece_material;
	king_safety_penalty /= MAX_MATERIAL;

	score -= king_safety_penalty;

	int white_proximity_bonus = 0;
	uint64_t white_bishops = position.p[WHITE][BISHOP];
	if (pop_count(white_bishops) == 2) {
		score += BISHOP_PAIR_BONUS;
	}
	while (white_bishops) {
		int i = lsb_to_square(white_bishops);
		score += BISHOP_SQUARE_TABLE[i];
		score += BISHOP_MOBILITY_BONUS * (pop_count(bishop_attacks(occupied_squares, i) & ~white_squares) - 5);
		white_proximity_bonus += square_proximity[black_king_square][i] * BISHOP_KING_PROXIMITY_BONUS;
		white_bishops = reset_lsb(white_bishops);
	}

	uint64_t white_knights = position.p[WHITE][KNIGHT];
	while (white_knights) {
		int i = lsb_to_square(white_knights);
		score += KNIGHT_SQUARE_TABLE[i];
		white_proximity_bonus += square_proximity[black_king_square][i] * KNIGHT_KING_PROXIMITY_BONUS;
		white_knights = reset_lsb(white_knights);
	}
	uint64_t white_rooks = position.p[WHITE][ROOK];
	uint64_t white_queens = position.p[WHITE][QUEEN];

	uint64_t white_open_file_pieces = open_files & (white_rooks | white_queens);
	score += pop_count(white_open_file_pieces)*OPEN_FILE_BONUS;

	uint64_t white_semi_open_file_pieces = white_semi_open_files & (white_rooks | white_queens);
	score += pop_count(white_semi_open_file_pieces)*SEMI_OPEN_FILE_BONUS;

	while (white_rooks) {
		int i = lsb_to_square(white_rooks);
		score += ROOK_SQUARE_TABLE[i];
		score += ROOK_MOBILITY_BONUS * (pop_count(rook_attacks(occupied_squares, i) & ~white_squares) - 5);
		white_proximity_bonus += square_proximity[black_king_square][i] * ROOK_KING_PROXIMITY_BONUS;
		white_rooks = reset_lsb(white_rooks);
	}
	while (white_queens) {
		score += 900;
		int queen_square = lsb_to_square(white_queens);
		white_proximity_bonus += square_proximity[black_king_square][queen_square] * QUEEN_KING_PROXIMITY_BONUS;
		white_queens = reset_lsb(white_queens);
	}

	white_proximity_bonus *= white_piece_material;
	white_proximity_bonus /= MAX_MATERIAL;

	score += white_proximity_bonus;

	uint64_t black_pawns = position.p[BLACK][PAWN];

	uint64_t black_passed_pawns = ~white_pawn_blocking_squares & black_pawns;
	score -= pop_count(black_passed_pawns) * PASSED_PAWN_BONUS * (MAX_MATERIAL - total_material) / MAX_MATERIAL;

	uint64_t black_doubled_pawns = black_double_pawn_mask & black_pawns;
	score += pop_count(black_doubled_pawns) * DOUBLED_PAWN_PENALTY;

	uint64_t black_isolated_pawns = black_pawns & ~file_fill(black_pawn_protection_squares);
	score += pop_count(black_isolated_pawns) * ISOLATED_PAWN_PENALTY;

	uint64_t white_dominated_stop_squares = ~south_fill(black_pawn_protection_squares) & white_pawn_protection_squares;
	uint64_t black_backward_pawns =  north_fill(white_dominated_stop_squares) & black_pawns;
	score += pop_count(black_backward_pawns) * BACKWARD_PAWN_PENALTY;

	while (black_pawns) {
		int i = lsb_to_square(black_pawns);
		score -= PAWN_SQUARE_TABLE_ENDGAME[63 - i]  - PAWN_SQUARE_TABLE_ENDGAME[63 -i] * total_material/MAX_MATERIAL;
		score -= PAWN_SQUARE_TABLE[63 - i] * total_material/MAX_MATERIAL;
		black_pawns = reset_lsb(black_pawns);
	}

	score -= KING_SQUARE_TABLE_ENDGAME[black_king_square] - KING_SQUARE_TABLE_ENDGAME[black_king_square] * total_material/MAX_MATERIAL;
	score -= KING_SQUARE_TABLE[black_king_square] * total_material/MAX_MATERIAL;

	int black_king_safety_penalty = 0;

	// king safety
	uint64_t black_pawn_mask = (7ULL << (black_king_square - 9)) & ROW_7;

	uint64_t black_open_files_around_king = open_files & black_pawn_mask;
	black_king_safety_penalty += pop_count(black_open_files_around_king) * UNSAFE_KING_PENALTY;

	// pawns in front of the king
	uint64_t black_pawn_missing_front_of_king = ~position.p[BLACK][PAWN] & black_pawn_mask;
	black_king_safety_penalty += pop_count(black_pawn_missing_front_of_king) * UNSAFE_KING_PENALTY;

	// no pawns on the to squares in front of the king
	uint64_t black_pawn_missing_two_squares_front_of_king = ~position.p[BLACK][PAWN] & (black_pawn_missing_front_of_king >> 8);
	black_king_safety_penalty += pop_count(black_pawn_missing_two_squares_front_of_king) * UNSAFE_KING_PENALTY;

	// scale the penalty by opponent material
	// we want to exchange pieces when king is unsafe
	black_king_safety_penalty *= white_piece_material;
	black_king_safety_penalty /= MAX_MATERIAL;

	score += black_king_safety_penalty;

	int black_proximity_bonus = 0;
	uint64_t black_bishops = position.p[BLACK][BISHOP];
	if (pop_count(black_bishops) == 2) {
		score -= BISHOP_PAIR_BONUS;
	}
	while (black_bishops) {
		int i = lsb_to_square(black_bishops);
		score -= BISHOP_SQUARE_TABLE[63 - i];
		score -= BISHOP_MOBILITY_BONUS * (pop_count(bishop_attacks(occupied_squares, i) & ~black_squares) - 5);
		black_proximity_bonus += square_proximity[white_king_square][i] * BISHOP_KING_PROXIMITY_BONUS;
		black_bishops = reset_lsb(black_bishops);
	}

	uint64_t black_knights = position.p[BLACK][KNIGHT];
	while (black_knights) {
		int i = lsb_to_square(black_knights);
		score -= KNIGHT_SQUARE_TABLE[i];
		black_proximity_bonus += square_proximity[white_king_square][i] * KNIGHT_KING_PROXIMITY_BONUS;
		black_knights = reset_lsb(black_knights);
	}
	uint64_t black_rooks = position.p[BLACK][ROOK];
	uint64_t black_queens = position.p[BLACK][QUEEN];

	uint64_t black_open_file_pieces = open_files & (black_rooks | black_queens);
	score -= pop_count(black_open_file_pieces)*OPEN_FILE_BONUS;

	uint64_t black_semi_open_file_pieces = black_semi_open_files & (black_rooks | black_queens);
	score -= pop_count(black_semi_open_file_pieces)*SEMI_OPEN_FILE_BONUS;

	while (black_rooks) {
		int i = lsb_to_square(black_rooks);
		score -= ROOK_SQUARE_TABLE[63 - i];
		score -= ROOK_MOBILITY_BONUS * (pop_count(rook_attacks(occupied_squares, i) & ~black_squares) - 5);
		black_proximity_bonus += square_proximity[white_king_square][i] * ROOK_KING_PROXIMITY_BONUS;
		black_rooks = reset_lsb(black_rooks);
	}
	while (black_queens) {
		score -= 900;
		int queen_square = lsb_to_square(black_queens);
		black_proximity_bonus += square_proximity[white_king_square][queen_square] * QUEEN_KING_PROXIMITY_BONUS;
		black_queens = reset_lsb(black_queens);
	}

	black_proximity_bonus *= black_piece_material;
	black_proximity_bonus /= MAX_MATERIAL;

	score -= black_proximity_bonus;

	return score;
}

void init_eval() {
	for (int i = 0; i < 64; ++i) {
		for (int j = 0; j < 64; ++j) {
			int row_i = i / 8;
			int file_i = i % 8;
			int row_j = j / 8;
			int file_j = j % 8;
			square_proximity[i][j] = 7 - std::max(std::abs(file_i - file_j), std::abs(row_i - row_j));
		}
	}
}
