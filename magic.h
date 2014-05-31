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
 * Must be called first
 */
void init_magic_lookup_table();

uint64_t bishop_attacks(uint64_t occupied_squares, int square);

uint64_t rook_attacks(uint64_t occupied_squares, int square);

uint64_t queen_attacks(uint64_t occupied_squares, int square);

#endif /* MAGIC_H_ */
