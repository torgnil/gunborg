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


const int HASH_MB_FACTOR = 131072; // 65536 for 16 bytes, 87381 for 12 bytes, 131072 for 8 bytes


/*
 * size of Transposition is 8 bytes
 */
struct Transposition {
	uint32_t hash = 0;
	uint32_t next_move = 0;
};


#define hash_verification(h) ((uint32_t)(h >> 32))
#define hash_index(h) ((uint32_t) h)

#endif /* CACHE_H_ */
