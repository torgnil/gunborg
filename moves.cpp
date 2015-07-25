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
 * moves.cpp
 *
 *  Created on: Jun 16, 2013
 *      Author: Torbjörn Nilsson
 */

#include "moves.h"
#include "magic.h"
#include <stdlib.h>

int rook_castle_to_squares[64];
int rook_castle_from_squares[64];

// pre-calculated move masks

uint64_t knight_moves[64];
uint64_t king_moves[64];

const int PIECE_VALUES[7] = {100, 300, 300, 500, 900, 10000, 100};


uint64_t south_fill(uint64_t l) {
	l |= l >> 8; // OR 1 row
	l |= l >> 16; // OR 2 rows
	l |= l >> 32; // OR 4 rows
	return l;
}

uint64_t north_fill(uint64_t l) {
	l |= l << 8; // OR 1 row
	l |= l << 16; // OR 2 rows
	l |= l << 32; // OR 4 rows
	return l;
}

bool make_move(Position& position, Move move) {
	position.p[color(move.m)][piece(move.m)] &= ~(1ULL << from_square(move.m));
	position.p[color(move.m)][piece(move.m)] |= (1ULL << to_square(move.m));
	uint64_t meta_info = position.meta_info_stack.back();
	int captured_piece = captured_piece(move.m);
	if (captured_piece != EMPTY) {
		int captured_color = color(move.m) ^ 1;
		if (captured_piece != EN_PASSANT) {
			position.p[captured_color][captured_piece] &= ~(1ULL << to_square(move.m));
		} else {
			position.p[captured_color][PAWN] &= ~(1ULL << (to_square(move.m) - 8 + (color(move.m) * 16)));
		}
	}
	int promotion_piece = promotion_piece(move.m);
	if (promotion_piece != EMPTY) {
		position.p[color(move.m)][piece(move.m)] &= ~(1ULL << to_square(move.m));
		position.p[color(move.m)][promotion_piece] |= (1ULL << to_square(move.m));
	}
	bool illegal_castling = false;
	if (is_castling(move.m)) {
		if (is_illegal_castling_move(move, get_attacked_squares(position, color(move.m)))) {
			illegal_castling = true;
		}
		position.p[color(move.m)][ROOK] &= ~(1ULL << rook_castle_from_squares[to_square(move.m)]);
		position.p[color(move.m)][ROOK] |= (1ULL << rook_castle_to_squares[to_square(move.m)]);
	}
	if (piece(move.m) == KING) {
		meta_info &= ~(ROW_1 << (56 * color(move.m)));
	}
	if (piece(move.m) == ROOK) {
		if (from_square(move.m) == 0) {
			meta_info &= ~C1;
		}
		if (from_square(move.m) == 7) {
			meta_info &= ~G1;
		}
		if (from_square(move.m) == 56) {
			meta_info &= ~C8;
		}
		if (from_square(move.m) == 63) {
			meta_info &= ~G8;
		}
	}
	meta_info &= clear_en_passant_mask;
	// set en passant square
	if (piece(move.m) == PAWN && abs(to_square(move.m) - from_square(move.m)) == 16) {
		if (color(move.m) == WHITE) {
			meta_info |= (1ULL << from_square(move.m)) << 8;
		} else {
			meta_info |= (1ULL << from_square(move.m)) >> 8;
		}
	}
	position.meta_info_stack.push_back(meta_info);
	position.hash_key = position.hash_key ^ move_hash(move.m);

	if (illegal_castling || (get_attacked_squares(position, color(move.m)) & position.p[color(move.m)][KING])) {
		return false;
	}
	return true;
}

void unmake_move(Position& position, Move& move) {
	position.p[color(move.m)][piece(move.m)] |= (1ULL << from_square(move.m));
	position.p[color(move.m)][piece(move.m)] &= ~(1ULL << to_square(move.m));
	int captured_piece = captured_piece(move.m);
	if (captured_piece != EMPTY) {
		int captured_color = color(move.m) ^ 1;
		if (captured_piece != EN_PASSANT) {
			position.p[captured_color][captured_piece] |= (1ULL << to_square(move.m));
		} else {
			position.p[captured_color][PAWN] |= (1ULL << (to_square(move.m) - 8 + (color(move.m) * 16)));
		}
	}
	int promotion_piece = promotion_piece(move.m);
	if (promotion_piece != EMPTY) {
		position.p[color(move.m)][promotion_piece] &= ~(1ULL << to_square(move.m));
	}
	if (is_castling(move.m)) {
		position.p[color(move.m)][ROOK] |= (1ULL << rook_castle_from_squares[to_square(move.m)]);
		position.p[color(move.m)][ROOK] &= ~(1ULL << rook_castle_to_squares[to_square(move.m)]);
	}
	// reverse meta-info by poping the last element from the stack
	position.meta_info_stack.pop_back();
	position.hash_key = position.hash_key ^ move_hash(move.m);
}


