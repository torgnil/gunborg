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
#include <math.h>

namespace {

const int MAX_MATERIAL = 3100;

}

int square_proximity[64][64];

// piece square tables [WHITE|BLACK][64]
// values are set by init_eval()
int pawn_square_table[2][64];
int pawn_square_table_endgame[2][64];
int knight_square_table[2][64];
int bishop_square_table[2][64];
int rook_square_table[2][64];
int queen_square_table[2][64];
int king_square_table_endgame[2][64];

// returns the score from the playing side's perspective
int nega_evaluate(const Position& position, const bool& white_turn) {
	return white_turn ? evaluate(position) : -evaluate(position);
}

int evaluate_side(const Position& position, const int& side, const int& piece_material, const int& opponent_piece_material) {
	int score = 0;
	int end_game_score = 0;
	int middle_game_score = 0;
	int opponent_king_proximity_bonus = 0;
	int opponent = 1 - side;

	int total_material = piece_material + opponent_piece_material;

	uint64_t king = position.p[side][KING];
	uint64_t opponent_king = position.p[opponent][KING];

	int king_square = lsb_to_square(king);
	int opponent_king_square = lsb_to_square(opponent_king);

	uint64_t opponent_squares = position.p[opponent][KING] | position.p[opponent][PAWN] | position.p[opponent][KNIGHT]
			| position.p[opponent][BISHOP] | position.p[opponent][ROOK] | position.p[opponent][QUEEN];

	uint64_t side_squares = position.p[side][KING] | position.p[side][PAWN] | position.p[side][KNIGHT]
			| position.p[side][BISHOP] | position.p[side][ROOK] | position.p[side][QUEEN];

	uint64_t occupied_squares = opponent_squares | side_squares;

	uint64_t pawns = position.p[side][PAWN];
	while (pawns) {
		int i = lsb_to_square(pawns);
		end_game_score += pawn_square_table_endgame[side][i];
		middle_game_score += pawn_square_table[side][i];
		pawns = reset_lsb(pawns);
	}

	end_game_score += king_square_table_endgame[side][king_square];
	middle_game_score += KING_SQUARE_TABLE[king_square];

	uint64_t bishops = position.p[side][BISHOP];
	if (pop_count(bishops) == 2) {
		score += BISHOP_PAIR_BONUS;
	}
	while (bishops) {
		int i = lsb_to_square(bishops);
		score += bishop_square_table[side][i];
		score += BISHOP_MOBILITY_BONUS * (pop_count(bishop_attacks(occupied_squares, i) & ~side_squares) - 5);
		opponent_king_proximity_bonus += square_proximity[opponent_king_square][i] * BISHOP_KING_PROXIMITY_BONUS;
		bishops = reset_lsb(bishops);
	}

	uint64_t knights = position.p[side][KNIGHT];
	while (knights) {
		int i = lsb_to_square(knights);
		score += knight_square_table[side][i];
		opponent_king_proximity_bonus += square_proximity[opponent_king_square][i] * KNIGHT_KING_PROXIMITY_BONUS;
		knights = reset_lsb(knights);
	}
	uint64_t rooks = position.p[side][ROOK];
	uint64_t queens = position.p[side][QUEEN];

	uint64_t side_pawn_files = file_fill(position.p[side][PAWN]);
	uint64_t opponent_pawn_files = file_fill(position.p[opponent][PAWN]);

	uint64_t open_files = ~(side_pawn_files | opponent_pawn_files);

	uint64_t semi_open_files = ~side_pawn_files & opponent_pawn_files;

	uint64_t open_file_pieces = open_files & (rooks | queens);
	score += pop_count(open_file_pieces)*OPEN_FILE_BONUS;

	uint64_t semi_open_file_pieces = semi_open_files & (rooks | queens);
	score += pop_count(semi_open_file_pieces)*SEMI_OPEN_FILE_BONUS;

	// king safety
	uint64_t pawn_mask = side == WHITE ? (7ULL << (king_square + 7)) & ROW_2 : (7ULL << (king_square - 9)) & ROW_7;

	while (rooks) {
		int i = lsb_to_square(rooks);
		score += rook_square_table[side][i];
		score += ROOK_MOBILITY_BONUS * (pop_count(rook_attacks(occupied_squares, i) & ~side_squares) - 5);
		opponent_king_proximity_bonus += square_proximity[opponent_king_square][i] * ROOK_KING_PROXIMITY_BONUS;
		rooks = reset_lsb(rooks);
	}
	while (queens) {
		int queen_square = lsb_to_square(queens);
		score += queen_square_table[side][queen_square];
		opponent_king_proximity_bonus += square_proximity[opponent_king_square][queen_square] * QUEEN_KING_PROXIMITY_BONUS;
		queens = reset_lsb(queens);
	}

	int king_safety_penalty = 0;

	uint64_t open_files_around_king = open_files & pawn_mask;
	king_safety_penalty += pop_count(open_files_around_king) * UNSAFE_KING_PENALTY;

	// pawns in front of the king
	uint64_t pawn_missing_front_of_king = ~position.p[side][PAWN] & pawn_mask;
	king_safety_penalty += pop_count(pawn_missing_front_of_king) * UNSAFE_KING_PENALTY;

	// no pawns on the two squares in front of the king
	uint64_t pawn_missing_two_squares_front_of_king = ~position.p[side][PAWN] &
			(side == WHITE ? (pawn_missing_front_of_king << 8) : (pawn_missing_front_of_king >> 8));
	king_safety_penalty += pop_count(pawn_missing_two_squares_front_of_king) * UNSAFE_KING_PENALTY;

	// scale the penalty by opponent material
	// we want to exchange pieces when king is unsafe
	king_safety_penalty *= opponent_piece_material;

	opponent_king_proximity_bonus *= piece_material;

	score += (opponent_king_proximity_bonus - king_safety_penalty - end_game_score * total_material + middle_game_score * total_material)/MAX_MATERIAL;

	score += end_game_score;

	return score;
}

