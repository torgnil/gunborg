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
 * magic.cpp
 *
 *  Created on: May 31, 2014
 *      Author: Torbjörn Nilsson
 */
#include "board.h"
#include "magic.h"
#include <inttypes.h>

uint64_t ray_moves[8][64];

const int N = 0;
const int NE = 1;
const int E = 2;
const int SE = 3;
const int S = 4;
const int SW = 5;
const int W = 6;
const int NW = 7;

uint64_t attack_set_table[107648]; // all possible attack sets for bishops and rooks

AttackSetLookup bishop_lookup_table[64];
AttackSetLookup rook_lookup_table[64];

// Magic numbers generated using Tord Romstad's proposal at http://chessprogramming.wikispaces.com/Looking+for+Magics

const uint64_t ROOK_MAGICS[64] = {
  0xa8002c000108020ULL,
  0x6c00049b0002001ULL,
  0x100200010090040ULL,
  0x2480041000800801ULL,
  0x280028004000800ULL,
  0x900410008040022ULL,
  0x280020001001080ULL,
  0x2880002041000080ULL,
  0xa000800080400034ULL,
  0x4808020004000ULL,
  0x2290802004801000ULL,
  0x411000d00100020ULL,
  0x402800800040080ULL,
  0xb000401004208ULL,
  0x2409000100040200ULL,
  0x1002100004082ULL,
  0x22878001e24000ULL,
  0x1090810021004010ULL,
  0x801030040200012ULL,
  0x500808008001000ULL,
  0xa08018014000880ULL,
  0x8000808004000200ULL,
  0x201008080010200ULL,
  0x801020000441091ULL,
  0x800080204005ULL,
  0x1040200040100048ULL,
  0x120200402082ULL,
  0xd14880480100080ULL,
  0x12040280080080ULL,
  0x100040080020080ULL,
  0x9020010080800200ULL,
  0x813241200148449ULL,
  0x491604001800080ULL,
  0x100401000402001ULL,
  0x4820010021001040ULL,
  0x400402202000812ULL,
  0x209009005000802ULL,
  0x810800601800400ULL,
  0x4301083214000150ULL,
  0x204026458e001401ULL,
  0x40204000808000ULL,
  0x8001008040010020ULL,
  0x8410820820420010ULL,
  0x1003001000090020ULL,
  0x804040008008080ULL,
  0x12000810020004ULL,
  0x1000100200040208ULL,
  0x430000a044020001ULL,
  0x280009023410300ULL,
  0xe0100040002240ULL,
  0x200100401700ULL,
  0x2244100408008080ULL,
  0x8000400801980ULL,
  0x2000810040200ULL,
  0x8010100228810400ULL,
  0x2000009044210200ULL,
  0x4080008040102101ULL,
  0x40002080411d01ULL,
  0x2005524060000901ULL,
  0x502001008400422ULL,
  0x489a000810200402ULL,
  0x1004400080a13ULL,
  0x4000011008020084ULL,
  0x26002114058042ULL,
};

const uint64_t BISHOP_MAGICS[64] = {
  0x89a1121896040240ULL,
  0x2004844802002010ULL,
  0x2068080051921000ULL,
  0x62880a0220200808ULL,
  0x4042004000000ULL,
  0x100822020200011ULL,
  0xc00444222012000aULL,
  0x28808801216001ULL,
  0x400492088408100ULL,
  0x201c401040c0084ULL,
  0x840800910a0010ULL,
  0x82080240060ULL,
  0x2000840504006000ULL,
  0x30010c4108405004ULL,
  0x1008005410080802ULL,
  0x8144042209100900ULL,
  0x208081020014400ULL,
  0x4800201208ca00ULL,
  0xf18140408012008ULL,
  0x1004002802102001ULL,
  0x841000820080811ULL,
  0x40200200a42008ULL,
  0x800054042000ULL,
  0x88010400410c9000ULL,
  0x520040470104290ULL,
  0x1004040051500081ULL,
  0x2002081833080021ULL,
  0x400c00c010142ULL,
  0x941408200c002000ULL,
  0x658810000806011ULL,
  0x188071040440a00ULL,
  0x4800404002011c00ULL,
  0x104442040404200ULL,
  0x511080202091021ULL,
  0x4022401120400ULL,
  0x80c0040400080120ULL,
  0x8040010040820802ULL,
  0x480810700020090ULL,
  0x102008e00040242ULL,
  0x809005202050100ULL,
  0x8002024220104080ULL,
  0x431008804142000ULL,
  0x19001802081400ULL,
  0x200014208040080ULL,
  0x3308082008200100ULL,
  0x41010500040c020ULL,
  0x4012020c04210308ULL,
  0x208220a202004080ULL,
  0x111040120082000ULL,
  0x6803040141280a00ULL,
  0x2101004202410000ULL,
  0x8200000041108022ULL,
  0x21082088000ULL,
  0x2410204010040ULL,
  0x40100400809000ULL,
  0x822088220820214ULL,
  0x40808090012004ULL,
  0x910224040218c9ULL,
  0x402814422015008ULL,
  0x90014004842410ULL,
  0x1000042304105ULL,
  0x10008830412a00ULL,
  0x2520081090008908ULL,
  0x40102000a0a60140ULL,
};

