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

int rook_castle_to_squares[64];
int rook_castle_from_squares[64];

// pre-calculated move masks
const int N = 0;
const int NE = 1;
const int E = 2;
const int SE = 3;
const int S = 4;
const int SW = 5;
const int W = 6;
const int NW = 7;

uint64_t knight_moves[64];
uint64_t ray_moves[8][64];
uint64_t king_moves[64];

// [V][A] Most valuable victim gives the highest scores
const int MVVLVA[7][6] = // 7th captured piece type is EN_PASSANT
{
		{6,5,4,3,2,1},
		{16,15,14,13,12,11},
		{26,25,24,23,22,21},
		{36,35,34,33,32,31},
		{46,45,44,43,42,41},
		{56,55,54,53,52,51},
		{6,6,6,6,6,6},
};


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

/*
 * returns mask with all files filled that are occupied with at least one bit of input
 */
uint64_t file_fill(uint64_t l) {
	return south_fill(l) | north_fill(l);
}


/*
 * castling is poorly implemented (perft(6) from 4 to 4.5 seconds).
 *
 * meta-info-stack:100-200 ms extra for perft(6)
 */

void make_move(Board& board, Move move) {
	board.b[color(move.m)][piece(move.m)] &= ~(1UL << from_square(move.m));
	board.b[color(move.m)][piece(move.m)] |= (1UL << to_square(move.m));
	uint64_t meta_info = board.meta_info_stack.back();
	int captured_piece = captured_piece(move.m);
	if (captured_piece != EMPTY) {
		int captured_color = color(move.m) ^ 1;
		if (captured_piece != EN_PASSANT) {
			board.b[captured_color][captured_piece] &= ~(1UL << to_square(move.m));
		} else {
			board.b[captured_color][PAWN] &= ~(1UL << (to_square(move.m) - 8 + (color(move.m)*16)));
		}
	}
	int promotion_piece = promotion_piece(move.m);
	if (promotion_piece != EMPTY) {
		board.b[color(move.m)][piece(move.m)] &= ~(1UL << to_square(move.m));
		board.b[color(move.m)][promotion_piece] |= (1UL << to_square(move.m));
	}
	if (is_castling(move.m)) {
		board.b[color(move.m)][ROOK] &= ~(1UL << rook_castle_from_squares[to_square(move.m)]);
		board.b[color(move.m)][ROOK] |= (1UL << rook_castle_to_squares[to_square(move.m)]);
	}
	if (piece(move.m) == KING){
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
			meta_info |= (1UL << from_square(move.m)) << 8;
		} else {
			meta_info |= (1UL << from_square(move.m)) >> 8;
		}
	}
	board.meta_info_stack.push_back(meta_info);
	board.hash_key = board.hash_key ^ move.m;
}

void unmake_move(Board& board, Move move) {
	board.b[color(move.m)][piece(move.m)] |= (1UL << from_square(move.m));
	board.b[color(move.m)][piece(move.m)] &= ~(1UL << to_square(move.m));
	int captured_piece = captured_piece(move.m);
	if (captured_piece != EMPTY) {
		int captured_color = color(move.m) ^ 1;
		if (captured_piece != EN_PASSANT) {
			board.b[captured_color][captured_piece] |= (1UL << to_square(move.m));
		} else {
			board.b[captured_color][PAWN] |= (1UL << (to_square(move.m) - 8 + (color(move.m)*16)));
		}
	}
	int promotion_piece = promotion_piece(move.m);
	if (promotion_piece != EMPTY) {
		board.b[color(move.m)][promotion_piece] &= ~(1UL << to_square(move.m));
	}
	if (is_castling(move.m)) {
		board.b[color(move.m)][ROOK] |= (1UL << rook_castle_from_squares[to_square(move.m)]);
		board.b[color(move.m)][ROOK] &= ~(1UL << rook_castle_to_squares[to_square(move.m)]);
	}
	// reverse meta-info by poping the last element from the stack
	board.meta_info_stack.pop_back();
	board.hash_key = board.hash_key ^ move.m;
}


inline void add_quite_move(const int& from, const int& to, const int& color, const int& piece, MoveList& moves, const int promotion) {
	Move move;
	move.m = to_move(from,to, piece, color, promotion);
	move.sort_score = 0;
	moves.push_back(move);
}

