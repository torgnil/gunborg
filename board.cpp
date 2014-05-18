/*
 * board.cpp
 *
 *  Created on: Apr 21, 2014
 *      Author: tnn
 */
#include "board.h"

/*
 * TODO REMOVE!!
 *
 *
 */
Board to_board2(const Board_legacy& board) {
	Board b2;
	b2.b[WHITE][PAWN] = board.white_pawns;
	b2.b[WHITE][KNIGHT] = board.white_knights;
	b2.b[WHITE][BISHOP] = board.white_bishops;
	b2.b[WHITE][ROOK] = board.white_rooks;
	b2.b[WHITE][QUEEN] = board.white_queen;
	b2.b[WHITE][KING] = board.white_king;
	b2.b[BLACK][PAWN] = board.black_pawns;
	b2.b[BLACK][KNIGHT] = board.black_knights;
	b2.b[BLACK][BISHOP] = board.black_bishops;
	b2.b[BLACK][ROOK] = board.black_rooks;
	b2.b[BLACK][QUEEN] = board.black_queen;
	b2.b[BLACK][KING] = board.black_king;
	b2.meta_info_stack.push_back(board.meta_info);
	return b2;
}

Board_legacy to_board(const Board& b2) {
	Board_legacy b;
	b.white_pawns = b2.b[WHITE][PAWN];
	b.white_knights = b2.b[WHITE][KNIGHT];
	b.white_bishops = b2.b[WHITE][BISHOP];
	b.white_rooks = b2.b[WHITE][ROOK];
	b.white_queen = b2.b[WHITE][QUEEN];
	b.white_king = b2.b[WHITE][KING];
	b.black_pawns = b2.b[BLACK][PAWN];
	b.black_knights = b2.b[BLACK][KNIGHT];
	b.black_bishops = b2.b[BLACK][BISHOP];
	b.black_rooks = b2.b[BLACK][ROOK];
	b.black_queen = b2.b[BLACK][QUEEN];
	b.black_king = b2.b[BLACK][KING];
	b.meta_info = b2.meta_info_stack.back();
	return b;
}


