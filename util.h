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
 * util.h
 *
 *  Created on: Jan 18, 2014
 *      Author: Torbjörn Nilsson
 */

#ifndef UTIL_H_
#define UTIL_H_

#include "board.h"
#include <string>
#include <vector>

std::string long_algebraic_notation(uint64_t square);

std::string pvstring_from_stack(int * pv, int size);

int parse_int_parameter(std::string line, std::string parameter);

void print_position(const Position& position);

void print_bit_mask(uint64_t bit_mask);

std::vector<std::string> split(std::string& line);

int perft(Position& position, int depth, bool white_turn);

#endif /* UTIL_H_ */