inline void add_capture_move(const int& from, const int& to, const int& color, const int& piece, int captured_piece, MoveList& moves, const int promotion) {
	Move move;
	move.m = to_capture_move(from,to, piece, captured_piece, color, promotion);
	move.sort_score = MVVLVA[captured_piece][piece] + 1000000;
	moves.push_front(move);
}

inline void add_castle_move(int from, int to, int color, MoveList& moves) {
	Move move;
	move.m = to_castle_move(from, to, color);
	move.sort_score = 1;
	moves.push_front(move);
}

/**
 * Ray for direction N E NE NW
 */
inline uint64_t get_positive_ray_moves(const int& dir, const int& from, const uint64_t& occupied_squares) {
	uint64_t attacked_squares = ray_moves[dir][from];
	uint64_t blocker = attacked_squares & occupied_squares;
	attacked_squares ^= ray_moves[dir][lsb_to_square(blocker | H8)];
	return attacked_squares;
}

/**
 * Ray for direction S W SW SE
 */
inline uint64_t get_negative_ray_moves(const int& dir, const int& from, const uint64_t& occupied_squares) {
	uint64_t attacked_squares = ray_moves[dir][from];
	uint64_t blocker = attacked_squares & occupied_squares;
	attacked_squares ^= ray_moves[dir][msb_to_square(blocker | A1)];
	return attacked_squares;
}

