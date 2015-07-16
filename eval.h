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


const int DOUBLED_PAWN_PENALTY = 18;
const int ISOLATED_PAWN_PENALTY = 14;
const int BACKWARD_PAWN_PENALTY = 8;
const int PASSED_PAWN_BONUS = 39;
const int OPEN_FILE_BONUS = 14;
const int SEMI_OPEN_FILE_BONUS = 8;
const int UNSAFE_KING_PENALTY = 9;
const int BISHOP_PAIR_BONUS = 47;
const int BISHOP_MOBILITY_BONUS = 3;
const int ROOK_MOBILITY_BONUS = 3;
const int KNIGHT_KING_PROXIMITY_BONUS = 4;
const int BISHOP_KING_PROXIMITY_BONUS = 1;
const int ROOK_KING_PROXIMITY_BONUS = 1;
const int QUEEN_KING_PROXIMITY_BONUS = 10;

// piece square table parameters

const int PAWN_PSQT_BASE_VALUE_MG = 95;
const int PAWN_CENTER_BONUS_MG = 7;
const int PAWN_OPPONENT_BACK_ROW_BONUS_MG = 17;

const int PAWN_PSQT_BASE_VALUE_EG = 67;
const int PAWN_CENTER_BONUS_EG = 1;
const int PAWN_OPPONENT_BACK_ROW_BONUS_EG = 166;

const int KNIGHT_PSQT_BASE_VALUE = 295;
const int KNIGHT_CENTER_BONUS = 18;
const int KNIGHT_OPPONENT_BACK_ROW_BONUS = 0;

const int BISHOP_PSQT_BASE_VALUE = 291;
const int BISHOP_CENTER_BONUS = 19;
const int BISHOP_OPPONENT_BACK_ROW_BONUS = 12;

const int KING_PSQT_BASE_VALUE_EG = 9992;
const int KING_CENTER_BONUS_EG = 19;
const int KING_OPPONENT_BACK_ROW_BONUS_EG = 1;

const int QUEEN_PSQT_BASE_VALUE = 900;
const int QUEEN_CENTER_BONUS = 0;
const int QUEEN_OPPONENT_BACK_ROW_BONUS = 0;

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

/**
 * High score: good for white
 * Low score: good for black
 */
int evaluate(const Position& position);

/**
 * Score from side's perspective
 */
int nega_evaluate(const Position& position, bool white_turn);

void init_eval();


#endif /* EVAL_H_ */
