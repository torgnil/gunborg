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
 * Main.cpp
 *
 *  Created on: Jun 11, 2013
 *      Author: Torbjörn Nilsson
 */

#include "board.h"
#include "eval.h"
#include "Main.h"
#include "moves.h"
#include "uci.h"
#include "util.h"
#include "test.h"
#include <cstring>
#include <iomanip>
#include <iostream>

using namespace std;
namespace {

}

int main(int argc, char* argv[]) {
	std::cout << fixed;
	std::cout << setprecision(2);
	init();
	init_eval();

	std::cout << "Gunborg Copyright (C) 2013-2015 Torbjörn Nilsson\n"
			<< "This program comes with ABSOLUTELY NO WARRANTY; for details type `show w'.\n"
			<< "This is free software, and you are welcome to redistribute it\n"
			<< "under certain conditions; type `show c' for details.\n";

	if (argc == 2 && strcmp(argv[1], "test") == 0) {
		run_tests();
	} else {
		uci();
	}
	return 0;
}
