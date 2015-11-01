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
 * uci.cpp
 *
 *  Created on: Jan 11, 2014
 *      Author: Torbjörn Nilsson
 */

#include "board.h"
#include "moves.h"
#include "Search.h"
#include "uci.h"
#include "util.h"
#include <atomic>
#include <iostream>
#include <limits.h>
#include <thread>
#include <vector>
#include <stdlib.h>

using namespace std;

namespace {

const char* VERSION = "1.65";
const int DEFAULT_HASH_SIZE_MB = 16;

}


uint64_t hash_size = get_hash_table_size(DEFAULT_HASH_SIZE_MB);

FenInfo start_pos() {
	return parse_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

FenInfo parse_fen(string fen) {
	Position position;
	uint64_t meta_info = 0;
	int row = 8;
	int file = 1;
	for (int i = 0; !(row == 1 && file == 9); i++) {
		char c = fen[i];
		int j = (row - 1) * 8 + file - 1;
		if (c == 'r') {
			position.p[BLACK][ROOK] += (1ULL << j);
		}
		if (c == 'n') {
			position.p[BLACK][KNIGHT] += (1ULL << j);
		}
		if (c == 'b') {
			position.p[BLACK][BISHOP] += (1ULL << j);
		}
		if (c == 'q') {
			position.p[BLACK][QUEEN] += (1ULL << j);
		}
		if (c == 'k') {
			position.p[BLACK][KING] += (1ULL << j);
		}
		if (c == 'p') {
			position.p[BLACK][PAWN] += (1ULL << j);
		}
		if (c == 'R') {
			position.p[WHITE][ROOK] += (1ULL << j);
		}
		if (c == 'N') {
			position.p[WHITE][KNIGHT] += (1ULL << j);
		}
		if (c == 'B') {
			position.p[WHITE][BISHOP] += (1ULL << j);
		}
		if (c == 'Q') {
			position.p[WHITE][QUEEN] += (1ULL << j);
		}
		if (c == 'K') {
			position.p[WHITE][KING] += (1ULL << j);
		}
		if (c == 'P') {
			position.p[WHITE][PAWN] += (1ULL << j);
		}
		if (c >= '0' && c <= '9') {
			int skips = c - '0';
			file += skips;
		} else {
			file++;
		}
		if (c == '/') {
			row--;
			file = 1;
		}
	}

	// 1. position
	// 2. turn
	// 3. castle rights
	// 4. en passant squares
	// 5. moves since capture and/or pawn move
	// 6. move number.
	vector<string> fen_strs = split(fen);

	string castling_rights_str = fen_strs[2];
	if (castling_rights_str.find('K') != string::npos) {
		meta_info |= G1;
	}
	if (castling_rights_str.find('Q') != string::npos) {
		meta_info |= C1;
	}
	if (castling_rights_str.find('k') != string::npos) {
		meta_info |= G8;
	}
	if (castling_rights_str.find('q') != string::npos) {
		meta_info |= C8;
	}
	string en_passant = fen_strs[3];
	if (en_passant.size() == 2) {
		uint64_t en_passant_square = 1ULL << (8 * (en_passant[1] - '0' -1) + en_passant[0] - 'a');
		meta_info |= en_passant_square;
	}
	position.meta_info_stack.push_back(meta_info);

	FenInfo fen_info;
	fen_info.position = position;
	if (fen.find("w") != string::npos) {
		fen_info.white_turn = true;
	} else {
		fen_info.white_turn = false;
	}

	if (fen_strs.size() > 4) {
		fen_info.move = atoi(fen_strs[5].c_str());
	}
	return fen_info;
}

void update_with_move(Position& position, string move_str, bool white_turn) {
	int from_file = move_str[0] - 'a' + 1;
	int from_row = move_str[1] - '0';
	int from = (from_row - 1) * 8 + from_file - 1;

	int to_file = move_str[2] - 'a' + 1;
	int to_row = move_str[3] - '0';
	int to = (to_row - 1) * 8 + to_file - 1;

	uint64_t black_squares = position.p[BLACK][KING] | position.p[BLACK][PAWN] | position.p[BLACK][KNIGHT]
			| position.p[BLACK][BISHOP] | position.p[BLACK][ROOK] | position.p[BLACK][QUEEN];

	uint64_t white_squares = position.p[WHITE][KING] | position.p[WHITE][PAWN] | position.p[WHITE][KNIGHT]
			| position.p[WHITE][BISHOP] | position.p[WHITE][ROOK] | position.p[WHITE][QUEEN];

	uint64_t occupied_squares = black_squares | white_squares;

	uint64_t from_square = 1ULL << from;
	uint64_t to_square = 1ULL << to;

	uint64_t meta_info = position.meta_info_stack.back();

	Move move;
	int promotion = EMPTY;
	int piece = piece_at_square(position, from, white_turn ? WHITE : BLACK);
	int color = white_turn ? WHITE : BLACK;
	if (piece == PAWN && ((to_square & ROW_8) || (to_square & ROW_1))) {
		if (move_str.size() == 5) {
			if (move_str[4] == 'n') {
				promotion = KNIGHT;
			} else if (move_str[4] == 'b') {
				promotion = BISHOP;
			} else if (move_str[4] == 'r') {
				promotion = ROOK;
			} else if (move_str[4] == 'q') {
				promotion = QUEEN;
			} else {
				// unexpected promotion piece type, assume queen
				promotion = QUEEN;
			}
		} else {
			// promotion piece type is for some reason is missing, assume queen
			promotion = QUEEN;
		}
	}
	bool en_passant_capture = false;
	if (piece == PAWN && (to_square & meta_info & (ROW_6 | ROW_3))) {
		en_passant_capture = true;
	}
	if (piece == KING
			&& ((to_square == G1 && from_square == E1) || (to_square == C1 && from_square == E1)
					|| (to_square == G8 && from_square == E8) || (to_square == C8 && from_square == E8))) {
		move.m = to_castle_move(from, to, color);
	} else if ((to_square & occupied_squares) || en_passant_capture) {
		int captured_color = color ^ 1;
		int captured_piece = piece_at_square(position, to, captured_color);
		move.m = to_capture_move(from, to, piece, captured_piece, color, promotion);
	} else {
		move.m = to_move(from, to, piece, color, promotion);
	}

	make_move(position, move);
}

void uci() {
	thread* search_thread = NULL;

	FenInfo fen_info =  start_pos();
	Position start_position = fen_info.position;
	bool white_turn = fen_info.white_turn;
	int move = fen_info.move;

	gunborg::Search* search = NULL;
	list history;
	Transposition * tt = new Transposition[hash_size];
	while (true) {
		string line;
		getline(cin, line);
		if (line.find("uci") != string::npos) {
			cout << "id name gunborg " << VERSION << "\n";
			cout << "id author Torbjorn Nilsson\n";
			cout << "option name Hash type spin default " << DEFAULT_HASH_SIZE_MB << " min 1 max 1024\n";
			cout << "option name Ponder type check default false\n";
			cout << "uciok\n" << flush;
		}
		if (line.find("isready") != string::npos) {
			cout << "readyok\n" << flush;
		}
		if (line.find("ucinewgame") != string::npos) {
			// new game
			FenInfo fen_info = start_pos();
			start_position = fen_info.position;
			white_turn = fen_info.white_turn;
			move = fen_info.move;
			delete[] tt;
			tt = new Transposition[hash_size];
		}
		if (line.find("setoption name Hash") != string::npos) {
			int hash_size_in_mb = parse_int_parameter(line, "value");
			if (hash_size_in_mb >= 1 && hash_size_in_mb <= 1024) {
				hash_size = get_hash_table_size(hash_size_in_mb);
			}
			delete[] tt;
			tt = new Transposition[hash_size];
		}
		if (line.find("position") != string::npos) {
			history.clear();
			// parse position
			// position [fen <fenstring> | startpos ]  moves <move1> .... <movei>
			if (line.find("startpos") != string::npos) {
				FenInfo fen_info = start_pos();
				start_position = fen_info.position;
				white_turn = fen_info.white_turn;
				move = fen_info.move;
			}
			string::size_type pos = line.find("fen");
			if (pos != string::npos) {
				string fen = line.substr(pos + 4);
				FenInfo fen_info = parse_fen(fen);
				start_position = fen_info.position;
				white_turn = fen_info.white_turn;
				move = fen_info.move;
			}
			string::size_type moves_pos = line.find("moves");
			if (moves_pos != string::npos) {
				string moves_str = line.substr(moves_pos + 6);
				vector<string> splited = split(moves_str);
				move = splited.size() / 2;
				for (auto it : splited) {
					Position copy = start_position;
					history.push_back(copy);

					update_with_move(start_position, it, white_turn);

					white_turn = !white_turn;
				}
			}
		}
		if (line.find("go") != string::npos) {
			if (search_thread != NULL) {
				search->should_run = false;
				search_thread->join();
				delete search_thread;
				search_thread = NULL;
			}
			if (search != NULL) {
				delete search;
				search = NULL;
			}
			search = new gunborg::Search();
			search->should_run = true;
			search->generation = move;

			int depth = parse_int_parameter(line, "depth");
			if (depth != 0 ) {
				search->max_depth = depth < 32 ? depth : 32;
			}
			if (line.find("infinite") != string::npos) {
				search->max_think_time_ms = INT_MAX;
			} else if (line.find("movetime") != string::npos) {
				int fixed_move_time_ms = parse_int_parameter(line, "movetime");
				search->max_think_time_ms = fixed_move_time_ms - 3;
				search->save_time = false;
			} else {
				int w_time = parse_int_parameter(line, "wtime");
				int b_time = parse_int_parameter(line, "btime");
				int w_inc = parse_int_parameter(line, "winc");
				int b_inc = parse_int_parameter(line, "binc");
				int moves_togo = parse_int_parameter(line, "movestogo");
				if (moves_togo == 0) {
					int ESTIMATED_NO_MOVES = 40;
					moves_togo = ESTIMATED_NO_MOVES - move;
					moves_togo = moves_togo < 10 ? 10 : moves_togo;
				}
				// use more time at move 1 - 20
				int factor = 2 - min(max(10, move), 20) / 20;
				if (white_turn && w_time != 0) {
					search->max_think_time_ms = factor * ((w_time + (moves_togo - 1) * w_inc) / moves_togo) - 3;
				}
				if (!white_turn && b_time != 0) {
					search->max_think_time_ms = factor * ((b_time + (moves_togo - 1) * b_inc) / moves_togo) - 3;
				}
			}
			if (line.find("ponder") != string::npos) {
				search->ponder();
			}
			search_thread = new thread(&gunborg::Search::search_best_move, search, start_position, white_turn, history, tt);
		}
		if (line.find("ponderhit") != string::npos) {
			search->ponder_hit();
		}
		if (line.find("stop") != string::npos) {
			if (search_thread != NULL) {
				search->should_run = false;
				search_thread->join();
				delete search_thread;
				search_thread = NULL;
			}
		}
		if (line.find("quit") != string::npos) {
			delete[] tt;
			return;
		}
		// non uci commands
		if (line.find("perft") != string::npos) {
			std::chrono::high_resolution_clock clock;
			std::chrono::high_resolution_clock::time_point start;

			int depth = parse_int_parameter(line, "depth");
			depth = depth == 0 ? 5 : depth;

			for (int i = 1; i <= depth; i++) {
				start = clock.now();
				std::cout << "perft depth(" << i << ") nodes: "
						<< perft(start_position, i, white_turn);
				int time_elapsed = std::chrono::duration_cast
						< std::chrono::milliseconds > (clock.now() - start).count();
				std::cout << " in " << time_elapsed << " ms\n";
			}
		}
		if (line.find("bench") != string::npos) {
			delete[] tt;
			tt = new Transposition[get_hash_table_size(DEFAULT_HASH_SIZE_MB)];
			history.clear();
			if (search != NULL) {
				delete search;
				search = NULL;
			}
			search = new gunborg::Search();
			search->should_run = true;
			search->max_depth = 10;
			search->max_think_time_ms = 60000;
			fen_info = parse_fen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");
			std::chrono::high_resolution_clock clock;
			std::chrono::high_resolution_clock::time_point start = clock.now();
			search->search_best_move(fen_info.position, fen_info.white_turn, history, tt);
			int time_elapsed = std::chrono::duration_cast
									< std::chrono::milliseconds > (clock.now() - start).count();
			std::cout << "bench " << search->node_count << " nodes in " << time_elapsed << " ms\n";
		}
		// license info
		if (line.find("show w") != string::npos) {
			std::cout << "This program is distributed in the hope that it will be useful,\n" <<
						"but WITHOUT ANY WARRANTY; without even the implied warranty of\n" <<
						"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the\n" <<
						"GNU General Public License for more details.\n";
		}
		if (line.find("show c") != string::npos) {
			std::cout << "This program is free software: you can redistribute it and/or modify\n" <<
						"it under the terms of the GNU General Public License as published by\n" <<
						"the Free Software Foundation, either version 3 of the License, or\n" <<
						"(at your option) any later version.\n";
		}
	}
}

