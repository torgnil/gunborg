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
 * util.cpp
 *
 *  Created on: Jan 18, 2014
 *      Author: Torbjörn Nilsson
 */

#include "board.h"
#include "util.h"
#include "moves.h"
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <stdlib.h>

std::string pvstring_from_stack(int * pv, int size) {
	std::string pv_string;
	for (int i = 0; i < size; i++) {
		if (1ULL << from_square(pv[i]) == 1ULL << to_square(pv[i])) {
			// garbage..
			return pv_string;
		}
		pv_string += long_algebraic_notation(1ULL << from_square(pv[i]));
		pv_string += long_algebraic_notation(1ULL << to_square(pv[i]));
		if (is_promotion(pv[i])) {
			pv_string += "q";
		}
		pv_string += " ";
	}
	return pv_string;
}

std::string long_algebraic_notation(uint64_t square) {
	switch (square) {
	case A1:
		return "a1";
	case A2:
		return "a2";
	case A3:
		return "a3";
	case A4:
		return "a4";
	case A5:
		return "a5";
	case A6:
		return "a6";
	case A7:
		return "a7";
	case A8:
		return "a8";
	case B1:
		return "b1";
	case B2:
		return "b2";
	case B3:
		return "b3";
	case B4:
		return "b4";
	case B5:
		return "b5";
	case B6:
		return "b6";
	case B7:
		return "b7";
	case B8:
		return "b8";
	case C1:
		return "c1";
	case C2:
		return "c2";
	case C3:
		return "c3";
	case C4:
		return "c4";
	case C5:
		return "c5";
	case C6:
		return "c6";
	case C7:
		return "c7";
	case C8:
		return "c8";
	case D1:
		return "d1";
	case D2:
		return "d2";
	case D3:
		return "d3";
	case D4:
		return "d4";
	case D5:
		return "d5";
	case D6:
		return "d6";
	case D7:
		return "d7";
	case D8:
		return "d8";
	case E1:
		return "e1";
	case E2:
		return "e2";
	case E3:
		return "e3";
	case E4:
		return "e4";
	case E5:
		return "e5";
	case E6:
		return "e6";
	case E7:
		return "e7";
	case E8:
		return "e8";
	case F1:
		return "f1";
	case F2:
		return "f2";
	case F3:
		return "f3";
	case F4:
		return "f4";
	case F5:
		return "f5";
	case F6:
		return "f6";
	case F7:
		return "f7";
	case F8:
		return "f8";
	case G1:
		return "g1";
	case G2:
		return "g2";
	case G3:
		return "g3";
	case G4:
		return "g4";
	case G5:
		return "g5";
	case G6:
		return "g6";
	case G7:
		return "g7";
	case G8:
		return "g8";
	case H1:
		return "h1";
	case H2:
		return "h2";
	case H3:
		return "h3";
	case H4:
		return "h4";
	case H5:
		return "h5";
	case H6:
		return "h6";
	case H7:
		return "h7";
	case H8:
		return "h8";
	}
	return "";
}

void print_position(const Position& position) {
	for (int i = 63; i >= 0; i--) {
		int file = 7 - i % 8;
		int row = i / 8;
		uint64_t square = 1ULL << ((8 * row) + file);
		if (position.p[WHITE][PAWN] & square) {
			std::cout << "♙";
		} else if (position.p[BLACK][PAWN] & square) {
			std::cout << "♟";
		} else if (position.p[WHITE][KING] & square) {
			std::cout << "♔";
		} else if (position.p[BLACK][KING] & square) {
			std::cout << "♚";
		} else if (position.p[WHITE][BISHOP] & square) {
			std::cout << "♗";
		} else if (position.p[BLACK][BISHOP] & square) {
			std::cout << "♝";
		} else if (position.p[WHITE][ROOK] & square) {
			std::cout << "♖";
		} else if (position.p[BLACK][ROOK] & square) {
			std::cout << "♜";
		} else if (position.p[WHITE][QUEEN] & square) {
			std::cout << "♕";
		} else if (position.p[BLACK][QUEEN] & square) {
			std::cout << "♛";
		} else if (position.p[WHITE][KNIGHT] & square) {
			std::cout << "♘";
		} else if (position.p[BLACK][KNIGHT] & square) {
			std::cout << "♞";
		} else {
			std::cout << ".";
		}
		std::cout << " ";
		if (i % 8 == 0) {
			std::cout << "\n";
		}
	}
}

void print_bit_mask(uint64_t bit_mask) {
	for (int i = 7; i >= 0; i--) {
		for (int j = 0; j < 8; j++) {
			uint64_t square = 1ULL << (i*8  + j);
			if (square & bit_mask) {
				std::cout << "1";
			} else {
				std::cout << "0";
			}
		}
		std::cout << "\n";
	}
	std::cout << "\n";
}
std::vector<std::string> split(std::string& line) {
	std::stringstream ss(line);
	std::string s;
	std::vector<std::string> result;
	while (getline(ss, s, ' ')) {
		result.push_back(s);
	}
	return result;
}

int parse_int_parameter(std::string line, std::string parameter) {
	std::string::size_type pos = line.find(parameter);
	if (pos != std::string::npos) {
		std::string sub_str = line.substr(pos + parameter.size() + 1);
		std::stringstream ss(sub_str);
		std::string result_str;
		getline(ss, result_str, ' ');
		return atoi(result_str.c_str());
	}
	return 0;
}

int perft(Position& position, const int depth, const bool white_turn) {
	if (depth == 0) {
		return 1;
	}
	int nodes = 0;
	MoveList moves = get_moves(position, white_turn);
	for (auto it : moves) {
		bool legal = make_move(position, it);
		if (legal) {
			nodes += perft(position, depth - 1, !white_turn);
		}
		unmake_move(position, it);
	}
	return nodes;
}
