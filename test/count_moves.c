#include "../src/include/chess.h"
#include "test.h"

void test_move_counts(void) {
	const int table[] = {
		1,
		20,
		400,
		8902,
		197281,
		4865609,
		119060324
	};
	ChessBoard board = INITIAL_CHESS_BOARD;
	for (int depth = 0; depth < 6; ++depth) {
		usize count = board_count_moves(&board, depth);
		if (count == table[depth]) {
			ASSERT_OK("Board move count at depth [%d] is %"SDL_PRIu64, depth, count);
		} else {
			ASSERT_FAIL("Expected move count at depth [%d] to equal %d, found %"SDL_PRIu64,
				depth, table[depth], count);
		}
	}

	const char * position = "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8";
	FENParseResult parse = fen_parse_board(position, &board, NULL);
	if (parse != FEN_PARSE_OK) {
		ASSERT_FAIL("FAILED TO PARSE TEST POSITION [%s]", position);
	} else {
		const int table[] = {
			1,
			44,
			1486,
			62379,
			2103487,
			89941194
		};
		for (int depth = 1; depth < 6; ++depth) {
			usize count = board_count_moves(&board, depth);
			if (count == table[depth]) {
				ASSERT_OK("Test board: move count at depth [%d] is %"SDL_PRIu64, depth, count);
			} else {
				ASSERT_FAIL("Test board: Expected move count at depth [%d] to equal %d, found %"SDL_PRIu64,
					depth, table[depth], count);
			}
		}
	}
}
