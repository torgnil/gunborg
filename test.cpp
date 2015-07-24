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
 * test.cpp
 *
 *  Created on: Apr 21, 2014
 *      Author: Torbjörn Nilsson
 */
#include "test.h"
#include "board.h"
#include "moves.h"
#include "uci.h"
#include "util.h"
#include <iostream>

int test_count = 0;

void assert_equals(const char* fail_message, uint64_t value, uint64_t expected) {
	test_count++;
	if (expected != value) {
		std::cout << "Assert failure msg: " << fail_message << " expected: " << expected << " got: " << value
				<< std::endl;
	}
}

void white_pawn_push() {
	Position position;
	position.meta_info_stack.push_back(0);
	position.p[WHITE][PAWN] = ROW_3;
	position.p[WHITE][KING] = A1;
	position.p[BLACK][KING] = H8;
	MoveList quites = get_moves(position, true);
	assert_equals("should be 11 moves", quites.size(), 11);
	assert_equals("first move from A3", from_square(quites.front().m), lsb_to_square(A3));
	assert_equals("first move to A4", to_square(quites.front().m), lsb_to_square(A4));
	assert_equals("last move from H3", from_square(quites.back().m), lsb_to_square(A1));
	assert_equals("last move to H4", to_square(quites.back().m), lsb_to_square(B2));
}

void white_pawn_two_square_push() {
	Position position;
	position.meta_info_stack.push_back(0);
	position.p[WHITE][PAWN] = ROW_2;
	position.p[WHITE][KING] = A1;
	position.p[BLACK][KING] = H8;
	MoveList quites = get_moves(position, true);
	assert_equals("should be 17 moves", quites.size(), 17);
	assert_equals("first move from A2", from_square(quites.front().m), lsb_to_square(A2));
	assert_equals("first move to A3", to_square(quites.front().m), lsb_to_square(A3));
}

void black_pawn_two_square_push() {
	Position position;
	position.meta_info_stack.push_back(0);
	position.p[BLACK][PAWN] = ROW_7;
	position.p[WHITE][KING] = A1;
	position.p[BLACK][KING] = H8;
	MoveList quites = get_moves(position, false);
	assert_equals("should be 17 moves", quites.size(), 17);
	assert_equals("first move from A7", from_square(quites.front().m), lsb_to_square(A7));
	assert_equals("first move to A6", to_square(quites.front().m), lsb_to_square(A6));
}

void black_pawn_push() {
	Position position;
	position.meta_info_stack.push_back(0);
	position.p[BLACK][PAWN] = ROW_6;
	position.p[WHITE][KING] = A1;
	position.p[BLACK][KING] = H8;
	MoveList quites = get_moves(position, false);
	assert_equals("b should be 11 moves", quites.size(), 11);
	assert_equals("first move from A6", from_square(quites.front().m), lsb_to_square(A6));
	assert_equals("first move to A5", to_square(quites.front().m), lsb_to_square(A5));
}

void white_blocked_pawn_push() {
	Position position;
	position.meta_info_stack.push_back(0);
	position.p[WHITE][PAWN] = ROW_3;
	position.p[BLACK][PAWN] = D4;
	position.p[WHITE][KING] = A1;
	position.p[BLACK][KING] = H8;
	MoveList quites = get_moves(position, true);
	assert_equals("should be 12 moves", quites.size(), 12);
	assert_equals("first move from A3", from_square(quites.front().m), lsb_to_square(C3));
	assert_equals("first move to A4", to_square(quites.front().m), lsb_to_square(D4));
}

void pawn_captures() {
	Position position;
	position.meta_info_stack.push_back(0);
	position.p[WHITE][PAWN] = D5;
	position.p[BLACK][PAWN] = E6;
	position.p[WHITE][KING] = A1;
	position.p[BLACK][KING] = H8;
	MoveList moves = get_moves(position, true);
	assert_equals("should be 5 moves", moves.size(), 5);
}

void make_unmake() {
	Position position;
	position.p[WHITE][PAWN] = A4;
	position.meta_info_stack.push_back(0);
	Move move;
	move.m = to_move(lsb_to_square(A4), lsb_to_square(A5), WHITE, PAWN, EMPTY);

	make_move(position, move);
	assert_equals("Pawn at A5", position.p[WHITE][PAWN], A5);
	assert_equals("make hash", position.hash_key, move_hash(move.m));
	unmake_move(position, move);
	assert_equals("Pawn unmaked to A4", position.p[WHITE][PAWN], A4);
	assert_equals("unmake hash", position.hash_key, 0);
}

