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
 * board.h
 *
 *  Created on: Jun 16, 2013
 *      Author: Torbjörn Nilsson
 */

#ifndef BOARD_H_
#define BOARD_H_

#include <deque>
#include <inttypes.h>
#include <vector>

struct Board {
	uint64_t b[2][6] = {}; //[WHITE|BLACK][PAWN ... KING]
	std::vector<uint64_t> meta_info_stack;
	int hash_key = 0;
};

// meta_info:
// en passant flags on ROW_3 and ROW_6
// castling rightis on the squares the king castle to, C1, G1, C8 and G8

// 0000 pppp 00Cc CCCC PPPP TTTT TTFF FFFF
struct Move {
	int m; // from: bit 0-5, to bit 6-11, piece bit 12-15, capture_piece bit 16-19, color bit 20, castle bit 21, promotion piece bit 24-27
	int sort_score = 0;
};


/* 2^56 . . . . 2^63
 *
 *
 *
 *
 * 2^8 etc...
 * 1 2 4 8 16 32 64 128
 */

/*
 * NORTH = << 8
 * SOUTH = >> 8
 * WEST = >> 1
 * EAST = << 1
 * NE = << 9
 * NW = << 7
 * SE = >> 7
 * SW = >> 9
 */

static const uint64_t A1 = 1UL << 0;
static const uint64_t B1 = 1UL << 1;
static const uint64_t C1 = 1UL << 2;
static const uint64_t D1 = 1UL << 3;
static const uint64_t E1 = 1UL << 4;
static const uint64_t F1 = 1UL << 5;
static const uint64_t G1 = 1UL << 6;
static const uint64_t H1 = 1UL << 7;
static const uint64_t A2 = 1UL << 8;
static const uint64_t B2 = 1UL << 9;
static const uint64_t C2 = 1UL << 10;
static const uint64_t D2 = 1UL << 11;
static const uint64_t E2 = 1UL << 12;
static const uint64_t F2 = 1UL << 13;
static const uint64_t G2 = 1UL << 14;
static const uint64_t H2 = 1UL << 15;
static const uint64_t A3 = 1UL << 16;
static const uint64_t B3 = 1UL << 17;
static const uint64_t C3 = 1UL << 18;
static const uint64_t D3 = 1UL << 19;
static const uint64_t E3 = 1UL << 20;
static const uint64_t F3 = 1UL << 21;
static const uint64_t G3 = 1UL << 22;
static const uint64_t H3 = 1UL << 23;
static const uint64_t A4 = 1UL << 24;
static const uint64_t B4 = 1UL << 25;
static const uint64_t C4 = 1UL << 26;
static const uint64_t D4 = 1UL << 27;
static const uint64_t E4 = 1UL << 28;
static const uint64_t F4 = 1UL << 29;
static const uint64_t G4 = 1UL << 30;
static const uint64_t H4 = 1UL << 31;
static const uint64_t A5 = 1UL << 32;
static const uint64_t B5 = 1UL << 33;
static const uint64_t C5 = 1UL << 34;
static const uint64_t D5 = 1UL << 35;
static const uint64_t E5 = 1UL << 36;
static const uint64_t F5 = 1UL << 37;
static const uint64_t G5 = 1UL << 38;
static const uint64_t H5 = 1UL << 39;
static const uint64_t A6 = 1UL << 40;
static const uint64_t B6 = 1UL << 41;
static const uint64_t C6 = 1UL << 42;
static const uint64_t D6 = 1UL << 43;
static const uint64_t E6 = 1UL << 44;
static const uint64_t F6 = 1UL << 45;
static const uint64_t G6 = 1UL << 46;
static const uint64_t H6 = 1UL << 47;
static const uint64_t A7 = 1UL << 48;
static const uint64_t B7 = 1UL << 49;
static const uint64_t C7 = 1UL << 50;
static const uint64_t D7 = 1UL << 51;
static const uint64_t E7 = 1UL << 52;
static const uint64_t F7 = 1UL << 53;
static const uint64_t G7 = 1UL << 54;
static const uint64_t H7 = 1UL << 55;
static const uint64_t A8 = 1UL << 56;
static const uint64_t B8 = 1UL << 57;
static const uint64_t C8 = 1UL << 58;
static const uint64_t D8 = 1UL << 59;
static const uint64_t E8 = 1UL << 60;
static const uint64_t F8 = 1UL << 61;
static const uint64_t G8 = 1UL << 62;
static const uint64_t H8 = 1UL << 63;

