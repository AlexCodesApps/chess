#include "../src/include/chess.h"
#include "test.h"

void test_move_counts(void) {
	const int table[] = {
		20,
		400,
		8902,
		197281,
		4865609,
		119060324
	};
	ChessBoard board = INITIAL_CHESS_BOARD;
	for (int i = 0; i < 5; ++i) {
		int depth = i + 1;
		usize count = board_count_moves(&board, depth);
		if (count == table[i]) {
			ASSERT_OK("Board move count at depth [%d] is %"SDL_PRIu64, depth, count);
		} else {
			ASSERT_FAIL("Expected move count at depth [%d] to equal %d, found %"SDL_PRIu64,
				depth, table[i], count);
		}
	}
}