uint64_t get_attacked_squares(const Position& position, const bool white_turn, uint64_t occupied_squares) {
	uint64_t attacked_squares = 0;
	uint64_t black_squares = position.p[BLACK][KING] | position.p[BLACK][PAWN] | position.p[BLACK][KNIGHT]
			| position.p[BLACK][BISHOP] | position.p[BLACK][ROOK] | position.p[BLACK][QUEEN];

	uint64_t white_squares = position.p[WHITE][KING] | position.p[WHITE][PAWN] | position.p[WHITE][KNIGHT]
			| position.p[WHITE][BISHOP] | position.p[WHITE][ROOK] | position.p[WHITE][QUEEN];

	int side = white_turn ? WHITE : BLACK;
	uint64_t side_squares = white_turn ? white_squares : black_squares;

	// knight moves
	uint64_t knights = position.p[side][KNIGHT];
	while (knights) {
		int from = lsb_to_square(knights);
		attacked_squares |= knight_moves[from];
		knights = reset_lsb(knights);
	}
	// bishop moves
	uint64_t bishops = position.p[side][BISHOP];
	while (bishops) {
		int from = lsb_to_square(bishops);
		attacked_squares |= bishop_attacks(occupied_squares, from);
		bishops = reset_lsb(bishops);
	}
	// rook moves
	uint64_t rooks = position.p[side][ROOK];
	while (rooks) {
		int from = lsb_to_square(rooks);
		attacked_squares |= rook_attacks(occupied_squares, from);
		rooks = reset_lsb(rooks);
	}
	// queen moves
	uint64_t queens = position.p[side][QUEEN];
	while (queens) {
		int from = lsb_to_square(queens);
		attacked_squares |= queen_attacks(occupied_squares, from);
		queens = reset_lsb(queens);
	}
	// king moves
	uint64_t king = position.p[side][KING];
	int from = lsb_to_square(king);
	attacked_squares |= king_moves[from];

	if (white_turn) {
		// white pawn captures
		attacked_squares |= (position.p[WHITE][PAWN] & ~NW_BORDER) << 7;
		attacked_squares |= (position.p[WHITE][PAWN] & ~NE_BORDER) << 9;
	} else {
		// black pawn captures
		attacked_squares |= (position.p[BLACK][PAWN] & ~SW_BORDER) >> 9;
		attacked_squares |= (position.p[BLACK][PAWN] & ~SE_BORDER) >> 7;
	}

	attacked_squares &= ~side_squares;
	return attacked_squares;
}

struct SEEInfo {
	uint64_t attacking_pieces[2][6] = {}; //[WHITE|BLACK][PAWN ... KING]
	uint64_t occupied_squares = 0;
};

int find_and_reset_least_valuable_piece(SEEInfo& see_info, const int& side,const int& square) {
	if (see_info.attacking_pieces[side][PAWN]) {
		uint64_t lsb = lsb(see_info.attacking_pieces[side][PAWN]);
		// reset piece on the occupied_squares bb
		see_info.occupied_squares &= ~lsb;
		see_info.attacking_pieces[side][PAWN] = reset_lsb(see_info.attacking_pieces[side][PAWN]);
		return 100;
	}
	if (see_info.attacking_pieces[side][KNIGHT]) {
		uint64_t lsb = lsb(see_info.attacking_pieces[side][KNIGHT]);
		// reset piece on the occupied_squares bb
		see_info.occupied_squares &= ~lsb;
		see_info.attacking_pieces[side][KNIGHT] = reset_lsb(see_info.attacking_pieces[side][KNIGHT]);
		return 300;
	}
	// find attacks for bishop, rooks and queens (inluding x-rays)
	uint64_t attacking_bishops = see_info.attacking_pieces[side][BISHOP] & bishop_attacks(see_info.occupied_squares, square);
	if (attacking_bishops) {
		uint64_t lsb = lsb(attacking_bishops);
		see_info.occupied_squares &= ~lsb;
		see_info.attacking_pieces[side][BISHOP] = reset_lsb(see_info.attacking_pieces[side][BISHOP]);
		return 300;
	}
	uint64_t attacking_rooks = see_info.attacking_pieces[side][ROOK] & rook_attacks(see_info.occupied_squares, square);
	if (attacking_rooks) {
		uint64_t lsb = lsb(attacking_rooks);
		see_info.occupied_squares &= ~lsb;
		see_info.attacking_pieces[side][ROOK] = reset_lsb(see_info.attacking_pieces[side][ROOK]);
		return 500;
	}
	uint64_t attacking_queens = see_info.attacking_pieces[side][QUEEN] & queen_attacks(see_info.occupied_squares, square);
	if (attacking_queens) {
		uint64_t lsb = lsb(attacking_queens);
		see_info.occupied_squares &= ~lsb;
		see_info.attacking_pieces[side][QUEEN] = reset_lsb(see_info.attacking_pieces[side][QUEEN]);
		return 900;
	}
	if (see_info.attacking_pieces[side][KING]) {
		uint64_t lsb = lsb(see_info.attacking_pieces[side][KING]);
		// reset piece on the occupied_squares bb
		see_info.occupied_squares &= ~lsb;
		see_info.attacking_pieces[side][KING] = reset_lsb(see_info.attacking_pieces[side][KING]);
		return 10000;
	}
	return 0;
}