bool is_drawish_endgame(const Position& position) {
	if (pop_count(
			position.p[WHITE][QUEEN] | position.p[BLACK][QUEEN]) != 0 ||
		pop_count(
			position.p[WHITE][KING] | position.p[BLACK][KING]) != 2 ) {
		return false;
	}
	// r vs R
	if (pop_count(
			position.p[WHITE][BISHOP] |
			position.p[WHITE][KNIGHT] |
			position.p[BLACK][BISHOP] |
			position.p[BLACK][KNIGHT]) == 0 &&
			pop_count(position.p[WHITE][ROOK]) == 1 &&
			pop_count(position.p[BLACK][ROOK]) == 1) {
		return true;
	}
	// r vs B
	if (pop_count(
			position.p[WHITE][KNIGHT] |
			position.p[BLACK][KNIGHT]) == 0 &&
				((pop_count(position.p[WHITE][ROOK]) == 1 &&
				pop_count(position.p[BLACK][ROOK]) == 0 &&
				pop_count(position.p[WHITE][BISHOP]) == 0 &&
				pop_count(position.p[BLACK][BISHOP]) == 1) ||
				(pop_count(position.p[WHITE][ROOK]) == 0 &&
				pop_count(position.p[BLACK][ROOK]) == 1 &&
				pop_count(position.p[WHITE][BISHOP]) == 1 &&
				pop_count(position.p[BLACK][BISHOP]) == 0))) {
		return true;
	}
	// r vs N
	if (pop_count(
			position.p[WHITE][BISHOP] |
			position.p[BLACK][BISHOP]) == 0 &&
				((pop_count(position.p[WHITE][ROOK]) == 1 &&
				pop_count(position.p[BLACK][ROOK]) == 0 &&
				pop_count(position.p[WHITE][KNIGHT]) == 0 &&
				pop_count(position.p[BLACK][KNIGHT]) == 1) ||
				(pop_count(position.p[WHITE][ROOK]) == 0 &&
				pop_count(position.p[BLACK][ROOK]) == 1 &&
				pop_count(position.p[WHITE][KNIGHT]) == 1 &&
				pop_count(position.p[BLACK][KNIGHT]) == 0))) {
		return true;
	}
	// opposite color bihops
	if (pop_count(
			position.p[WHITE][ROOK] |
			position.p[WHITE][KNIGHT] |
			position.p[BLACK][ROOK] |
			position.p[BLACK][KNIGHT]) == 0 &&
			pop_count(position.p[WHITE][BISHOP]) == 1 &&
			pop_count(position.p[BLACK][BISHOP]) == 1 &&
				(((WHITE_SQUARES & position.p[WHITE][BISHOP]) != 0 &&
				(WHITE_SQUARES & position.p[BLACK][BISHOP]) == 0) ||
				((BLACK_SQUARES & position.p[WHITE][BISHOP]) != 0 &&
				(BLACK_SQUARES & position.p[BLACK][BISHOP]) == 0))) {
		return true;
	}
	return false;
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


	score += evaluate_side(position, WHITE, white_piece_material, black_piece_material);
	score -= evaluate_side(position, BLACK, black_piece_material, white_piece_material);


	if (is_drawish_endgame(position)) {
		if (pop_count(position.p[WHITE][PAWN] | position.p[BLACK][PAWN]) == 0) {
			score = score / 8;
		} else {
			score = score * 6 / 8;
		}
	}

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
		rook_square_table[WHITE][i] = ROOK_SQUARE_TABLE[i];
		rook_square_table[BLACK][i] = ROOK_SQUARE_TABLE[63-i];
		knight_square_table[WHITE][i] = KNIGHT_SQUARE_TABLE[i];
		knight_square_table[BLACK][i] = KNIGHT_SQUARE_TABLE[63-i];
		bishop_square_table[WHITE][i] = BISHOP_SQUARE_TABLE[i];
		bishop_square_table[BLACK][i] = BISHOP_SQUARE_TABLE[63-i];
		king_square_table_endgame[WHITE][i] = KING_SQUARE_TABLE_ENDGAME[i];
		king_square_table_endgame[BLACK][i] = KING_SQUARE_TABLE_ENDGAME[63-i];
		pawn_square_table[WHITE][i] = PAWN_SQUARE_TABLE[i];
		pawn_square_table[BLACK][i] = PAWN_SQUARE_TABLE[63-i];
		pawn_square_table_endgame[WHITE][i] = PAWN_SQUARE_TABLE_ENDGAME[i];
		pawn_square_table_endgame[BLACK][i] = PAWN_SQUARE_TABLE_ENDGAME[63-i];
		queen_square_table[WHITE][i] = QUEEN_SQUARE_TABLE[i];
		queen_square_table[BLACK][i] = QUEEN_SQUARE_TABLE[63-i];
	}

}
