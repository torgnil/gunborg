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
 * eval.h
 *
 *  Created on: Jan 18, 2014
 *      Author: Torbjörn Nilsson
 */

#ifndef EVAL_H_
#define EVAL_H_

#include "board.h"


const int DOUBLED_PAWN_PENALTY = 14;
const int ISOLATED_PAWN_PENALTY = 9;
const int BACKWARD_PAWN_PENALTY = 7;
const int PASSED_PAWN_BONUS = 38;
const int OPEN_FILE_BONUS = 10;
const int SEMI_OPEN_FILE_BONUS = 5;
const int UNSAFE_KING_PENALTY = 9;
const int BISHOP_PAIR_BONUS = 44;
const int BISHOP_MOBILITY_BONUS = 2;
const int ROOK_MOBILITY_BONUS = 4;
const int KNIGHT_KING_PROXIMITY_BONUS = 6;
const int ROOK_KING_PROXIMITY_BONUS = 4;
const int QUEEN_KING_PROXIMITY_BONUS = 11;
const int BISHOP_KING_PROXIMITY_BONUS = 0;

const int PIECE_VALUES[7] = {100, 300, 300, 500, 900, 10000, 100};

// the value of a white pawn at all squares from white's perspective
const int PAWN_SQUARE_TABLE[] = {
		// A1
		100, 100, 100, 100, 100, 100, 100, 100, // H1
		100, 100, 100, 95, 95, 100, 100, 100,
		101, 103, 104, 98, 98, 104, 103, 101,
		102, 105, 106, 111, 111, 106, 105, 102,
		103, 107, 109, 111, 111, 109, 107, 103,
		105, 109, 111, 111, 111, 111, 109, 105,
		107, 111, 113, 113,	113, 113, 111, 107,
  /*A8*/100, 100, 100, 100, 100, 100, 100, 100 // H8
		};

const int PAWN_SQUARE_TABLE_ENDGAME[] = {
		// A1
		100, 100, 100, 100, 100, 100, 100, 100, // H1
		100, 100, 100, 100, 100, 100, 100, 100,
		105, 105, 105, 105, 105, 105, 105, 105,
		111, 111, 111, 111, 111, 111, 111, 111,
		130, 130, 130, 130, 130, 130, 130, 130,
		170, 170, 170, 170, 170, 170, 170, 170,
		220, 220, 220, 220, 220, 220, 220, 220,
  /*A8*/100, 100, 100, 100, 100, 100, 100, 100 // H8
		};

const int KNIGHT_SQUARE_TABLE[] = {
		290, 295, 295, 295, 295, 295, 295, 290,
		295, 300, 300, 300, 300, 300, 300, 295,
		295, 300, 305, 307, 307, 305, 300, 295,
		295, 300, 309, 310, 310, 309, 300, 295,
		295, 300, 309, 310, 310, 309, 300, 295,
		295, 300, 305, 307, 307, 305, 300, 295,
		295, 300, 300, 300, 300, 300, 300, 295,
		290, 295, 295, 295, 295, 295, 295, 290, };

const int BISHOP_SQUARE_TABLE[] = {
		275, 295, 295, 295, 295, 295, 295, 275,
		295, 300, 300, 300, 300, 300, 300, 295,
		295, 300, 308, 306, 306, 308, 300, 295,
		295, 300, 310, 309, 309, 310, 300, 295,
		295, 300, 311, 310, 310, 311, 300, 295,
		295, 300, 312, 312, 312, 312, 300, 295,
		295, 305, 314, 315, 315, 314, 305, 295,
		295, 305, 305, 309, 309, 305, 305, 295, };

const int ROOK_SQUARE_TABLE[] = {
		// A1
		495, 500, 500, 510, 510, 500, 500, 495,
		500, 500, 500, 500, 500, 500, 500, 500,
		500, 500, 500, 500, 500, 500, 500, 500,
		500, 500, 500, 500, 500, 500, 500, 500,
		500, 500, 500, 500, 500, 500, 500, 500,
		500, 500, 500, 500, 500, 500, 500, 500,
		515, 515, 515, 515, 515, 515, 515, 515,
		495, 500, 500, 500, 500, 500, 500, 495, }; //H8

const int QUEEN_SQUARE_TABLE[] {
		895, 895, 895, 895, 895, 895, 895, 895,
		895, 900, 900, 900, 900, 900, 900, 895,
		895, 900, 900, 900, 900, 900, 900, 895,
		895, 900, 900, 905, 905, 900, 900, 895,
		895, 900, 900, 905, 905, 900, 900, 895,
		895, 900, 900, 900, 900, 900, 900, 895,
		895, 900, 900, 900, 900, 900, 900, 895,
		895, 895, 895, 895, 895, 895, 895, 895,
};

const int KING_SQUARE_TABLE[] {
		// A1
		10000, 10010, 10020, 9990, 9995, 9990, 10020, 10000,
		10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000,
		10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000,
		10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000,
		10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000,
		10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000,
		10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000,
		10000, 10010, 10020, 9990, 9995, 9990, 10020, 10000, // H8
};

const int KING_SQUARE_TABLE_ENDGAME[] = {
		9980, 9990, 9990, 9990, 9990, 9990, 9990, 9980,
		9990, 9995, 9995, 9995, 9995, 9995, 9995, 9990,
		9995, 10000, 10000, 10005, 10005, 10000, 10000, 9995,
		9995, 10005, 10005, 10010, 10010, 10005, 10005, 9995,
		9995, 10005, 10005, 10010, 10010, 10005, 10005, 9995,
		9995, 10000, 10000, 10005, 10005, 10000, 10000, 9995,
		9990, 9995, 9995, 9995, 9995, 9995, 9995, 9990,
		9980, 9990, 9990, 9990, 9990, 9990, 9990, 9980
};

/**
 * High score: good for white
 * Low score: good for black
 */
int evaluate(const Position& position);

/**
 * Score from side's perspective
 */
int nega_evaluate(const Position& position, const bool& white_turn);

void init_eval();


#endif /* EVAL_H_ */
