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
 * magic.h
 *
 *  Created on: May 31, 2014
 *      Author: Torbjörn Nilsson
 */

#ifndef MAGIC_H_
#define MAGIC_H_

#include <inttypes.h>

/*
 * Must be called first to initialize the attack sets
 */
void init_magic_lookup_table();

/*
 * Magic bit boards.
 *
 * A pre-generated lookup table holds all possible rook and bishop attack sets.
 *
 * Square + occupied_squares mask, using some magic numbers, gives the correct index in the table.
 *
 * See http://chessprogramming.wikispaces.com/Magic+Bitboards
 */

struct AttackSetLookup {
	uint64_t* attack_set_table_ptr;  // pointer to attack_table for each particular square
	uint64_t mask;  // to mask relevant squares of both lines (no outer squares)
	uint64_t magic; // magic 64-bit factor
	int shift; // shift right (64 - number of bits)
};

extern AttackSetLookup bishop_lookup_table[64];
extern AttackSetLookup rook_lookup_table[64];


inline int get_lookup_offset(uint64_t occupied_squares, uint64_t magic, int shift) {
	return (int) ((occupied_squares * magic) >> shift);
}

inline uint64_t bishop_attacks(uint64_t occupied_squares, int square) {
	int index = get_lookup_offset(occupied_squares & bishop_lookup_table[square].mask,
			bishop_lookup_table[square].magic, bishop_lookup_table[square].shift);
	return bishop_lookup_table[square].attack_set_table_ptr[index];
}

inline uint64_t rook_attacks(uint64_t occupied_squares, int square) {
	int index = get_lookup_offset(occupied_squares & rook_lookup_table[square].mask, rook_lookup_table[square].magic,
			rook_lookup_table[square].shift);
	return rook_lookup_table[square].attack_set_table_ptr[index];
}

inline uint64_t queen_attacks(uint64_t occupied_squares, int square) {
	return bishop_attacks(occupied_squares, square) | rook_attacks(occupied_squares, square);
}

#endif /* MAGIC_H_ */