int see(const Position& position, const Move& capturing_move) {
	if (captured_piece(capturing_move.m) > KING) {
		return 0; // special move - ignore
	}
	int captured_piece_value = PIECE_VALUES[captured_piece(capturing_move.m)];
	int capturing_piece_value = PIECE_VALUES[piece(capturing_move.m)];
	if (captured_piece_value >= capturing_piece_value) {
		return captured_piece_value - capturing_piece_value;
	}
	int square = to_square(capturing_move.m);

	uint64_t bb_square = 1L << square;

	uint64_t black_squares = position.p[BLACK][KING]
			| position.p[BLACK][PAWN]
			| position.p[BLACK][KNIGHT]
			| position.p[BLACK][BISHOP]
			| position.p[BLACK][ROOK]
			| position.p[BLACK][QUEEN];

	uint64_t white_squares = position.p[WHITE][KING]
			| position.p[WHITE][PAWN]
			| position.p[WHITE][KNIGHT]
			| position.p[WHITE][BISHOP]
			| position.p[WHITE][ROOK]
			| position.p[WHITE][QUEEN];

	SEEInfo see_info;

	see_info.occupied_squares = black_squares | white_squares;

	// all pieces attacking the square that are independent of x-rays
	see_info.attacking_pieces[WHITE][PAWN] |= position.p[WHITE][PAWN] & ((bb_square & ~SW_BORDER) >> 9);
	see_info.attacking_pieces[WHITE][PAWN] |= position.p[WHITE][PAWN] & ((bb_square & ~SE_BORDER) >> 7);

	see_info.attacking_pieces[BLACK][PAWN] |= position.p[BLACK][PAWN] & ((bb_square & ~NW_BORDER) << 7);
	see_info.attacking_pieces[BLACK][PAWN] |= position.p[BLACK][PAWN] & ((bb_square & ~NE_BORDER) << 9);

	see_info.attacking_pieces[WHITE][KNIGHT] |= position.p[WHITE][KNIGHT] & knight_moves[square];
	see_info.attacking_pieces[BLACK][KNIGHT] |= position.p[BLACK][KNIGHT] & knight_moves[square];

	see_info.attacking_pieces[WHITE][KING] |= position.p[WHITE][KING] & king_moves[square];
	see_info.attacking_pieces[BLACK][KING] |= position.p[BLACK][KING] & king_moves[square];

	see_info.attacking_pieces[WHITE][BISHOP] = position.p[WHITE][BISHOP];
	see_info.attacking_pieces[BLACK][BISHOP] = position.p[BLACK][BISHOP];
	see_info.attacking_pieces[WHITE][ROOK] = position.p[WHITE][ROOK];
	see_info.attacking_pieces[BLACK][ROOK] = position.p[BLACK][ROOK];
	see_info.attacking_pieces[WHITE][QUEEN] = position.p[WHITE][QUEEN];
	see_info.attacking_pieces[BLACK][QUEEN] = position.p[BLACK][QUEEN];

	int side = color(capturing_move.m);
	// reset the attacking piece
	see_info.attacking_pieces[side][piece(capturing_move.m)] &= ~(1ULL << from_square(capturing_move.m));
	see_info.occupied_squares &= ~(1ULL << from_square(capturing_move.m));

	int gain_swap_list[32] = {};

	gain_swap_list[0] = captured_piece_value;
	gain_swap_list[1] = capturing_piece_value - gain_swap_list[0];
	int d = 1;
	while (true) {
		int move_side = (side + d) & 1;
		int least_valueable_attacker_value = find_and_reset_least_valuable_piece(see_info, move_side,
				square);
		if (least_valueable_attacker_value) {
			// in check after capture?
			if (get_attacked_squares(position, move_side != WHITE, see_info.occupied_squares) & position.p[move_side][KING]) {
				//illegal move
				break;
			}
			d++;
			gain_swap_list[d] = least_valueable_attacker_value - gain_swap_list[d - 1];
			if (std::max(-gain_swap_list[d - 1], gain_swap_list[d]) < 0) {
				break;
			}
		} else {
			break;
		}
	}
	d--;
	while (d) {
		gain_swap_list[d - 1] = -std::max(-gain_swap_list[d - 1], gain_swap_list[d]);
		d--;
	}
	return gain_swap_list[0];

}