MoveList get_captures(const Board& board, const bool white_turn) {
	MoveList children;
	if ((board.b[WHITE][KING] == 0) || (board.b[BLACK][KING]) == 0) {
		return children;
	}
	uint64_t black_squares = board.b[BLACK][KING] |
							board.b[BLACK][PAWN] |
							board.b[BLACK][KNIGHT] |
							board.b[BLACK][BISHOP] |
							board.b[BLACK][ROOK] |
							board.b[BLACK][QUEEN];

	uint64_t white_squares = board.b[WHITE][KING] |
							board.b[WHITE][PAWN] |
							board.b[WHITE][KNIGHT] |
							board.b[WHITE][BISHOP] |
							board.b[WHITE][ROOK] |
							board.b[WHITE][QUEEN];

	uint64_t occupied_squares = black_squares | white_squares;
	uint64_t meta_info = board.meta_info_stack.back();
	if (white_turn) {
		// white pawn captures
		uint64_t capture_squares_w = ((board.b[WHITE][PAWN] & ~NW_BORDER) << 7) & (black_squares | (meta_info & ROW_6));
		while(capture_squares_w) {
			int to = lsb_to_square(capture_squares_w);
			int from = to - 7;
			int promotion = to > 55 ? QUEEN : EMPTY;
			add_capture_move(from, to, WHITE, PAWN, piece_at_square(board,to, BLACK), children, promotion);
			capture_squares_w = reset_lsb(capture_squares_w);
		}
		uint64_t capture_squares_e = ((board.b[WHITE][PAWN] & ~NE_BORDER) << 9) & (black_squares | (meta_info & ROW_6));
		while(capture_squares_e) {
			int to = lsb_to_square(capture_squares_e);
			int from = to - 9;
			int promotion = to > 55 ? QUEEN : EMPTY;
			add_capture_move(from, to, WHITE, PAWN, piece_at_square(board,to, BLACK), children, promotion);
			capture_squares_e = reset_lsb(capture_squares_e);
		}
		// white knight moves
		uint64_t knights = board.b[WHITE][KNIGHT];
		while(knights) {
			int from = lsb_to_square(knights);
			uint64_t to_squares = knight_moves[from] & black_squares;
			while(to_squares) {
				int to = lsb_to_square(to_squares);
				uint64_t lsb = lsb(to_squares);
				add_capture_move(from,to, WHITE, KNIGHT, piece_at_board(board, lsb, BLACK), children, EMPTY);
				to_squares -= lsb;
			}
			knights = reset_lsb(knights);
		}
		// white bishop moves
		uint64_t bishops = board.b[WHITE][BISHOP];
		while(bishops) {
			int from = lsb_to_square(bishops);
			uint64_t to_squares =(
					get_positive_ray_moves(NW, from, occupied_squares) |
					get_positive_ray_moves(NE, from, occupied_squares) |
					get_negative_ray_moves(SW, from, occupied_squares) |
					get_negative_ray_moves(SE, from, occupied_squares)) & black_squares;
			while(to_squares) {
				int to = lsb_to_square(to_squares);
				uint64_t lsb = lsb(to_squares);
				add_capture_move(from,to, WHITE, BISHOP, piece_at_board(board, lsb, BLACK), children, EMPTY);
				to_squares -= lsb;
			}
			bishops = reset_lsb(bishops);
		}
		// white rook moves
		uint64_t rooks = board.b[WHITE][ROOK];
		while(rooks) {
			int from = lsb_to_square(rooks);
			uint64_t to_squares =(
					get_positive_ray_moves(N, from, occupied_squares) |
					get_positive_ray_moves(E, from, occupied_squares) |
					get_negative_ray_moves(W, from, occupied_squares) |
					get_negative_ray_moves(S, from, occupied_squares)) & black_squares;
			while(to_squares) {
				int to = lsb_to_square(to_squares);
				uint64_t lsb = lsb(to_squares);
				add_capture_move(from,to, WHITE, ROOK, piece_at_board(board, lsb, BLACK), children, EMPTY);
				to_squares -= lsb;
			}
			rooks = reset_lsb(rooks);
		}
		// white queen moves
		uint64_t queens = board.b[WHITE][QUEEN];
		while(queens) {
			int from = lsb_to_square(queens);
			uint64_t to_squares =(
					get_positive_ray_moves(N, from, occupied_squares) |
					get_positive_ray_moves(E, from, occupied_squares) |
					get_positive_ray_moves(NW, from, occupied_squares) |
					get_positive_ray_moves(NE, from, occupied_squares) |
					get_negative_ray_moves(W, from, occupied_squares) |
					get_negative_ray_moves(S, from, occupied_squares) |
					get_negative_ray_moves(SW, from, occupied_squares) |
					get_negative_ray_moves(SE, from, occupied_squares)) & black_squares;
			while(to_squares) {
				int to = lsb_to_square(to_squares);
				uint64_t lsb = lsb(to_squares);
				add_capture_move(from,to, WHITE, QUEEN, piece_at_board(board, lsb, BLACK), children, EMPTY);
				to_squares -= lsb;
			}
			queens = reset_lsb(queens);
		}
		// white king moves
		uint64_t king = board.b[WHITE][KING];
		int from = lsb_to_square(king);
		uint64_t to_squares = king_moves[from] & black_squares;
		while(to_squares) {
			int to = lsb_to_square(to_squares);
			uint64_t lsb = lsb(to_squares);
			add_capture_move(from,to, WHITE, KING, piece_at_board(board, lsb, BLACK), children, EMPTY);
			to_squares -= lsb;
		}
	} else {
		// black pawn captures
		uint64_t capture_squares_w = ((board.b[BLACK][PAWN] & ~SW_BORDER) >> 9) & (white_squares | (meta_info & ROW_3));
		while(capture_squares_w) {
			int to = lsb_to_square(capture_squares_w);
			int from = to + 9;
			int promotion = to < 8 ? QUEEN : EMPTY;
			add_capture_move(from, to, BLACK, PAWN, piece_at_square(board,to, WHITE), children, promotion);
			capture_squares_w = reset_lsb(capture_squares_w);
		}
		uint64_t capture_squares_e = ((board.b[BLACK][PAWN] & ~SE_BORDER) >> 7) & (white_squares | (meta_info & ROW_3));
		while(capture_squares_e) {
			int to = lsb_to_square(capture_squares_e);
			int from = to + 7;
			int promotion = to < 8 ? QUEEN : EMPTY;
			add_capture_move(from, to, BLACK, PAWN, piece_at_square(board,to, WHITE), children, promotion);
			capture_squares_e = reset_lsb(capture_squares_e);
		}
		// black knight moves
		uint64_t knights = board.b[BLACK][KNIGHT];
		while(knights) {
			int from = lsb_to_square(knights);
			uint64_t to_squares = knight_moves[from] & white_squares;
			while(to_squares) {
				int to = lsb_to_square(to_squares);
				uint64_t lsb = lsb(to_squares);
				add_capture_move(from,to, BLACK, KNIGHT, piece_at_board(board, lsb, WHITE), children, EMPTY);
				to_squares -= lsb;
			}
			knights = reset_lsb(knights);
		}
		// black bishop moves
		uint64_t bishops = board.b[BLACK][BISHOP];
		while(bishops) {
			int from = lsb_to_square(bishops);
			uint64_t to_squares =(
					get_positive_ray_moves(NW, from, occupied_squares) |
					get_positive_ray_moves(NE, from, occupied_squares) |
					get_negative_ray_moves(SW, from, occupied_squares) |
					get_negative_ray_moves(SE, from, occupied_squares)) & white_squares;
			while(to_squares) {
				int to = lsb_to_square(to_squares);
				uint64_t lsb = lsb(to_squares);
				add_capture_move(from,to, BLACK, BISHOP, piece_at_board(board, lsb, WHITE), children, EMPTY);
				to_squares -= lsb;
			}
			bishops = reset_lsb(bishops);
		}
		// black rook moves
		uint64_t rooks = board.b[BLACK][ROOK];
		while(rooks) {
			int from = lsb_to_square(rooks);
			uint64_t to_squares = (
					get_positive_ray_moves(N, from, occupied_squares) |
					get_positive_ray_moves(E, from, occupied_squares) |
					get_negative_ray_moves(S, from, occupied_squares) |
					get_negative_ray_moves(W, from, occupied_squares) ) & white_squares;
			while(to_squares) {
				int to = lsb_to_square(to_squares);
				uint64_t lsb = lsb(to_squares);
				add_capture_move(from,to, BLACK, ROOK, piece_at_board(board, lsb, WHITE), children, EMPTY);
				to_squares -= lsb;
			}
			rooks = reset_lsb(rooks);
		}
		// black queen moves
		uint64_t queens = board.b[BLACK][QUEEN];
		while(queens) {
			int from = lsb_to_square(queens);
			uint64_t to_squares = (
					get_positive_ray_moves(N, from, occupied_squares) |
					get_positive_ray_moves(E, from, occupied_squares) |
					get_positive_ray_moves(NE, from, occupied_squares) |
					get_positive_ray_moves(NW, from, occupied_squares) |
					get_negative_ray_moves(S, from, occupied_squares) |
					get_negative_ray_moves(W, from, occupied_squares) |
					get_negative_ray_moves(SW, from, occupied_squares) |
					get_negative_ray_moves(SE, from, occupied_squares) ) & white_squares;
			while(to_squares) {
				int to = lsb_to_square(to_squares);
				uint64_t lsb = lsb(to_squares);
				add_capture_move(from,to, BLACK, QUEEN, piece_at_board(board, lsb, WHITE), children, EMPTY);
				to_squares -= lsb;
			}
			queens = reset_lsb(queens);
		}
		// black king moves
		uint64_t king = board.b[BLACK][KING];
		int from = lsb_to_square(king);
		uint64_t to_squares = king_moves[from] & white_squares;
		while(to_squares) {
			uint64_t lsb = lsb(to_squares);
			int to = lsb_to_square(to_squares);
			add_capture_move(from,to, BLACK, KING, piece_at_board(board, lsb, WHITE), children, EMPTY);
			to_squares -= lsb;
		}
	}
	return children;
}