// Begin code snippet from Tord Romstad's magic generator(relevant bits/square and mask generation)

const int ROOK_BITS[64] = {
  12, 11, 11, 11, 11, 11, 11, 12,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  11, 10, 10, 10, 10, 10, 10, 11,
  12, 11, 11, 11, 11, 11, 11, 12
};

const int BISHOP_BITS[64] = {
  6, 5, 5, 5, 5, 5, 5, 6,
  5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 7, 7, 7, 7, 5, 5,
  5, 5, 7, 9, 9, 7, 5, 5,
  5, 5, 7, 9, 9, 7, 5, 5,
  5, 5, 7, 7, 7, 7, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5,
  6, 5, 5, 5, 5, 5, 5, 6
};

uint64_t rook_mask(int square) {
	uint64_t result = 0ULL;
	int rank = square / 8, file = square % 8, r, f;
	for (r = rank + 1; r <= 6; r++)
		result |= (1ULL << (file + r * 8));
	for (r = rank - 1; r >= 1; r--)
		result |= (1ULL << (file + r * 8));
	for (f = file + 1; f <= 6; f++)
		result |= (1ULL << (f + rank * 8));
	for (f = file - 1; f >= 1; f--)
		result |= (1ULL << (f + rank * 8));
	return result;
}

uint64_t bishop_mask(int square) {
	uint64_t result = 0ULL;
	int rank = square / 8, file = square % 8, r, f;
	for (r = rank + 1, f = file + 1; r <= 6 && f <= 6; r++, f++)
		result |= (1ULL << (f + r * 8));
	for (r = rank + 1, f = file - 1; r <= 6 && f >= 1; r++, f--)
		result |= (1ULL << (f + r * 8));
	for (r = rank - 1, f = file + 1; r >= 1 && f <= 6; r--, f++)
		result |= (1ULL << (f + r * 8));
	for (r = rank - 1, f = file - 1; r >= 1 && f >= 1; r--, f--)
		result |= (1ULL << (f + r * 8));
	return result;
}

// end code snippet

/**
 * Ray for direction N E NE NW
 */
uint64_t get_positive_ray_moves(const int& dir, const int& from, const uint64_t& occupied_squares) {
	uint64_t attacked_squares = ray_moves[dir][from];
	uint64_t blocker = attacked_squares & occupied_squares;
	attacked_squares ^= ray_moves[dir][lsb_to_square(blocker | H8)];
	return attacked_squares;
}

/**
 * Ray for direction S W SW SE
 */
uint64_t get_negative_ray_moves(const int& dir, const int& from, const uint64_t& occupied_squares) {
	uint64_t attacked_squares = ray_moves[dir][from];
	uint64_t blocker = attacked_squares & occupied_squares;
	attacked_squares ^= ray_moves[dir][msb_to_square(blocker | A1)];
	return attacked_squares;
}