inline void add_quite_move(const int& from, const int& to, const int& color, const int& piece, MoveList& moves,
		const int promotion) {
	Move move;
	move.m = to_move(from, to, piece, color, promotion);
	move.sort_score = 0;
	moves.push_back(move);
}

inline void add_capture_move(const int& from, const int& to, const int& color, const int& piece, int captured_piece,
		MoveList& moves, const int promotion, const Position& position) {
	Move move;
	move.m = to_capture_move(from, to, piece, captured_piece, color, promotion);

	move.sort_score = PIECE_VALUES[captured_piece] - PIECE_VALUES[piece];
	if (move.sort_score < 0) {
		move.sort_score = see(position, move);
	}
	move.sort_score += 1000000;
	moves.push_front(move);
}

inline void add_castle_move(int from, int to, int color, MoveList& moves) {
	Move move;
	move.m = to_castle_move(from, to, color);
	move.sort_score = 1;
	moves.push_front(move);
}

uint64_t get_attacked_squares(const Position& position, const bool white_turn) {
	uint64_t black_squares = position.p[BLACK][KING] | position.p[BLACK][PAWN] | position.p[BLACK][KNIGHT]
			| position.p[BLACK][BISHOP] | position.p[BLACK][ROOK] | position.p[BLACK][QUEEN];

	uint64_t white_squares = position.p[WHITE][KING] | position.p[WHITE][PAWN] | position.p[WHITE][KNIGHT]
			| position.p[WHITE][BISHOP] | position.p[WHITE][ROOK] | position.p[WHITE][QUEEN];

	uint64_t occupied_squares = black_squares | white_squares;

	return get_attacked_squares(position, white_turn, occupied_squares);
}

