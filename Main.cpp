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
 * Main.cpp
 *
 *  Created on: Jun 11, 2013
 *      Author: Torbjörn Nilsson
 */

#include "board.h"
#include "Cache.h"
#include "Main.h"
#include "moves.h"
#include "uci.h"
#include "util.h"
#include "test.h"
#include <atomic>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits.h>
#include <stdlib.h>
#include <time.h>
#include <thread>
#include <deque>
#include <cmath>

using namespace std;
namespace {

}

int main(int argc, char* argv[]) {
	std::cout << fixed;
	std::cout << setprecision(2);
	init();

	std::cout << "Gunborg Copyright (C) 2013-2014 Torbjörn Nilsson\n"
			<< "This program comes with ABSOLUTELY NO WARRANTY;\n"
			<< "This is free software, and you are welcome to redistribute it\n"
			<< "under certain conditions; See LICENSE.TXT\n";

	if (argc == 2 && strcmp(argv[1], "perft") == 0) {
		std::chrono::high_resolution_clock clock;
		std::chrono::high_resolution_clock::time_point start;

		string fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
		Board b2 = parseFen(fen).board;
		for (int i = 1; i <= 6; i++) {
			start = clock.now();
			std::cout << "perft depth(" << i << ") nodes: "
					<< perft(b2, i, true);
			int time_elapsed = std::chrono::duration_cast
					< std::chrono::milliseconds > (clock.now() - start).count();
			std::cout << " in " << time_elapsed << " ms\n";
		}
	} else if (argc == 2 && strcmp(argv[1], "test") == 0) {
		run_tests();
	} else {
		thread uci_thread(uci);
		uci_thread.join();
	}
	return 0;
}
