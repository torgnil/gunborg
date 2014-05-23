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
#include <iostream>

int test_count = 0;

void assertEquals(const char* fail_message, uint64_t value, uint64_t expected) {
	test_count++;
	if (expected != value) {
		std::cout << "Assert failure msg: " << fail_message << " expected: " << expected << " got: " << value
				<< std::endl;
	}
}

void white_pawn_push() {
	Board board;
	board.meta_info_stack.push_back(0);
	board.b[WHITE][PAWN] = ROW_3;
	board.b[WHITE][KING] = A1;
	board.b[BLACK][KING] = H8;
	MoveList quites = get_moves(board, true);
	assertEquals("should be 11 moves", quites.size(), 11);
	assertEquals("first move from A3", from_square(quites.front().m), lsb_to_square(A3));
	assertEquals("first move to A4", to_square(quites.front().m), lsb_to_square(A4));
	assertEquals("last move from H3", from_square(quites.back().m), lsb_to_square(A1));
	assertEquals("last move to H4", to_square(quites.back().m), lsb_to_square(B2));
}

void white_pawn_two_square_push() {
	Board board;
	board.meta_info_stack.push_back(0);
	board.b[WHITE][PAWN] = ROW_2;
	board.b[WHITE][KING] = A1;
	board.b[BLACK][KING] = H8;
	MoveList quites = get_moves(board, true);
	assertEquals("should be 17 moves", quites.size(), 17);
	assertEquals("first move from A2", from_square(quites.front().m), lsb_to_square(A2));
	assertEquals("first move to A3", to_square(quites.front().m), lsb_to_square(A3));
}

void black_pawn_two_square_push() {
	Board board;
	board.meta_info_stack.push_back(0);
	board.b[BLACK][PAWN] = ROW_7;
	board.b[WHITE][KING] = A1;
	board.b[BLACK][KING] = H8;
	MoveList quites = get_moves(board, false);
	assertEquals("should be 17 moves", quites.size(), 17);
	assertEquals("first move from A7", from_square(quites.front().m), lsb_to_square(A7));
	assertEquals("first move to A6", to_square(quites.front().m), lsb_to_square(A6));
}

void black_pawn_push() {
	Board board;
	board.meta_info_stack.push_back(0);
	board.b[BLACK][PAWN] = ROW_6;
	board.b[WHITE][KING] = A1;
	board.b[BLACK][KING] = H8;
	MoveList quites = get_moves(board, false);
	assertEquals("b should be 11 moves", quites.size(), 11);
	assertEquals("first move from A6", from_square(quites.front().m), lsb_to_square(A6));
	assertEquals("first move to A5", to_square(quites.front().m), lsb_to_square(A5));
}

void white_blocked_pawn_push() {
	Board board;
	board.meta_info_stack.push_back(0);
	board.b[WHITE][PAWN] = ROW_3;
	board.b[BLACK][PAWN] = D4;
	board.b[WHITE][KING] = A1;
	board.b[BLACK][KING] = H8;
	MoveList quites = get_moves(board, true);
	assertEquals("should be 12 moves", quites.size(), 12);
	assertEquals("first move from A3", from_square(quites.front().m), lsb_to_square(C3));
	assertEquals("first move to A4", to_square(quites.front().m), lsb_to_square(D4));
}

void pawn_captures() {
	Board board;
	board.meta_info_stack.push_back(0);
	board.b[WHITE][PAWN] = D5;
	board.b[BLACK][PAWN] = E6;
	board.b[WHITE][KING] = A1;
	board.b[BLACK][KING] = H8;
	MoveList children = get_moves(board, true);
	assertEquals("should be 5 moves", children.size(), 5);
}

void make_unmake() {
	Board board;
	board.b[WHITE][PAWN] = A4;
	board.meta_info_stack.push_back(0);
	Move move;
	move.m = to_move(lsb_to_square(A4), lsb_to_square(A5), WHITE, PAWN, EMPTY);

	make_move(board, move);
	assertEquals("Pawn at A5", board.b[WHITE][PAWN], A5);
	assertEquals("make hash", board.hash_key, move.m);
	unmake_move(board, move);
	assertEquals("Pawn unmaked to A4", board.b[WHITE][PAWN], A4);
	assertEquals("unmake hash", board.hash_key, 0);
}

void make_unmake_capture() {
	Board board;
	board.b[WHITE][PAWN] = A4;
	board.b[BLACK][PAWN] = B5;
	board.meta_info_stack.push_back(0);

	Move move;
	move.m = to_capture_move(lsb_to_square(A4), lsb_to_square(B5), PAWN, PAWN, WHITE, EMPTY);

	make_move(board, move);
	assertEquals("Pawn at B5", board.b[WHITE][PAWN], B5);
	assertEquals("Pawn is captured", board.b[BLACK][PAWN], 0);
	unmake_move(board, move);
	assertEquals("Pawn unmaked to A4", board.b[WHITE][PAWN], A4);
	assertEquals("Captured pawn unmaked to B5", board.b[BLACK][PAWN], B5);

}

void make_unmake_king_capture() {
	Board board;
	board.b[WHITE][PAWN] = A4;
	board.b[BLACK][KING] = B5;
	board.meta_info_stack.push_back(0);

	Move move;
	move.m = to_capture_move(lsb_to_square(A4), lsb_to_square(B5), PAWN, KING, WHITE, EMPTY);

	make_move(board, move);
	assertEquals("Pawn at B5", board.b[WHITE][PAWN], B5);
	assertEquals("Pawn is captured", board.b[BLACK][KING], 0);
	unmake_move(board, move);
	assertEquals("Pawn unmaked to A4", board.b[WHITE][PAWN], A4);
	assertEquals("Captured king unmaked to B5", board.b[BLACK][KING], B5);

}