MoveList get_captures(const Position& position, const bool white_turn) {
	MoveList moves;
	if ((position.p[WHITE][KING] == 0) || (position.p[BLACK][KING]) == 0) {
		return moves;
	}
	uint64_t black_squares = position.p[BLACK][KING] | position.p[BLACK][PAWN] | position.p[BLACK][KNIGHT]
			| position.p[BLACK][BISHOP] | position.p[BLACK][ROOK] | position.p[BLACK][QUEEN];

	uint64_t white_squares = position.p[WHITE][KING] | position.p[WHITE][PAWN] | position.p[WHITE][KNIGHT]
			| position.p[WHITE][BISHOP] | position.p[WHITE][ROOK] | position.p[WHITE][QUEEN];

	uint64_t occupied_squares = black_squares | white_squares;
	uint64_t meta_info = position.meta_info_stack.back();

	int side = white_turn ? WHITE : BLACK;
	int opponent = 1 - side;

	uint64_t opponent_squares = white_turn ? black_squares : white_squares;

	// knight moves
	uint64_t knights = position.p[side][KNIGHT];
	while (knights) {
		int from = lsb_to_square(knights);
		uint64_t to_squares = knight_moves[from] & opponent_squares;
		while (to_squares) {
			int to = lsb_to_square(to_squares);
			uint64_t lsb = lsb(to_squares);
			add_capture_move(from, to, side, KNIGHT, piece_at_board(position, lsb, opponent), moves, EMPTY, position);
			to_squares -= lsb;
		}
		knights = reset_lsb(knights);
	}
	// bishop moves
	uint64_t bishops = position.p[side][BISHOP];
	while (bishops) {
		int from = lsb_to_square(bishops);
		uint64_t to_squares = bishop_attacks(occupied_squares, from) & opponent_squares;
		while (to_squares) {
			int to = lsb_to_square(to_squares);
			uint64_t lsb = lsb(to_squares);
			add_capture_move(from, to, side, BISHOP, piece_at_board(position, lsb, opponent), moves, EMPTY, position);
			to_squares -= lsb;
		}
		bishops = reset_lsb(bishops);
	}
	// rook moves
	uint64_t rooks = position.p[side][ROOK];
	while (rooks) {
		int from = lsb_to_square(rooks);
		uint64_t to_squares = rook_attacks(occupied_squares, from) & opponent_squares;
		while (to_squares) {
			int to = lsb_to_square(to_squares);
			uint64_t lsb = lsb(to_squares);
			add_capture_move(from, to, side, ROOK, piece_at_board(position, lsb, opponent), moves, EMPTY, position);
			to_squares -= lsb;
		}
		rooks = reset_lsb(rooks);
	}
	// queen moves
	uint64_t queens = position.p[side][QUEEN];
	while (queens) {
		int from = lsb_to_square(queens);
		uint64_t to_squares = queen_attacks(occupied_squares, from) & opponent_squares;
		while (to_squares) {
			int to = lsb_to_square(to_squares);
			uint64_t lsb = lsb(to_squares);
			add_capture_move(from, to, side, QUEEN, piece_at_board(position, lsb, opponent), moves, EMPTY, position);
			to_squares -= lsb;
		}
		queens = reset_lsb(queens);
	}

	// king moves
	uint64_t king = position.p[side][KING];
	int from = lsb_to_square(king);
	uint64_t to_squares = king_moves[from] & opponent_squares;
	while (to_squares) {
		int to = lsb_to_square(to_squares);
		uint64_t lsb = lsb(to_squares);
		add_capture_move(from, to, side, KING, piece_at_board(position, lsb, opponent), moves, EMPTY, position);
		to_squares -= lsb;
	}

	if (white_turn) {
		// white pawn captures
		uint64_t capture_squares_w = ((position.p[WHITE][PAWN] & ~NW_BORDER) << 7) & (black_squares | (meta_info & ROW_6));
		while (capture_squares_w) {
			int to = lsb_to_square(capture_squares_w);
			int from = to - 7;
			if (to <= 55) {
				add_capture_move(from, to, WHITE, PAWN, piece_at_square(position, to, BLACK), moves, EMPTY, position);
			} else {
				add_capture_move(from, to, WHITE, PAWN, piece_at_square(position, to, BLACK), moves, QUEEN, position);
				add_capture_move(from, to, WHITE, PAWN, piece_at_square(position, to, BLACK), moves, ROOK, position);
				add_capture_move(from, to, WHITE, PAWN, piece_at_square(position, to, BLACK), moves, BISHOP, position);
				add_capture_move(from, to, WHITE, PAWN, piece_at_square(position, to, BLACK), moves, KNIGHT, position);
			}
			capture_squares_w = reset_lsb(capture_squares_w);
		}
		uint64_t capture_squares_e = ((position.p[WHITE][PAWN] & ~NE_BORDER) << 9) & (black_squares | (meta_info & ROW_6));
		while (capture_squares_e) {
			int to = lsb_to_square(capture_squares_e);
			int from = to - 9;
			if (to <= 55) {
				add_capture_move(from, to, WHITE, PAWN, piece_at_square(position, to, BLACK), moves, EMPTY, position);
			} else {
				add_capture_move(from, to, WHITE, PAWN, piece_at_square(position, to, BLACK), moves, QUEEN, position);
				add_capture_move(from, to, WHITE, PAWN, piece_at_square(position, to, BLACK), moves, ROOK, position);
				add_capture_move(from, to, WHITE, PAWN, piece_at_square(position, to, BLACK), moves, BISHOP, position);
				add_capture_move(from, to, WHITE, PAWN, piece_at_square(position, to, BLACK), moves, KNIGHT, position);
			}
			capture_squares_e = reset_lsb(capture_squares_e);
		}
	} else {
		// black pawn captures
		uint64_t capture_squares_w = ((position.p[BLACK][PAWN] & ~SW_BORDER) >> 9) & (white_squares | (meta_info & ROW_3));
		while (capture_squares_w) {
			int to = lsb_to_square(capture_squares_w);
			int from = to + 9;
			if (to >= 8) {
				add_capture_move(from, to, BLACK, PAWN, piece_at_square(position, to, WHITE), moves, EMPTY, position);
			} else {
				add_capture_move(from, to, BLACK, PAWN, piece_at_square(position, to, WHITE), moves, QUEEN, position);
				add_capture_move(from, to, BLACK, PAWN, piece_at_square(position, to, WHITE), moves, ROOK, position);
				add_capture_move(from, to, BLACK, PAWN, piece_at_square(position, to, WHITE), moves, BISHOP, position);
				add_capture_move(from, to, BLACK, PAWN, piece_at_square(position, to, WHITE), moves, KNIGHT, position);
			}
			capture_squares_w = reset_lsb(capture_squares_w);
		}
		uint64_t capture_squares_e = ((position.p[BLACK][PAWN] & ~SE_BORDER) >> 7) & (white_squares | (meta_info & ROW_3));
		while (capture_squares_e) {
			int to = lsb_to_square(capture_squares_e);
			int from = to + 7;
			if (to >= 8) {
				add_capture_move(from, to, BLACK, PAWN, piece_at_square(position, to, WHITE), moves, EMPTY, position);
			} else {
				add_capture_move(from, to, BLACK, PAWN, piece_at_square(position, to, WHITE), moves, QUEEN, position);
				add_capture_move(from, to, BLACK, PAWN, piece_at_square(position, to, WHITE), moves, ROOK, position);
				add_capture_move(from, to, BLACK, PAWN, piece_at_square(position, to, WHITE), moves, BISHOP, position);
				add_capture_move(from, to, BLACK, PAWN, piece_at_square(position, to, WHITE), moves, KNIGHT, position);
			}
			capture_squares_e = reset_lsb(capture_squares_e);
		}
	}
	return moves;
}

