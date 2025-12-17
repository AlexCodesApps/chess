#include "../src/include/chess.h"
#include "../src/include/uci.h"

LegalBoardMoves refresh_moves(ChessBoard * board, LegalBoardMoves moves[static 64]) {
	LegalBoardMoves composite_moves = 0;
	for (u8 i = 0; i < 64; ++i) {
		BoardSlot * slot = &board->slots[i];
		if (slot->has_piece && slot->side == board->side) {
			LegalBoardMoves _moves = board_get_legal_moves_for_piece(board, i);
			moves[i] = _moves;
			composite_moves |= _moves;
		}
	}
	return composite_moves;
}

int main() {
	ChessBoard board = INITIAL_CHESS_BOARD;
	UciServer server;
	UciClient client;
	LegalBoardMoves moves[64];
	UciMoveRequestData req;
	const char * args[] = { "stockfish", NULL };
	refresh_moves(&board, moves);
	if (!uci_server_start(&server, args)) {
		return 1;
	}
	uci_client_init(&client);
	req = (UciMoveRequestData){
		.board = &board,
		.timeout_ms = 1000,
	};
	uci_client_request_move(&client, &req);
	for (;;) {
		UciClientPollResult poll = uci_poll_client(&client, &server);
		switch (poll) {
		case UCI_POLL_CLIENT_OOM:
			SDL_Log("OOM");
			goto finish;
		case UCI_POLL_CLIENT_INVALID_DATA:
			SDL_Log("Invalid data");
			goto finish;
		case UCI_POLL_CLIENT_QUIT:
			SDL_Log("Server quit");
			goto finish;
		case UCI_POLL_CLIENT_CONTINUE:
			break;
		case UCI_POLL_CLIENT_MOVE_RESPONSE: {
			LegalBoardMoves legal = moves[req.out_from];
			SDL_assert(legal_board_moves_contains_idx(legal, req.out_to));
			BoardMoveResult result = board_make_move(&board, req.out_from, req.out_to);
			if (result.promotion) {
				SDL_assert(req.out_did_promo);
				board.slots[req.out_to].piece = req.out_promo;
			}
			LegalBoardMoves composite_moves = refresh_moves(&board, moves);
			if (composite_moves == 0) {
				SDL_Log("CHECKMATE or STALEMATE");
				goto finish;
			}
			uci_client_request_move(&client, &req);
			break;
		}
		}
	}
finish:
	uci_client_free(&client);
	uci_server_close(&server);
}
