#include "test.h"
#include "../src/include/chess.h"

bool chess_boards_equal(const ChessBoard * test, const ChessBoard * expected, bool report) {
#define ASSERT_EQ(c) \
if ((test)->c != (expected)->c) { \
	if (report) \
		ASSERT_FAIL("CHESSBOARD EQUALITY CHECK, test->" #c " != expected->" #c); \
	return false; \
}
	ASSERT_EQ(side);
	for (u8 i = 0; i < 2; ++i) {
		ASSERT_EQ(sides[i].king_idx);
		ASSERT_EQ(sides[i].ks_castle_ok);
		ASSERT_EQ(sides[i].qs_castle_ok);
	}
	ASSERT_EQ(full_moves);
	ASSERT_EQ(half_moves);
	ASSERT_EQ(opt_pawn);
	for (u8 i = 0; i < 64; ++i) {
		if (!test->slots[i].has_piece) {
			ASSERT_EQ(slots[i].has_piece);
			continue;
		}
		ASSERT_EQ(slots[i].side);
		ASSERT_EQ(slots[i].piece);
	}
#undef ASSERT_EQ
	return true;
}

void test_fen_parse_and_encode(void) {
	ChessBoard board;
	ASSERT(fen_parse_board("", &board, NULL) == FEN_PARSE_INVALID_INPUT,
		"Empty string must be invalid input");
	ASSERT(fen_parse_board("k7/8/8/8/7k/8/K7/8 b - - 0 0", &board, NULL) == FEN_PARSE_ILLEGAL_STATE,
		"Multiple kings in FEN string must be illegal state");
	const char * initial_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 0";
	ASSERT(fen_parse_board(initial_fen, &board, NULL) == FEN_PARSE_OK,
		"The default chess board FEN string must be ok");
	ASSERT(chess_boards_equal(&board, &INITIAL_CHESS_BOARD, true),
		"The parsed default chess board and the initial must be the same");
	StrBuilder builder = str_builder_new();
	OOM_CHECK(fen_encode_board(&builder, &INITIAL_CHESS_BOARD));
	OOM_CHECK(str_builder_ensure_null_term(&builder));
	ASSERT(SDL_strcmp(initial_fen, builder.data) == 0, "Initial board converted to FEN string must equal initial FEN string");
	str_builder_free(&builder);
}
