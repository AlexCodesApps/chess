#include "../src/include/chess.h"

int main(void) {
	ChessBoard board = INITIAL_CHESS_BOARD;
	for (int i = 1; i < 7; ++i) {
		usize count = board_count_moves(&board, i);
		SDL_Log("board at [%d] = %lu possible moves", i, count);
	}
}