void white_knight_moves() {
	Board board;
	board.meta_info_stack.push_back(0);
	board.b[WHITE][KING] = A1;
	board.b[BLACK][KING] = H8;
	board.b[WHITE][KNIGHT] = D4;
	MoveList quites = get_moves(board, true);
	assertEquals("should be 11 knight moves", quites.size(), 11);

	board.b[WHITE][KNIGHT] = A1;
	quites = get_moves(board, true);
	assertEquals("should be 5 knight moves", quites.size(), 5);

	board.b[WHITE][KNIGHT] = D4;
	board.b[WHITE][PAWN] = C6 | B5;
	quites = get_moves(board, true);
	assertEquals("should be 6 knight + 2 pawn moves + 3 king", quites.size(), 11);

	board.b[WHITE][KNIGHT] = D4;
	board.b[WHITE][PAWN] = 0;
	board.b[BLACK][PAWN] = C6 | B5;
	quites = get_moves(board, true);
	assertEquals("should be 11 knight moves", quites.size(), 11);

}

void start_moves() {
	Board board = start_pos().board;

	MoveList children = get_moves(board, true);
	assertEquals("should be 20 white start moves", children.size(), 20);
	children = get_moves(board, false);
	assertEquals("should be 20 black start moves", children.size(), 20);

}

void white_castling() {
	Board board = start_pos().board;
	board.b[WHITE][QUEEN] = 0;
	board.b[WHITE][KNIGHT] = 0;
	board.b[WHITE][BISHOP] = 0;
	board.b[WHITE][ROOK] = H1;

	board.meta_info_stack.push_back(G1);
	MoveList children = get_moves(board, true);
	assertEquals("should be 21 white start moves", children.size(), 21);
	Move castle_move = children.front();
	assertEquals("from square", from_square(castle_move.m), 4);
	assertEquals("to square", to_square(castle_move.m), 6);
	assertEquals("piece is king", piece(castle_move.m), KING);
	assertEquals("is not a capture", is_capture(castle_move.m), 0);
	assertEquals("is not a promotion", promotion_piece(castle_move.m), EMPTY);
	make_move(board, castle_move);
	assertEquals("castle rights removed", board.meta_info_stack.back(), 0);
	assertEquals("king square", board.b[WHITE][KING], G1);
	assertEquals("rook square", board.b[WHITE][ROOK], F1);
	unmake_move(board, castle_move);
	assertEquals("king is back", board.b[WHITE][KING], E1);
	assertEquals("rook is back", board.b[WHITE][ROOK], H1);
	assertEquals("castle rights are given back", board.meta_info_stack.back(), G1);
}

void white_en_passant_capture() {
	Board board;
	board.b[WHITE][PAWN] = E5;
	board.b[BLACK][PAWN] = D5;
	board.meta_info_stack.push_back(D6);
	board.b[WHITE][KING] = A1;
	board.b[BLACK][KING] = H8;

	MoveList children = get_moves(board, true);
	assertEquals("Expect five moves", children.size(), 5);
	for (auto it : children) {
		if (is_capture(it.m)) {
			assertEquals("capture on e.p square", to_square(it.m), 43);
			assertEquals("pawn mvvlva", it.sort_score, 1000006);
			make_move(board, it);
			assertEquals("black pawn is captured", board.b[BLACK][PAWN], 0);
			unmake_move(board, it);
			assertEquals("black pawn is back", board.b[BLACK][PAWN], D5);
		}
	}
}

void black_en_passant_capture() {
	Board board;
	board.b[WHITE][PAWN] = E4;
	board.b[BLACK][PAWN] = D4;
	board.meta_info_stack.push_back(E3);
	board.b[WHITE][KING] = A1;
	board.b[BLACK][KING] = H8;

	MoveList children = get_moves(board, false);
	assertEquals("Expect five moves", children.size(), 5);
	for (auto it : children) {
		if (is_capture(it.m)) {
			assertEquals("capture on e.p square", to_square(it.m), 20);
			assertEquals("pawn mvvlva", it.sort_score, 1000006);
			make_move(board, it);
			assertEquals("white pawn is captured", board.b[WHITE][PAWN], 0);
			unmake_move(board, it);
			assertEquals("white pawn is back", board.b[WHITE][PAWN], E4);
		}
	}
}

void forced_move() {
	FenInfo fen_info = parse_fen("6k1/pp3pp1/4p2p/8/3P3P/3R2P1/q1K5/4R3 w - - 2 37");
	Board board = fen_info.board;
	MoveList children = get_moves(board, fen_info.white_turn);

	bool in_check = get_attacked_squares(board, !fen_info.white_turn);
	assertEquals("in check", in_check, true);
	int legal_moves = 0;
	for (auto it  = children.begin(); it != children.end(); ++it) {
		make_move(board, *it);
		if (!(get_attacked_squares(board, !fen_info.white_turn) & board.b[WHITE][KING])) {
			legal_moves++;
		}
		unmake_move(board, *it);
	}
	assertEquals("three legal moves", legal_moves, 3);
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

	forced_move();

	std::cout << test_count << " tests executed" << std::endl;
}