void make_unmake_capture() {
	Position position;
	position.p[WHITE][PAWN] = A4;
	position.p[BLACK][PAWN] = B5;
	position.meta_info_stack.push_back(0);

	Move move;
	move.m = to_capture_move(lsb_to_square(A4), lsb_to_square(B5), PAWN, PAWN, WHITE, EMPTY);

	make_move(position, move);
	assert_equals("Pawn at B5", position.p[WHITE][PAWN], B5);
	assert_equals("Pawn is captured", position.p[BLACK][PAWN], 0);
	unmake_move(position, move);
	assert_equals("Pawn unmaked to A4", position.p[WHITE][PAWN], A4);
	assert_equals("Captured pawn unmaked to B5", position.p[BLACK][PAWN], B5);

}

void make_unmake_king_capture() {
	Position position;
	position.p[WHITE][PAWN] = A4;
	position.p[BLACK][KING] = B5;
	position.meta_info_stack.push_back(0);

	Move move;
	move.m = to_capture_move(lsb_to_square(A4), lsb_to_square(B5), PAWN, KING, WHITE, EMPTY);

	make_move(position, move);
	assert_equals("Pawn at B5", position.p[WHITE][PAWN], B5);
	assert_equals("Pawn is captured", position.p[BLACK][KING], 0);
	unmake_move(position, move);
	assert_equals("Pawn unmaked to A4", position.p[WHITE][PAWN], A4);
	assert_equals("Captured king unmaked to B5", position.p[BLACK][KING], B5);

}

void white_knight_moves() {
	Position position;
	position.meta_info_stack.push_back(0);
	position.p[WHITE][KING] = A1;
	position.p[BLACK][KING] = H8;
	position.p[WHITE][KNIGHT] = D4;
	MoveList quites = get_moves(position, true);
	assert_equals("should be 11 knight moves", quites.size(), 11);

	position.p[WHITE][KNIGHT] = A1;
	quites = get_moves(position, true);
	assert_equals("should be 5 knight moves", quites.size(), 5);

	position.p[WHITE][KNIGHT] = D4;
	position.p[WHITE][PAWN] = C6 | B5;
	quites = get_moves(position, true);
	assert_equals("should be 6 knight + 2 pawn moves + 3 king", quites.size(), 11);

	position.p[WHITE][KNIGHT] = D4;
	position.p[WHITE][PAWN] = 0;
	position.p[BLACK][PAWN] = C6 | B5;
	quites = get_moves(position, true);
	assert_equals("should be 11 knight moves", quites.size(), 11);

}

void start_moves() {
	Position position = start_pos().position;

	MoveList moves = get_moves(position, true);
	assert_equals("should be 20 white start moves", moves.size(), 20);
	moves = get_moves(position, false);
	assert_equals("should be 20 black start moves", moves.size(), 20);

}

void white_castling() {
	Position position = start_pos().position;
	position.p[WHITE][QUEEN] = 0;
	position.p[WHITE][KNIGHT] = 0;
	position.p[WHITE][BISHOP] = 0;
	position.p[WHITE][ROOK] = H1;

	position.meta_info_stack.push_back(G1);
	MoveList moves = get_moves(position, true);
	assert_equals("should be 21 white start moves", moves.size(), 21);
	Move castle_move = moves.front();
	assert_equals("from square", from_square(castle_move.m), 4);
	assert_equals("to square", to_square(castle_move.m), 6);
	assert_equals("piece is king", piece(castle_move.m), KING);
	assert_equals("is not a capture", is_capture(castle_move.m), 0);
	assert_equals("is not a promotion", promotion_piece(castle_move.m), EMPTY);
	make_move(position, castle_move);
	assert_equals("castle rights removed", position.meta_info_stack.back(), 0);
	assert_equals("king square", position.p[WHITE][KING], G1);
	assert_equals("rook square", position.p[WHITE][ROOK], F1);
	unmake_move(position, castle_move);
	assert_equals("king is back", position.p[WHITE][KING], E1);
	assert_equals("rook is back", position.p[WHITE][ROOK], H1);
	assert_equals("castle rights are given back", position.meta_info_stack.back(), G1);
}

