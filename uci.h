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
 * uci.h
 *
 *  Created on: Jan 11, 2014
 *      Author: Torbjörn Nilsson
 */

#ifndef UCI_H_
#define UCI_H_

struct FenInfo {
	Position position;
	bool white_turn;
	int move;
};

void uci();
FenInfo start_pos();
FenInfo parse_fen(std::string fen);

#endif /* UCI_H_ */