MoveList get_children(const Board& board, const bool white_turn) {
	MoveList children;
	if ((board.b[WHITE][KING] == 0) || (board.b[BLACK][KING]) == 0) {
		return children;
	}
	uint64_t black_squares = board.b[BLACK][KING] |
							board.b[BLACK][PAWN] |
							board.b[BLACK][KNIGHT] |
							board.b[BLACK][BISHOP] |
							board.b[BLACK][ROOK] |
							board.b[BLACK][QUEEN];

	uint64_t white_squares = board.b[WHITE][KING] |
							board.b[WHITE][PAWN] |
							board.b[WHITE][KNIGHT] |
							board.b[WHITE][BISHOP] |
							board.b[WHITE][ROOK] |
							board.b[WHITE][QUEEN];

	uint64_t occupied_squares = black_squares | white_squares;
	uint64_t meta_info = board.meta_info_stack.back();
	if (white_turn) {
		// white pawn push
		uint64_t to_squares = (board.b[WHITE][PAWN] & ~ROW_8) << 8;
		to_squares = to_squares & ~occupied_squares;
		uint64_t two_step_squares = (to_squares & ROW_3) << 8 & ~occupied_squares;
		while (to_squares) {
			int to = lsb_to_square(to_squares);
			int from = to - 8;
			int promotion = to > 55 ? QUEEN : EMPTY;
			add_quite_move(from, to, WHITE, PAWN, children, promotion);
			to_squares = reset_lsb(to_squares);
		}
		while (two_step_squares) {
			int to = lsb_to_square(two_step_squares);
			int from = to - 16;
			add_quite_move(from, to, WHITE, PAWN, children, EMPTY);
			two_step_squares = reset_lsb(two_step_squares);
		}
		// white pawn captures
		uint64_t capture_squares_w = ((board.b[WHITE][PAWN] & ~NW_BORDER) << 7) & (black_squares | (meta_info & ROW_6));
		while(capture_squares_w) {
			int to = lsb_to_square(capture_squares_w);
			int from = to - 7;
			int promotion = to > 55 ? QUEEN : EMPTY;
			add_capture_move(from, to, WHITE, PAWN, piece_at_square(board,to, BLACK), children, promotion);
			capture_squares_w = reset_lsb(capture_squares_w);
		}
		uint64_t capture_squares_e = ((board.b[WHITE][PAWN] & ~NE_BORDER) << 9) & (black_squares | (meta_info & ROW_6));
		while(capture_squares_e) {
			int to = lsb_to_square(capture_squares_e);
			int from = to - 9;
			int promotion = to > 55 ? QUEEN : EMPTY;
			add_capture_move(from, to, WHITE, PAWN, piece_at_square(board,to, BLACK), children, promotion);
			capture_squares_e = reset_lsb(capture_squares_e);
		}
		// white knight moves
		uint64_t knights = board.b[WHITE][KNIGHT];
		while(knights) {
			int from = lsb_to_square(knights);
			uint64_t to_squares = knight_moves[from] & ~white_squares;
			while(to_squares) {
				int to = lsb_to_square(to_squares);
				uint64_t lsb = lsb(to_squares);
				if (lsb & black_squares) {
					add_capture_move(from,to, WHITE, KNIGHT, piece_at_board(board, lsb, BLACK), children, EMPTY);
				} else {
					add_quite_move(from, to, WHITE, KNIGHT, children, EMPTY);
				}
				to_squares -= lsb;
			}
			knights = reset_lsb(knights);
		}
		// white bishop moves
		uint64_t bishops = board.b[WHITE][BISHOP];
		while(bishops) {
			int from = lsb_to_square(bishops);
			uint64_t to_squares =
					get_positive_ray_moves(NW, from, occupied_squares) |
					get_positive_ray_moves(NE, from, occupied_squares) |
					get_negative_ray_moves(SW, from, occupied_squares) |
					get_negative_ray_moves(SE, from, occupied_squares);
			while(to_squares) {
				int to = lsb_to_square(to_squares);
				uint64_t lsb = lsb(to_squares);
				if (lsb & black_squares) {
					add_capture_move(from,to, WHITE, BISHOP, piece_at_board(board, lsb, BLACK), children, EMPTY);
				} else if (lsb & ~occupied_squares){
					add_quite_move(from, to, WHITE, BISHOP, children, EMPTY);
				}
				to_squares -= lsb;
			}
			bishops = reset_lsb(bishops);
		}
		// white rook moves
		uint64_t rooks = board.b[WHITE][ROOK];
		while(rooks) {
			int from = lsb_to_square(rooks);
			uint64_t to_squares =
					get_positive_ray_moves(N, from, occupied_squares) |
					get_positive_ray_moves(E, from, occupied_squares) |
					get_negative_ray_moves(W, from, occupied_squares) |
					get_negative_ray_moves(S, from, occupied_squares);
			while(to_squares) {
				int to = lsb_to_square(to_squares);
				uint64_t lsb = lsb(to_squares);
				if (lsb & black_squares) {
					add_capture_move(from,to, WHITE, ROOK, piece_at_board(board, lsb, BLACK), children, EMPTY);
				} else if (lsb & ~occupied_squares){
					add_quite_move(from, to, WHITE, ROOK, children, EMPTY);
				}
				to_squares -= lsb;
			}
			rooks = reset_lsb(rooks);
		}
		// white queen moves
		uint64_t queens = board.b[WHITE][QUEEN];
		while(queens) {
			int from = lsb_to_square(queens);
			uint64_t to_squares =
					get_positive_ray_moves(N, from, occupied_squares) |
					get_positive_ray_moves(E, from, occupied_squares) |
					get_positive_ray_moves(NW, from, occupied_squares) |
					get_positive_ray_moves(NE, from, occupied_squares) |
					get_negative_ray_moves(W, from, occupied_squares) |
					get_negative_ray_moves(S, from, occupied_squares) |
					get_negative_ray_moves(SW, from, occupied_squares) |
					get_negative_ray_moves(SE, from, occupied_squares);
			while(to_squares) {
				int to = lsb_to_square(to_squares);
				uint64_t lsb = lsb(to_squares);
				if (lsb & black_squares) {
					add_capture_move(from,to, WHITE, QUEEN, piece_at_board(board, lsb, BLACK), children, EMPTY);
				} else if (lsb & ~occupied_squares){
					add_quite_move(from, to, WHITE, QUEEN, children, EMPTY);
				}
				to_squares -= lsb;
			}
			queens = reset_lsb(queens);
		}
		// white king moves
		uint64_t king = board.b[WHITE][KING];
		int from = lsb_to_square(king);
		to_squares = king_moves[from] & ~white_squares;
		while(to_squares) {
			int to = lsb_to_square(to_squares);
			uint64_t lsb = lsb(to_squares);
			if (lsb & black_squares) {
				add_capture_move(from,to, WHITE, KING, piece_at_board(board, lsb, BLACK), children, EMPTY);
			} else {
				add_quite_move(from, to, WHITE, KING, children, EMPTY);
			}
			to_squares -= lsb;
		}
		// castling
		if ((meta_info & G1) && !(occupied_squares & white_king_side_castle_squares)) {
			add_castle_move(from, lsb_to_square(G1), WHITE, children);
		}
		if ((meta_info & C1) && !(occupied_squares & white_queen_side_castle_squares)) {
			add_castle_move(from, lsb_to_square(C1), WHITE, children);
		}
	} else {
		// black pawn push
		uint64_t to_squares = (board.b[BLACK][PAWN] & ~ROW_1) >> 8;
		to_squares = to_squares & ~occupied_squares;
		uint64_t two_step_squares = ((to_squares & ROW_6) >> 8) & ~occupied_squares;
		while (to_squares) {
			int to = lsb_to_square(to_squares);
			int from = to + 8;
			int promotion = to < 8 ? QUEEN : EMPTY;
			add_quite_move(from, to, BLACK, PAWN, children, promotion);
			to_squares = reset_lsb(to_squares);
		}
		while (two_step_squares) {
			int to = lsb_to_square(two_step_squares);
			int from = to + 16;
			add_quite_move(from, to, BLACK, PAWN, children, EMPTY);
			two_step_squares = reset_lsb(two_step_squares);
		}
		// black pawn captures
		uint64_t capture_squares_w = ((board.b[BLACK][PAWN] & ~SW_BORDER) >> 9) & (white_squares | (meta_info & ROW_3));
		while(capture_squares_w) {
			int to = lsb_to_square(capture_squares_w);
			int from = to + 9;
			int promotion = to < 8 ? QUEEN : EMPTY;
			add_capture_move(from, to, BLACK, PAWN, piece_at_square(board,to, WHITE), children, promotion);
			capture_squares_w = reset_lsb(capture_squares_w);
		}
		uint64_t capture_squares_e = ((board.b[BLACK][PAWN] & ~SE_BORDER) >> 7) & (white_squares | (meta_info & ROW_3));
		while(capture_squares_e) {
			int to = lsb_to_square(capture_squares_e);
			int from = to + 7;
			int promotion = to < 8 ? QUEEN : EMPTY;
			add_capture_move(from, to, BLACK, PAWN, piece_at_square(board,to, WHITE), children, promotion);
			capture_squares_e = reset_lsb(capture_squares_e);
		}
		// black knight moves
		uint64_t knights = board.b[BLACK][KNIGHT];
		while(knights) {
			int from = lsb_to_square(knights);
			uint64_t to_squares = knight_moves[from] & ~black_squares;
			while(to_squares) {
				int to = lsb_to_square(to_squares);
				uint64_t lsb = lsb(to_squares);
				if (lsb & white_squares) {
					add_capture_move(from,to, BLACK, KNIGHT, piece_at_board(board, lsb, WHITE), children, EMPTY);
				} else {
					add_quite_move(from, to, BLACK, KNIGHT, children, EMPTY);
				}
				to_squares -= lsb;
			}
			knights = reset_lsb(knights);
		}
		// black bishop moves
		uint64_t bishops = board.b[BLACK][BISHOP];
		while(bishops) {
			int from = lsb_to_square(bishops);
			uint64_t to_squares =
					get_positive_ray_moves(NW, from, occupied_squares) |
					get_positive_ray_moves(NE, from, occupied_squares) |
					get_negative_ray_moves(SW, from, occupied_squares) |
					get_negative_ray_moves(SE, from, occupied_squares);
			while(to_squares) {
				int to = lsb_to_square(to_squares);
				uint64_t lsb = lsb(to_squares);
				if (lsb & white_squares) {
					add_capture_move(from,to, BLACK, BISHOP, piece_at_board(board, lsb, WHITE), children, EMPTY);
				} else if (lsb & ~occupied_squares){
					add_quite_move(from, to, BLACK, BISHOP, children, EMPTY);
				}
				to_squares -= lsb;
			}
			bishops = reset_lsb(bishops);
		}
		// black rook moves
		uint64_t rooks = board.b[BLACK][ROOK];
		while(rooks) {
			int from = lsb_to_square(rooks);
			uint64_t to_squares =
					get_positive_ray_moves(N, from, occupied_squares) |
					get_positive_ray_moves(E, from, occupied_squares) |
					get_negative_ray_moves(S, from, occupied_squares) |
					get_negative_ray_moves(W, from, occupied_squares);
			while(to_squares) {
				int to = lsb_to_square(to_squares);
				uint64_t lsb = lsb(to_squares);
				if (lsb & white_squares) {
					add_capture_move(from,to, BLACK, ROOK, piece_at_board(board, lsb, WHITE), children, EMPTY);
				} else if (lsb & ~occupied_squares){
					add_quite_move(from, to, BLACK, ROOK, children, EMPTY);
				}
				to_squares -= lsb;
			}
			rooks = reset_lsb(rooks);
		}
		// black queen moves
		uint64_t queens = board.b[BLACK][QUEEN];
		while(queens) {
			int from = lsb_to_square(queens);
			uint64_t to_squares =
					get_positive_ray_moves(N, from, occupied_squares) |
					get_positive_ray_moves(E, from, occupied_squares) |
					get_positive_ray_moves(NE, from, occupied_squares) |
					get_positive_ray_moves(NW, from, occupied_squares) |
					get_negative_ray_moves(S, from, occupied_squares) |
					get_negative_ray_moves(W, from, occupied_squares) |
					get_negative_ray_moves(SW, from, occupied_squares) |
					get_negative_ray_moves(SE, from, occupied_squares);
			while(to_squares) {
				int to = lsb_to_square(to_squares);
				uint64_t lsb = lsb(to_squares);
				if (lsb & white_squares) {
					add_capture_move(from,to, BLACK, QUEEN, piece_at_board(board, lsb, WHITE), children, EMPTY);
				} else if (lsb & ~occupied_squares){
					add_quite_move(from, to, BLACK, QUEEN, children, EMPTY);
				}
				to_squares -= lsb;
			}
			queens = reset_lsb(queens);
		}
		// black king moves
		uint64_t king = board.b[BLACK][KING];
		int from = lsb_to_square(king);
		to_squares = king_moves[from] & ~black_squares;
		while(to_squares) {
			uint64_t lsb = lsb(to_squares);
			int to = lsb_to_square(to_squares);
			if (lsb & white_squares) {
				add_capture_move(from,to, BLACK, KING, piece_at_board(board, lsb, WHITE), children, EMPTY);
			} else {
				add_quite_move(from, to, BLACK, KING, children, EMPTY);
			}
			to_squares -= lsb;
		}
		// castling
		if ((meta_info & G8) && !(occupied_squares & black_king_side_castle_squares)) {
			add_castle_move(from, lsb_to_square(G8), BLACK, children);
		}
		if ((meta_info & C8) && !(occupied_squares & black_queen_side_castle_squares)) {
			add_castle_move(from, lsb_to_square(C8), BLACK, children);
		}
	}

	return children;
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

	for (int i = 0; i < 64 ; i++) {
		uint64_t b = 1UL << i;
		uint64_t to_squares =
		((b & ~NNW_BORDER) << 15) |
		((b & ~NNE_BORDER) << 17) |
		((b & ~EEN_BORDER) << 10) |
		((b & ~WWN_BORDER) << 6) |
		((b & ~EES_BORDER) >> 6) |
		((b & ~SSE_BORDER) >> 15) |
		((b & ~SSW_BORDER) >> 17) |
		((b & ~WWS_BORDER) >> 10);

		knight_moves[i] = to_squares;
	}
	for (int i = 0; i < 64; i++) {
		uint64_t b = 1UL <<i;
		uint64_t to_squares =
		((b & ~NW_BORDER) << 7) |
		((b & ~NE_BORDER) << 9) |
		((b & ~ROW_8) << 8) |
		((b & ~H_FILE) << 1) |
		((b & ~A_FILE) >> 1) |
		((b & ~ROW_1) >> 8) |
		((b & ~SW_BORDER) >> 9) |
		((b & ~SE_BORDER) >> 7);

		king_moves[i] = to_squares;
	}
	for (int i = 0; i < 64; i++) {
		uint64_t b = 1UL << i;
		uint64_t to_squares = 0;
		while(b & ~ROW_8) {
			b = b << 8UL;
			to_squares |= b;
		}
		ray_moves[N][i] = to_squares;
	}
	for (int i = 0; i < 64; i++) {
		uint64_t b = 1UL << i;
		uint64_t to_squares = 0;
		while(b & ~ROW_1) {
			b = b >> 8UL;
			to_squares |= b;
		}
		ray_moves[S][i] = to_squares;
	}
	for (int i = 0; i < 64; i++) {
		uint64_t b = 1UL << i;
		uint64_t to_squares = 0;
		while(b & ~H_FILE) {
			b = b << 1UL;
			to_squares |= b;
		}
		ray_moves[E][i] = to_squares;
	}
	for (int i = 0; i < 64; i++) {
		uint64_t b = 1UL << i;
		uint64_t to_squares = 0;
		while(b & ~A_FILE) {
			b = b >> 1UL;
			to_squares |= b;
		}
		ray_moves[W][i] = to_squares;
	}
	for (int i = 0; i < 64; i++) {
		uint64_t b = 1UL << i;
		uint64_t to_squares = 0;
		while(b & ~NW_BORDER) {
			b = b << 7UL;
			to_squares |= b;
		}
		ray_moves[NW][i] = to_squares;
	}
	for (int i = 0; i < 64; i++) {
		uint64_t b = 1UL << i;
		uint64_t to_squares = 0;
		while(b & ~SW_BORDER) {
			b = b >> 9UL;
			to_squares |= b;
		}
		ray_moves[SW][i] = to_squares;
	}
	for (int i = 0; i < 64; i++) {
		uint64_t b = 1UL << i;
		uint64_t to_squares = 0;
		while(b & ~NE_BORDER) {
			b = b << 9UL;
			to_squares |= b;
		}
		ray_moves[NE][i] = to_squares;
	}
	for (int i = 0; i < 64; i++) {
		uint64_t b = 1UL << i;
		uint64_t to_squares = 0;
		while(b & ~SE_BORDER) {
			b = b >> 7UL;
			to_squares |= b;
		}
		ray_moves[SE][i] = to_squares;
	}
}