void white_en_passant_capture() {
	Position position;
	position.p[WHITE][PAWN] = E5;
	position.p[BLACK][PAWN] = D5;
	position.meta_info_stack.push_back(D6);
	position.p[WHITE][KING] = A1;
	position.p[BLACK][KING] = H8;

	MoveList moves = get_moves(position, true);
	assert_equals("Expect five moves", moves.size(), 5);
	for (auto it : moves) {
		if (is_capture(it.m)) {
			assert_equals("capture on e.p square", to_square(it.m), 43);
			assert_equals("pawn mvvlva", it.sort_score, 1000000);
			make_move(position, it);
			assert_equals("black pawn is captured", position.p[BLACK][PAWN], 0);
			unmake_move(position, it);
			assert_equals("black pawn is back", position.p[BLACK][PAWN], D5);
		}
	}
}

void black_en_passant_capture() {
	Position position;
	position.p[WHITE][PAWN] = E4;
	position.p[BLACK][PAWN] = D4;
	position.meta_info_stack.push_back(E3);
	position.p[WHITE][KING] = A1;
	position.p[BLACK][KING] = H8;

	MoveList moves = get_moves(position, false);
	assert_equals("Expect five moves", moves.size(), 5);
	for (auto it : moves) {
		if (is_capture(it.m)) {
			assert_equals("capture on e.p square", to_square(it.m), 20);
			assert_equals("pawn mvvlva", it.sort_score, 1000000);
			make_move(position, it);
			assert_equals("white pawn is captured", position.p[WHITE][PAWN], 0);
			unmake_move(position, it);
			assert_equals("white pawn is back", position.p[WHITE][PAWN], E4);
		}
	}
}

void forced_move() {
	FenInfo fen_info = parse_fen("6k1/pp3pp1/4p2p/8/3P3P/3R2P1/q1K5/4R3 w - - 2 37");
	Position position = fen_info.position;
	MoveList moves = get_moves(position, fen_info.white_turn);

	bool in_check = get_attacked_squares(position, !fen_info.white_turn);
	assert_equals("in check", in_check, true);
	int legal_moves = 0;
	for (auto it  = moves.begin(); it != moves.end(); ++it) {
		make_move(position, *it);
		if (!(get_attacked_squares(position, !fen_info.white_turn) & position.p[WHITE][KING])) {
			legal_moves++;
		}
		unmake_move(position, *it);
	}
	assert_equals("three legal moves", legal_moves, 3);
}

void perft_test()  {
	FenInfo fen_info = parse_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"); // startpos
	Position position = fen_info.position;
	assert_equals("depth 1", perft(position, 1, fen_info.white_turn), 20);
	assert_equals("depth 2", perft(position, 2, fen_info.white_turn), 400);
	assert_equals("depth 3", perft(position, 3, fen_info.white_turn), 8902);
	assert_equals("depth 4", perft(position, 4, fen_info.white_turn), 197281);
	assert_equals("depth 5", perft(position, 5, fen_info.white_turn), 4865609);

	fen_info = parse_fen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -"); // "kiwipete" by Peter McKenzie
	position = fen_info.position;
	assert_equals("depth 1", perft(position, 1, fen_info.white_turn), 48);
	assert_equals("depth 2", perft(position, 2, fen_info.white_turn), 2039);
	assert_equals("depth 3", perft(position, 3, fen_info.white_turn), 97862);
	assert_equals("depth 4", perft(position, 4, fen_info.white_turn), 4085603);

	fen_info = parse_fen("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
	position = fen_info.position;
	assert_equals("depth 1", perft(position, 1, fen_info.white_turn), 6);
	assert_equals("depth 2", perft(position, 2, fen_info.white_turn), 264);
	assert_equals("depth 3", perft(position, 3, fen_info.white_turn), 9467);
	assert_equals("depth 4", perft(position, 4, fen_info.white_turn), 422333);
}

void fen_en_passant() {
	FenInfo fen_info = parse_fen("rnbqkbnr/ppppp1pp/8/8/4Pp2/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
	Position position = fen_info.position;
	MoveList moves = get_captures(position, fen_info.white_turn);
	assert_equals("one en passant capture", moves.size(), 1);
}

void run_tests() {
	init();

	white_pawn_push();
	white_blocked_pawn_push();
	black_pawn_push();
	white_pawn_two_square_push();
	black_pawn_two_square_push();

	pawn_captures();
	make_unmake();
	make_unmake_capture();
	make_unmake_king_capture();
	white_knight_moves();
	start_moves();
	white_castling();

	white_en_passant_capture();
	black_en_passant_capture();
	fen_en_passant();

	perft_test();

	forced_move();

	std::cout << test_count << " tests executed" << std::endl;
}