static const bool white = 1;
static const bool black = 0;

static const uint64_t ROW_1 = A1 + B1 + C1 + D1 + E1 + F1 + G1 + H1;
static const uint64_t ROW_2 = A2 + B2 + C2 + D2 + E2 + F2 + G2 + H2;
static const uint64_t ROW_3 = A3 + B3 + C3 + D3 + E3 + F3 + G3 + H3;
static const uint64_t ROW_4 = A4 + B4 + C4 + D4 + E4 + F4 + G4 + H4;
static const uint64_t ROW_5 = A5 + B5 + C5 + D5 + E5 + F5 + G5 + H5;
static const uint64_t ROW_6 = A6 + B6 + C6 + D6 + E6 + F6 + G6 + H6;
static const uint64_t ROW_7 = A7 + B7 + C7 + D7 + E7 + F7 + G7 + H7;
static const uint64_t ROW_8 = A8 + B8 + C8 + D8 + E8 + F8 + G8 + H8;

static const uint64_t A_FILE = A1 + A2 + A3 + A4 + A5 + A6 + A7 + A8;
static const uint64_t B_FILE = B1 + B2 + B3 + B4 + B5 + B6 + B7 + B8;
static const uint64_t G_FILE = G1 + G2 + G3 + G4 + G5 + G6 + G7 + G8;
static const uint64_t H_FILE = H1 + H2 + H3 + H4 + H5 + H6 + H7 + H8;

static const uint64_t SW_BORDER = A_FILE | ROW_1;
static const uint64_t SE_BORDER = H_FILE | ROW_1;
static const uint64_t NW_BORDER = A_FILE | ROW_8;
static const uint64_t NE_BORDER = H_FILE | ROW_8;

static const uint64_t NNE_BORDER = NE_BORDER | ROW_7;
static const uint64_t EEN_BORDER = NE_BORDER | G_FILE;
static const uint64_t EES_BORDER = SE_BORDER | G_FILE;
static const uint64_t SSE_BORDER = SE_BORDER | ROW_2;
static const uint64_t NNW_BORDER = NW_BORDER | ROW_7;
static const uint64_t WWN_BORDER = NW_BORDER | B_FILE;
static const uint64_t WWS_BORDER = SW_BORDER | B_FILE;
static const uint64_t SSW_BORDER = SW_BORDER | ROW_2;

static const uint64_t clear_en_passant_mask = ~ROW_3 & ~ROW_6;

static const uint64_t white_king_side_castle_squares = F1 + G1;
static const uint64_t white_queen_side_castle_squares = B1 + C1 + D1;
static const uint64_t black_king_side_castle_squares = F8 + G8;
static const uint64_t black_queen_side_castle_squares = B8 + C8 + D8;

static const uint64_t center_mask_1 = D4 + D5 + E4 + E5;
static const uint64_t center_mask_2 = C3 + C4 + C5 + C6 + D3 + D6 + E3 + E6 + F3 + F4 + F5 + F6;


struct Board_legacy {
	uint64_t white_pawns = 0;
	uint64_t white_king = 0;
	uint64_t white_bishops = 0;
	uint64_t white_knights = 0;
	uint64_t white_rooks = 0;
	uint64_t white_queen = 0;
	uint64_t black_pawns = 0;
	uint64_t black_king = 0;
	uint64_t black_bishops = 0;
	uint64_t black_knights = 0;
	uint64_t black_rooks = 0;
	uint64_t black_queen = 0;
	uint64_t meta_info = C1 + G1 + C8 + G8; // castling rights
	// move
	uint64_t from = 0; //unnecessary with 64-bits, optimize later
	uint64_t to = 0; //unnecessary with 64-bits, optimize later use meta_info instead
	bool queened = false;
	bool is_capture = false;
};


