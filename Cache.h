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
 * Cache.h
 *
 *  Created on: Jun 17, 2013
 *      Author: Torbjörn Nilsson
 */

#include "board.h"

#ifndef CACHE_H_
#define CACHE_H_

const uint8_t TT_TYPE_EXACT = 1;
const uint8_t TT_TYPE_LOWER_BOUND = 2;
const uint8_t TT_TYPE_UPPER_BOUND = 3;

const int TT_BUCKET_SIZE = 4;

extern uint64_t hash_size;


/*
 * size of Transposition is 13 bytes
 */
struct Transposition {
	uint32_t hash = 0;
	uint32_t next_move = 0;
	uint8_t depth = 0;
	uint8_t type = 0;
	int16_t score = 0;
	uint8_t generation = 0;
};


#define hash_verification(h) ((uint32_t)(h >> 32))

/*
 * probes the transposition table
 *
 * returns a hit(same verifcation and the bucket) or a pointer to the element that should be replaced in the bucket
 *
 * replaces the element with the lowest depth and the lowest generation
 *
 */
inline Transposition* probe_tt(Transposition *tt, const uint64_t& hash_key, const uint8_t& generation) {
	uint64_t bucket_start_index = TT_BUCKET_SIZE * ((uint32_t) ((hash_key)) & ((hash_size - 1) / TT_BUCKET_SIZE));
	uint64_t tt_index = bucket_start_index;
	uint8_t lowest_depth = 32;
	uint8_t lowest_generation = 255;
	for (int i = 0; i < TT_BUCKET_SIZE; i++) {
		if (tt[bucket_start_index + i].hash == hash_verification(hash_key)) {
			return &tt[bucket_start_index + i];
		}
		if (tt[bucket_start_index + i].generation <= lowest_generation) {
			if (tt[bucket_start_index + i].generation < lowest_generation) {
				lowest_depth = 32;
			}
			if (tt[bucket_start_index + i].depth <= lowest_depth) {
				tt_index = bucket_start_index + i;
				lowest_depth = tt[bucket_start_index + i].depth;
			}
			lowest_generation = tt[bucket_start_index + i].generation;
		}
	}
	return &tt[tt_index];
}

inline uint64_t get_hash_table_size(int hash_size_mb) {
	return 1ULL << msb_to_square(hash_size_mb*1024*1024 / sizeof(Transposition));
}

/*
 * approximation of hash full in per mille
 *
 */
inline int hashfull(Transposition *tt) {
	int count = 0;
	for (unsigned int i = 0; i < 1000; i++) {
		if (tt[i+i].hash != 0) {
			count++;
		}
	}
	return count;
}

#endif /* CACHE_H_ */
