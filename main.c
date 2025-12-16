#include "src/include/str.h"
#include "src/include/texture.h"
#include "src/include/state.h"
#include "src/include/uci.h"

LegalBoardMoves gen_legal_board_moves_array(LegalBoardMoves moves[32], ChessBoard * board);

int main(int argc, char ** argv) {
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
		SDL_Log("%s", SDL_GetError());
		return 1;
	}
	SDL_Log("Initialized SDL subsystems");
	Display display;
	if (!display_open(&display)) {
		SDL_Log("%s", SDL_GetError());
		SDL_Quit();
		return 1;
	}
	SDL_Log("Opened display window");
	TextureCache cache;
	if (!texture_cache_init(&cache, display.renderer)) {
		SDL_Log("%s", SDL_GetError());
		display_close(&display);
		SDL_Quit();
		return 1;
	}
	SDL_Log("Loaded all textures");
	SDL_Event event;
	State state;
	state_init(&state, &display);
	bool running = true;
	UciServer server;
	const char * args[] = { "stockfish", NULL };
	if (!uci_server_start(&server, args)) {
		SDL_Log("%s", SDL_GetError());
		texture_cache_free(&cache);
		display_close(&display);
		SDL_Quit();
		return 1;
	}
	UciClient client;
	uci_client_init(&client);
	UciMoveRequestData move_req = {
		.board = state.board,
		.timeout_ms = 1000,
	};
	uci_client_request_move(&client, &move_req);
	SDL_Log("Entering main loop");
	while (running) {
		SDL_Event _event;
		Event event;
		while (SDL_PollEvent(&_event)) {
			switch (_event.type) {
			case SDL_EVENT_QUIT:
				event.type = QUIT_EVENT;
				break;
			case SDL_EVENT_MOUSE_MOTION:
				event.type = MOUSE_MOVE_EVENT;
				event.as.mouse_move.x = _event.motion.x;
				event.as.mouse_move.y = _event.motion.y;
				break;
			case SDL_EVENT_MOUSE_BUTTON_DOWN:
				event.type = MOUSE_DOWN_EVENT;
				break;
			case SDL_EVENT_MOUSE_BUTTON_UP:
				event.type = MOUSE_UP_EVENT;
				break;
			default:
				continue;
			}
			state_process_event(&state, &event);
		}
		StateUpdateResult result = state_update(&state);
		switch (result) {
		case STATE_UPDATE_QUIT:
			running = false;
			break;
		case STATE_UPDATE_CONTINUE:
			break;
		}
		switch (uci_poll_client(&client, &server)) {
		case UCI_POLL_CLIENT_MOVE_RESPONSE:
			u8 idx = board_lookup_idx_idx(&state.board, move_req.out_from);
			BoardMoveResult result = board_make_move(&state.board, move_req.out_from, &idx, move_req.out_to);
			if (result.promotion)
				state.board.slots[move_req.out_to].piece = move_req.out_promo;
			if (!gen_legal_board_moves_array(state.legal_moves, &state.board)) {
				SDL_Log("CHECKMATE");
				running = false;
				break;
			}
			move_req.board = state.board;
			uci_client_request_move(&client, &move_req);
			break;
		default:
			continue;
		}
		state_draw(&state, &cache, &display);
		display_flip(&display);
	}
	uci_client_free(&client);
	uci_server_close(&server);
	texture_cache_free(&cache);
	SDL_Log("Freed textures");
	display_close(&display);
	SDL_Log("Closed display");
	SDL_Quit();
	SDL_Log("Closed SDL Subsystems");
	return 0;
}