MoveList get_moves(const Position& position, const bool white_turn) {
	MoveList moves;
	if ((position.p[WHITE][KING] == 0) || (position.p[BLACK][KING]) == 0) {
		return moves;
	}
	uint64_t black_squares = position.p[BLACK][KING] | position.p[BLACK][PAWN] | position.p[BLACK][KNIGHT]
			| position.p[BLACK][BISHOP] | position.p[BLACK][ROOK] | position.p[BLACK][QUEEN];

	uint64_t white_squares = position.p[WHITE][KING] | position.p[WHITE][PAWN] | position.p[WHITE][KNIGHT]
			| position.p[WHITE][BISHOP] | position.p[WHITE][ROOK] | position.p[WHITE][QUEEN];

	uint64_t occupied_squares = black_squares | white_squares;
	uint64_t meta_info = position.meta_info_stack.back();

	int side = white_turn ? WHITE : BLACK;
	int opponent = 1 - side;

	uint64_t side_squares = white_turn ? white_squares : black_squares;
	uint64_t opponent_squares = white_turn ? black_squares : white_squares;

	//knight moves
	uint64_t knights = position.p[side][KNIGHT];
	while (knights) {
		int from = lsb_to_square(knights);
		uint64_t to_squares = knight_moves[from] & ~side_squares;
		while (to_squares) {
			int to = lsb_to_square(to_squares);
			uint64_t lsb = lsb(to_squares);
			if (lsb & opponent_squares) {
				add_capture_move(from, to, side, KNIGHT, piece_at_board(position, lsb, opponent), moves, EMPTY, position);
			} else {
				add_quite_move(from, to, side, KNIGHT, moves, EMPTY);
			}
			to_squares -= lsb;
		}
		knights = reset_lsb(knights);
	}
	uint64_t bishops = position.p[side][BISHOP];
	while (bishops) {
		int from = lsb_to_square(bishops);
		uint64_t to_squares = bishop_attacks(occupied_squares, from) & ~side_squares;
		while (to_squares) {
			int to = lsb_to_square(to_squares);
			uint64_t lsb = lsb(to_squares);
			if (lsb & opponent_squares) {
				add_capture_move(from, to, side, BISHOP, piece_at_board(position, lsb, opponent), moves, EMPTY, position);
			} else {
				add_quite_move(from, to, side, BISHOP, moves, EMPTY);
			}
			to_squares -= lsb;
		}
		bishops = reset_lsb(bishops);
	}
	// rook moves
	uint64_t rooks = position.p[side][ROOK];
	while (rooks) {
		int from = lsb_to_square(rooks);
		uint64_t to_squares = rook_attacks(occupied_squares, from) & ~side_squares;
		while (to_squares) {
			int to = lsb_to_square(to_squares);
			uint64_t lsb = lsb(to_squares);
			if (lsb & opponent_squares) {
				add_capture_move(from, to, side, ROOK, piece_at_board(position, lsb, opponent), moves, EMPTY, position);
			} else {
				add_quite_move(from, to, side, ROOK, moves, EMPTY);
			}
			to_squares -= lsb;
		}
		rooks = reset_lsb(rooks);
	}
	// queen moves
	uint64_t queens = position.p[side][QUEEN];
	while (queens) {
		int from = lsb_to_square(queens);
		uint64_t to_squares = queen_attacks(occupied_squares, from) & ~side_squares;
		while (to_squares) {
			int to = lsb_to_square(to_squares);
			uint64_t lsb = lsb(to_squares);
			if (lsb & opponent_squares) {
				add_capture_move(from, to, side, QUEEN, piece_at_board(position, lsb, opponent), moves, EMPTY, position);
			} else {
				add_quite_move(from, to, side, QUEEN, moves, EMPTY);
			}
			to_squares -= lsb;
		}
		queens = reset_lsb(queens);
	}

	if (white_turn) {
		// white pawn push
		uint64_t to_squares = (position.p[WHITE][PAWN] & ~ROW_8) << 8;
		to_squares = to_squares & ~occupied_squares;
		uint64_t two_step_squares = (to_squares & ROW_3) << 8 & ~occupied_squares;
		while (to_squares) {
			int to = lsb_to_square(to_squares);
			int from = to - 8;
			if (to <= 55) {
				add_quite_move(from, to, WHITE, PAWN, moves, EMPTY);
			} else {
				add_quite_move(from, to, WHITE, PAWN, moves, QUEEN);
				add_quite_move(from, to, WHITE, PAWN, moves, ROOK);
				add_quite_move(from, to, WHITE, PAWN, moves, BISHOP);
				add_quite_move(from, to, WHITE, PAWN, moves, KNIGHT);
			}
			to_squares = reset_lsb(to_squares);
		}
		while (two_step_squares) {
			int to = lsb_to_square(two_step_squares);
			int from = to - 16;
			add_quite_move(from, to, WHITE, PAWN, moves, EMPTY);
			two_step_squares = reset_lsb(two_step_squares);
		}
		// white pawn captures
		uint64_t capture_squares_w = ((position.p[WHITE][PAWN] & ~NW_BORDER) << 7) & (black_squares | (meta_info & ROW_6));
		while (capture_squares_w) {
			int to = lsb_to_square(capture_squares_w);
			int from = to - 7;
			if (to <= 55) {
				add_capture_move(from, to, WHITE, PAWN, piece_at_square(position, to, BLACK), moves, EMPTY, position);
			} else {
				add_capture_move(from, to, WHITE, PAWN, piece_at_square(position, to, BLACK), moves, QUEEN, position);
				add_capture_move(from, to, WHITE, PAWN, piece_at_square(position, to, BLACK), moves, ROOK, position);
				add_capture_move(from, to, WHITE, PAWN, piece_at_square(position, to, BLACK), moves, BISHOP, position);
				add_capture_move(from, to, WHITE, PAWN, piece_at_square(position, to, BLACK), moves, KNIGHT, position);
			}
			capture_squares_w = reset_lsb(capture_squares_w);
		}
		uint64_t capture_squares_e = ((position.p[WHITE][PAWN] & ~NE_BORDER) << 9) & (black_squares | (meta_info & ROW_6));
		while (capture_squares_e) {
			int to = lsb_to_square(capture_squares_e);
			int from = to - 9;
			if (to <= 55) {
				add_capture_move(from, to, WHITE, PAWN, piece_at_square(position, to, BLACK), moves, EMPTY, position);
			} else {
				add_capture_move(from, to, WHITE, PAWN, piece_at_square(position, to, BLACK), moves, QUEEN, position);
				add_capture_move(from, to, WHITE, PAWN, piece_at_square(position, to, BLACK), moves, ROOK, position);
				add_capture_move(from, to, WHITE, PAWN, piece_at_square(position, to, BLACK), moves, BISHOP, position);
				add_capture_move(from, to, WHITE, PAWN, piece_at_square(position, to, BLACK), moves, KNIGHT, position);
			}
			capture_squares_e = reset_lsb(capture_squares_e);
		}
		// white king moves
		uint64_t king = position.p[WHITE][KING];
		int from = lsb_to_square(king);
		to_squares = king_moves[from] & ~white_squares;
		while (to_squares) {
			int to = lsb_to_square(to_squares);
			uint64_t lsb = lsb(to_squares);
			if (lsb & black_squares) {
				add_capture_move(from, to, WHITE, KING, piece_at_board(position, lsb, BLACK), moves, EMPTY, position);
			} else {
				add_quite_move(from, to, WHITE, KING, moves, EMPTY);
			}
			to_squares -= lsb;
		}
		// castling
		if ((meta_info & G1) && !(occupied_squares & white_king_side_castle_squares) && (position.p[WHITE][ROOK] & H1)) {
			add_castle_move(from, lsb_to_square(G1), WHITE, moves);
		}
		if ((meta_info & C1) && !(occupied_squares & white_queen_side_castle_squares) && (position.p[WHITE][ROOK] & A1)) {
			add_castle_move(from, lsb_to_square(C1), WHITE, moves);
		}
	} else {
		// black pawn push
		uint64_t to_squares = (position.p[BLACK][PAWN] & ~ROW_1) >> 8;
		to_squares = to_squares & ~occupied_squares;
		uint64_t two_step_squares = ((to_squares & ROW_6) >> 8) & ~occupied_squares;
		while (to_squares) {
			int to = lsb_to_square(to_squares);
			int from = to + 8;
			if (to >= 8) {
				add_quite_move(from, to, BLACK, PAWN, moves, EMPTY);
			} else {
				add_quite_move(from, to, BLACK, PAWN, moves, QUEEN);
				add_quite_move(from, to, BLACK, PAWN, moves, ROOK);
				add_quite_move(from, to, BLACK, PAWN, moves, BISHOP);
				add_quite_move(from, to, BLACK, PAWN, moves, KNIGHT);
			}
			to_squares = reset_lsb(to_squares);
		}
		while (two_step_squares) {
			int to = lsb_to_square(two_step_squares);
			int from = to + 16;
			add_quite_move(from, to, BLACK, PAWN, moves, EMPTY);
			two_step_squares = reset_lsb(two_step_squares);
		}
		// black pawn captures
		uint64_t capture_squares_w = ((position.p[BLACK][PAWN] & ~SW_BORDER) >> 9) & (white_squares | (meta_info & ROW_3));
		while (capture_squares_w) {
			int to = lsb_to_square(capture_squares_w);
			int from = to + 9;
			if (to >= 8) {
				add_capture_move(from, to, BLACK, PAWN, piece_at_square(position, to, WHITE), moves, EMPTY, position);
			} else {
				add_capture_move(from, to, BLACK, PAWN, piece_at_square(position, to, WHITE), moves, QUEEN, position);
				add_capture_move(from, to, BLACK, PAWN, piece_at_square(position, to, WHITE), moves, ROOK, position);
				add_capture_move(from, to, BLACK, PAWN, piece_at_square(position, to, WHITE), moves, BISHOP, position);
				add_capture_move(from, to, BLACK, PAWN, piece_at_square(position, to, WHITE), moves, KNIGHT, position);
			}
			capture_squares_w = reset_lsb(capture_squares_w);
		}
		uint64_t capture_squares_e = ((position.p[BLACK][PAWN] & ~SE_BORDER) >> 7) & (white_squares | (meta_info & ROW_3));
		while (capture_squares_e) {
			int to = lsb_to_square(capture_squares_e);
			int from = to + 7;
			if (to >= 8) {
				add_capture_move(from, to, BLACK, PAWN, piece_at_square(position, to, WHITE), moves, EMPTY, position);
			} else {
				add_capture_move(from, to, BLACK, PAWN, piece_at_square(position, to, WHITE), moves, QUEEN, position);
				add_capture_move(from, to, BLACK, PAWN, piece_at_square(position, to, WHITE), moves, ROOK, position);
				add_capture_move(from, to, BLACK, PAWN, piece_at_square(position, to, WHITE), moves, BISHOP, position);
				add_capture_move(from, to, BLACK, PAWN, piece_at_square(position, to, WHITE), moves, KNIGHT, position);
			}
			capture_squares_e = reset_lsb(capture_squares_e);
		}
		// black king moves
		uint64_t king = position.p[BLACK][KING];
		int from = lsb_to_square(king);
		to_squares = king_moves[from] & ~black_squares;
		while (to_squares) {
			uint64_t lsb = lsb(to_squares);
			int to = lsb_to_square(to_squares);
			if (lsb & white_squares) {
				add_capture_move(from, to, BLACK, KING, piece_at_board(position, lsb, WHITE), moves, EMPTY, position);
			} else {
				add_quite_move(from, to, BLACK, KING, moves, EMPTY);
			}
			to_squares -= lsb;
		}
		// castling
		if ((meta_info & G8) && !(occupied_squares & black_king_side_castle_squares) && (position.p[BLACK][ROOK] & H8)) {
			add_castle_move(from, lsb_to_square(G8), BLACK, moves);
		}
		if ((meta_info & C8) && !(occupied_squares & black_queen_side_castle_squares) && (position.p[BLACK][ROOK] & A8)) {
			add_castle_move(from, lsb_to_square(C8), BLACK, moves);
		}
	}

	return moves;
}