void init_ray_moves() {
	for (int i = 0; i < 64; i++) {
		uint64_t b = 1ULL << i;
		uint64_t to_squares = 0;
		while (b & ~ROW_8) {
			b = b << 8UL;
			to_squares |= b;
		}
		ray_moves[N][i] = to_squares;
	}
	for (int i = 0; i < 64; i++) {
		uint64_t b = 1ULL << i;
		uint64_t to_squares = 0;
		while (b & ~ROW_1) {
			b = b >> 8UL;
			to_squares |= b;
		}
		ray_moves[S][i] = to_squares;
	}
	for (int i = 0; i < 64; i++) {
		uint64_t b = 1ULL << i;
		uint64_t to_squares = 0;
		while (b & ~H_FILE) {
			b = b << 1ULL;
			to_squares |= b;
		}
		ray_moves[E][i] = to_squares;
	}
	for (int i = 0; i < 64; i++) {
		uint64_t b = 1ULL << i;
		uint64_t to_squares = 0;
		while (b & ~A_FILE) {
			b = b >> 1ULL;
			to_squares |= b;
		}
		ray_moves[W][i] = to_squares;
	}
	for (int i = 0; i < 64; i++) {
		uint64_t b = 1ULL << i;
		uint64_t to_squares = 0;
		while (b & ~NW_BORDER) {
			b = b << 7UL;
			to_squares |= b;
		}
		ray_moves[NW][i] = to_squares;
	}
	for (int i = 0; i < 64; i++) {
		uint64_t b = 1ULL << i;
		uint64_t to_squares = 0;
		while (b & ~SW_BORDER) {
			b = b >> 9UL;
			to_squares |= b;
		}
		ray_moves[SW][i] = to_squares;
	}
	for (int i = 0; i < 64; i++) {
		uint64_t b = 1ULL << i;
		uint64_t to_squares = 0;
		while (b & ~NE_BORDER) {
			b = b << 9UL;
			to_squares |= b;
		}
		ray_moves[NE][i] = to_squares;
	}
	for (int i = 0; i < 64; i++) {
		uint64_t b = 1ULL << i;
		uint64_t to_squares = 0;
		while (b & ~SE_BORDER) {
			b = b >> 7UL;
			to_squares |= b;
		}
		ray_moves[SE][i] = to_squares;
	}
}

void init_magic_lookup_table() {

	init_ray_moves();

	int attack_table_offset = 0;
	for (int square = 0; square < 64; ++square) {

		// generate all possible bishop attack sets

		uint64_t b_mask = bishop_mask(square);
		uint64_t* bishop_attack_table_ptr = &attack_set_table[attack_table_offset];

		AttackSetLookup bishop_lookup;
		bishop_lookup.attack_set_table_ptr = bishop_attack_table_ptr;
		bishop_lookup.magic = BISHOP_MAGICS[square];
		bishop_lookup.mask = b_mask;
		bishop_lookup.shift = 64 - BISHOP_BITS[square];
		bishop_lookup_table[square] = bishop_lookup;

		// loop over all possible combinations of bits set in the mask
		uint64_t occupied_squares = 0;
		do {
			uint64_t to_squares = get_positive_ray_moves(NW, square, occupied_squares)
					| get_positive_ray_moves(NE, square, occupied_squares)
					| get_negative_ray_moves(SW, square, occupied_squares)
					| get_negative_ray_moves(SE, square, occupied_squares);

			int key = get_lookup_offset(occupied_squares, BISHOP_MAGICS[square], 64 - BISHOP_BITS[square]);
			bishop_attack_table_ptr[key] = to_squares;
			occupied_squares = (occupied_squares - b_mask) & b_mask;
			attack_table_offset++;
		} while (occupied_squares);

		// generate all possible rook attack sets

		uint64_t r_mask = rook_mask(square);
		uint64_t* rook_attack_table_ptr = &attack_set_table[attack_table_offset];

		AttackSetLookup rook_lookup;
		rook_lookup.attack_set_table_ptr = rook_attack_table_ptr;
		rook_lookup.magic = ROOK_MAGICS[square];
		rook_lookup.mask = r_mask;
		rook_lookup.shift = 64 - ROOK_BITS[square];
		rook_lookup_table[square] = rook_lookup;

		// loop over all possible combinations of bits set in the mask
		occupied_squares = 0;
		do {
			uint64_t to_squares = get_positive_ray_moves(N, square, occupied_squares)
					| get_positive_ray_moves(E, square, occupied_squares)
					| get_negative_ray_moves(W, square, occupied_squares)
					| get_negative_ray_moves(S, square, occupied_squares);

			int key = get_lookup_offset(occupied_squares, ROOK_MAGICS[square], 64 - ROOK_BITS[square]);
			rook_attack_table_ptr[key] = to_squares;
			occupied_squares = (occupied_squares - r_mask) & r_mask;

			attack_table_offset++;
		} while (occupied_squares);
	}
}