static const int WHITE = 0;
static const int BLACK = 1;

static const int PAWN = 0;
static const int KNIGHT = 1;
static const int BISHOP = 2;
static const int ROOK = 3;
static const int QUEEN = 4;
static const int KING = 5;

static const int EN_PASSANT = 6;
static const int EMPTY = 7;

// move.m set- and get macros

#define to_move(from, to, piece, color, promotion) (from + (to << 6) + (piece << 12) + (EMPTY << 16) + (color << 20) + (promotion << 24))
#define to_capture_move(from, to, piece, captured_piece, color, promotion) (from + (to << 6) + (piece << 12) + (captured_piece << 16)+ (color << 20) + (promotion << 24))
#define to_castle_move(from, to, color) (from + (to << 6) + (KING << 12) + (EMPTY << 16) + (color << 20) + (1UL << 21) + (EMPTY << 24))
#define from_square(m) (m & 0x3f)
#define to_square(m) ((m >> 6) & 0x3f)
#define piece(m) ((m >> 12) & 0xf)
#define captured_piece(m) ((m >> 16) & 0xf)
#define promotion_piece(m) ((m >> 24) & 0xf)
#define is_promotion(m) (((m >> 24) & 0xf) != EMPTY)
#define is_capture(m) (((m >> 16) & 0xf) != EMPTY)
#define color(m) ((m >> 20) & 0x1)
#define is_castling(m) ((m >> 21) & 0x1)

/* convertion from legacy*/
Board_legacy to_board(const Board& board);
Board to_board2(const Board_legacy& board);


// begin code snippet from https://chessprogramming.wikispaces.com/BitScan

const int index64[64] = {
    0, 47,  1, 56, 48, 27,  2, 60,
   57, 49, 41, 37, 28, 16,  3, 61,
   54, 58, 35, 52, 50, 42, 21, 44,
   38, 32, 29, 23, 17, 11,  4, 62,
   46, 55, 26, 59, 40, 36, 15, 53,
   34, 51, 20, 43, 31, 22, 10, 45,
   25, 39, 14, 33, 19, 30,  9, 24,
   13, 18,  8, 12,  7,  6,  5, 63
};

/**
 * bitScanForward
 * @author Kim Walisch (2012)
 * @param bb bitboard to scan
 * @precondition bb != 0
 * @return index (0..63) of least significant one bit
 */
inline int bitScanForward(uint64_t bb) {
   const uint64_t debruijn64 = 0x03f79d71b4cb0a89UL;
   return index64[((bb ^ (bb-1)) * debruijn64) >> 58];
}

/**
 * bitScanReverse
 * @authors Kim Walisch, Mark Dickinson
 * @param bb bitboard to scan
 * @precondition bb != 0
 * @return index (0..63) of most significant one bit
*/
inline int bitScanReverse(uint64_t bb) {
   const uint64_t debruijn64 = 0x03f79d71b4cb0a89UL;

   bb |= bb >> 1;
   bb |= bb >> 2;
   bb |= bb >> 4;
   bb |= bb >> 8;
   bb |= bb >> 16;
   bb |= bb >> 32;
   return index64[(bb * debruijn64) >> 58];
}

// end code snippet

#ifdef __GNUC__
	#define lsb_to_square(b) __builtin_ctzll(b)
	#define msb_to_square(b) (63 - __builtin_clzll(b))
#else
	#define lsb_to_square(b) bitScanForward(b)
	#define msb_to_square(b) bitScanReverse(b)
#endif
// TODO Macro for _MSC_VER intrinsics


#define reset_lsb(b) (b & (b-1))
#define lsb(b) (b & ~(b-1))

typedef std::deque<Board> list;
typedef std::deque<Move> MoveList;


#endif /* BOARD_H_ */