void init() {

	rook_castle_to_squares[2] = 3;
	rook_castle_to_squares[6] = 5;
	rook_castle_to_squares[62] = 61;
	rook_castle_to_squares[58] = 59;
	rook_castle_from_squares[2] = 0;
	rook_castle_from_squares[6] = 7;
	rook_castle_from_squares[62] = 63;
	rook_castle_from_squares[58] = 56;

	for (int i = 0; i < 64; i++) {
		uint64_t b = 1ULL << i;
		uint64_t to_squares = ((b & ~NNW_BORDER) << 15) | ((b & ~NNE_BORDER) << 17) | ((b & ~EEN_BORDER) << 10)
				| ((b & ~WWN_BORDER) << 6) | ((b & ~EES_BORDER) >> 6) | ((b & ~SSE_BORDER) >> 15)
				| ((b & ~SSW_BORDER) >> 17) | ((b & ~WWS_BORDER) >> 10);

		knight_moves[i] = to_squares;
	}
	for (int i = 0; i < 64; i++) {
		uint64_t b = 1ULL << i;
		uint64_t to_squares = ((b & ~NW_BORDER) << 7) | ((b & ~NE_BORDER) << 9) | ((b & ~ROW_8) << 8)
				| ((b & ~H_FILE) << 1) | ((b & ~A_FILE) >> 1) | ((b & ~ROW_1) >> 8) | ((b & ~SW_BORDER) >> 9)
				| ((b & ~SE_BORDER) >> 7);

		king_moves[i] = to_squares;
	}

	init_magic_lookup_table();
}

bool is_illegal_castling_move(const Move& move, const uint64_t attacked_squares_by_opponent) {
	uint64_t to_square = 1ULL << to_square(move.m);
	if (to_square == C1) {
		return (C1 | D1 | E1) & attacked_squares_by_opponent;
	}
	if (to_square == G1) {
		return (E1 | F1 | G1) & attacked_squares_by_opponent;
	}
	if (to_square == C8) {
		return (C8 | D8 | E8) & attacked_squares_by_opponent;
	}
	if (to_square == G8) {
		return (E8 | F8 | G8) & attacked_squares_by_opponent;
	}
	return false;
}
