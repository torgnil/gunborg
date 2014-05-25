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

using namespace std;


FenInfo start_pos() {
	return parse_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
}

FenInfo parse_fen(string fen) {

	Board board;
	uint64_t meta_info = 0;
	int row = 8;
	int file = 1;
	for (int i = 0; !(row == 1 && file == 9); i++) {
		char c = fen[i];
		int j = (row - 1) * 8 + file - 1;
		if (c == 'r') {
			board.b[BLACK][ROOK] += (1ULL << j);
		}
		if (c == 'n') {
			board.b[BLACK][KNIGHT] += (1ULL << j);
		}
		if (c == 'b') {
			board.b[BLACK][BISHOP] += (1ULL << j);
		}
		if (c == 'q') {
			board.b[BLACK][QUEEN] += (1ULL << j);
		}
		if (c == 'k') {
			board.b[BLACK][KING] += (1ULL << j);
		}
		if (c == 'p') {
			board.b[BLACK][PAWN] += (1ULL << j);
		}
		if (c == 'R') {
			board.b[WHITE][ROOK] += (1ULL << j);
		}
		if (c == 'N') {
			board.b[WHITE][KNIGHT] += (1ULL << j);
		}
		if (c == 'B') {
			board.b[WHITE][BISHOP] += (1ULL << j);
		}
		if (c == 'Q') {
			board.b[WHITE][QUEEN] += (1ULL << j);
		}
		if (c == 'K') {
			board.b[WHITE][KING] += (1ULL << j);
		}
		if (c == 'P') {
			board.b[WHITE][PAWN] += (1ULL << j);
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

	board.meta_info_stack.push_back(meta_info);

	FenInfo fen_info;
	fen_info.board = board;
	if (fen.find("w") != string::npos) {
		fen_info.white_turn = true;
	} else {
		fen_info.white_turn = false;
	}

	fen_info.move = atoi(fen_strs[5].c_str());

	return fen_info;
}

void update_with_move(Board& board, string move_str, bool white_turn) {
	int from_file = move_str[0] - 'a' + 1;
	int from_row = move_str[1] - '0';
	int from = (from_row - 1) * 8 + from_file - 1;

	int to_file = move_str[2] - 'a' + 1;
	int to_row = move_str[3] - '0';
	int to = (to_row - 1) * 8 + to_file - 1;

	uint64_t black_squares = board.b[BLACK][KING] | board.b[BLACK][PAWN] | board.b[BLACK][KNIGHT]
			| board.b[BLACK][BISHOP] | board.b[BLACK][ROOK] | board.b[BLACK][QUEEN];

	uint64_t white_squares = board.b[WHITE][KING] | board.b[WHITE][PAWN] | board.b[WHITE][KNIGHT]
			| board.b[WHITE][BISHOP] | board.b[WHITE][ROOK] | board.b[WHITE][QUEEN];

	uint64_t occupied_squares = black_squares | white_squares;

	uint64_t from_square = 1ULL << from;
	uint64_t to_square = 1ULL << to;

	uint64_t meta_info = board.meta_info_stack.back();

	Move move;
	int promotion = EMPTY;
	int piece = piece_at_square(board, from, white_turn ? WHITE : BLACK);
	int color = white_turn ? WHITE : BLACK;
	if (piece == PAWN && ((to_square & ROW_8) || (to_square & ROW_1))) {
		// TODO under promotion
		promotion = QUEEN;
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
		int captured_piece = piece_at_square(board, to, captured_color);
		move.m = to_capture_move(from, to, piece, captured_piece, color, promotion);
	} else {
		move.m = to_move(from, to, piece, color, promotion);
	}

	make_move(board, move);
}

void uci() {
	thread* search_thread = NULL;

	FenInfo fen_info =  start_pos();
	Board start_board = fen_info.board;
	bool white_turn = fen_info.white_turn;
	int move = fen_info.move;

	gunborg::Search* search = NULL;
	list history;
	int hash_size = 4 * HASH_MB_FACTOR;
	while (true) {
		string line;
		getline(cin, line);
		if (line.find("uci") != string::npos) {
			cout << "id name gunborg 0.42\n";
			cout << "id author Torbjorn Nilsson\n";
			cout << "option name Hash type spin default 4 min 1 max 32\n";
			cout << "uciok\n" << flush;
		}
		if (line.find("isready") != string::npos) {
			cout << "readyok\n" << flush;
		}
		if (line.find("ucinewgame") != string::npos) {
			// new game
			FenInfo fen_info = start_pos();
			start_board = fen_info.board;
			white_turn = fen_info.white_turn;
			move = fen_info.move;
		}
		if (line.find("setoption name Hash") != string::npos) {
			int hash_size_in_mb = parse_int_parameter(line, "value");
			if (hash_size_in_mb >= 1 && hash_size_in_mb <= 32) {
				hash_size = hash_size_in_mb * HASH_MB_FACTOR;
			}

		}
		if (line.find("position") != string::npos) {
			history.clear();
			// parse position
			// position [fen <fenstring> | startpos ]  moves <move1> .... <movei>
			if (line.find("startpos") != string::npos) {
				FenInfo fen_info = start_pos();
				start_board = fen_info.board;
				white_turn = fen_info.white_turn;
				move = fen_info.move;
			}
			string::size_type pos = line.find("fen");
			if (pos != string::npos) {
				string fen = line.substr(pos + 4);
				FenInfo fen_info = parse_fen(fen);
				start_board = fen_info.board;
				white_turn = fen_info.white_turn;
				move = fen_info.move;
			}
			string::size_type moves_pos = line.find("moves");
			if (moves_pos != string::npos) {
				string moves_str = line.substr(moves_pos + 6);
				vector<string> splited = split(moves_str);
				move = splited.size() / 2;
				for (auto it : splited) {
					Board copy = start_board;
					history.push_back(copy);

					update_with_move(start_board, it, white_turn);

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
			search->hash_size = hash_size;
			if (line.find("infinite") != string::npos) {
				search->max_think_time_ms = INT_MAX;
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
				// use more time at the beginning
				int factor = 2;
				if (white_turn && w_time != 0) {
					int time_left = w_time + (moves_togo - 1) * w_inc;
					int think_time = factor * time_left/ moves_togo;
					if (think_time > time_left) {
						think_time = time_left;
					}
					search->max_think_time_ms = think_time - 5;
				}
				if (!white_turn && b_time != 0) {
					int time_left = b_time + (moves_togo - 1) * b_inc;
					int think_time = factor * time_left/ moves_togo;
					if (think_time > time_left) {
						think_time = time_left;
					}
					search->max_think_time_ms = think_time - 5;
				}
			}
			search_thread = new thread(&gunborg::Search::search_best_move, search, start_board, white_turn, history);
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
			return;
		}
		if (line.find("perft") != string::npos) {

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

