#include "include/state.h"
#include "include/str.h"

#define SLOT_HEIGHT (SCREEN_WIDTH / 12.0)
#define SLOT_WIDTH (SCREEN_WIDTH * 0.5)
#define SLOT_X (SCREEN_WIDTH * 0.25)

#define MENU_TITLE_SLOT 3
#define PLAY_BUTTON_SLOT 6
#define ABOUT_BUTTON_SLOT 7
#define BACK_BUTTON_SLOT 7
#define QUIT_BUTTON_SLOT 8
#define START_BUTTON_SLOT 10

#define MENU_RECT \
	((Rect2f){SCREEN_WIDTH * 0.25, SCREEN_WIDTH * 0.25, SCREEN_WIDTH * 0.5, \
		SCREEN_WIDTH * 0.5})

#define BOARD_RECT ((Rect2f){ SCREEN_WIDTH * 0.1, SCREEN_WIDTH * 0.1, SCREEN_WIDTH * 0.8, SCREEN_WIDTH * 0.8 })
#define BOARD_SLOT_WIDTH ((SCREEN_WIDTH * 0.8) / 8.0)

#define DIALOG_BOX_RECT \
	((Rect2f){ \
		BOARD_RECT.x, \
		BOARD_RECT.y, \
		BOARD_SLOT_WIDTH * 4 + 5, \
		BOARD_SLOT_WIDTH + 2, \
	})
#define DIALOG_KNIGHT_RECT \
	((Rect2f){ \
		(SCREEN_WIDTH * 0.1) + 1, (SCREEN_WIDTH * 0.1) + 1, BOARD_SLOT_WIDTH, BOARD_SLOT_WIDTH \
	})
#define DIALOG_BISHOP_RECT \
	((Rect2f){ \
		(SCREEN_WIDTH * 0.1) + BOARD_SLOT_WIDTH + 2, \
		(SCREEN_WIDTH * 0.1) + 1, \
		BOARD_SLOT_WIDTH, \
		BOARD_SLOT_WIDTH \
	})
#define DIALOG_ROOK_RECT \
	((Rect2f) { \
		(SCREEN_WIDTH * 0.1) + BOARD_SLOT_WIDTH * 2 + 3, \
		(SCREEN_WIDTH * 0.1) + 1, \
		BOARD_SLOT_WIDTH, \
		BOARD_SLOT_WIDTH, \
	})
#define DIALOG_QUEEN_RECT \
	((Rect2f) { \
		(SCREEN_WIDTH * 0.1) + BOARD_SLOT_WIDTH * 3 + 4, \
		(SCREEN_WIDTH * 0.1) + 1, \
		BOARD_SLOT_WIDTH, \
		BOARD_SLOT_WIDTH, \
	})

static inline Rect2f get_slot_rect(int slot) {
	return rect2f_new(SLOT_X, SLOT_HEIGHT * slot, SLOT_WIDTH, SLOT_HEIGHT);
}

static LegalBoardMoves refresh_moves(ChessBoard * board, LegalBoardMoves moves[64]) {
	LegalBoardMoves composite_moves = 0;
	SDL_memset(moves, 0, sizeof(*moves) * 64);
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

void state_init(State * state, const Display * display) {
	SDL_zerop(state);
	state->stage = STATE_STAGE_TITLE;
	state->bg_scale = (f32)SCREEN_WIDTH / (8 * 16 * 2);
	state->bg_rect = rect2f_new(0, 0, 8 * 8, 8 * 8);
	state->bg_speed = -50;
	state->bg_direction = 0;
	state->mouse_down = false;
	state->mouse_press = false;
	state->board_rotate_slider = (Slider) {
		.state = SLIDER_ON,
	};
	state->p2_slider = (Slider){
		.state = SLIDER_ON,
	};
	state->p1_slider = (Slider) {
		.state = SLIDER_ON,
	};
}

static bool mouse_in_rect(State * state, Rect2f rect) {
	return SDL_PointInRectFloat(&state->mouse_pos, &rect);
}

void state_process_event(State * state, const Event * event) {
	switch (event->type) {
	case QUIT_EVENT:
		state->should_quit = true;
		break;
	case MOUSE_DOWN_EVENT:
		state->mouse_down = true;
		state->mouse_press = true;
		break;
	case MOUSE_MOVE_EVENT:
		state->mouse_pos = event->as.mouse_move;
		break;
	case MOUSE_UP_EVENT:
		state->mouse_down = false;
		break;
	}
}

static bool state_is_menu(State * state) {
	const u32 bitmask =
		STATE_STAGE_TITLE |
		STATE_STAGE_ABOUT |
		STATE_STAGE_GAME_SETTINGS |
		STATE_STAGE_ERR_MSG;
	return (state->stage & bitmask) != 0;
}

static void state_stage_cleanup(State * state) {
	switch (state->stage) {
		case STATE_STAGE_GAME:
			player_free(&state->game.p1);
			player_free(&state->game.p2);
			break;
		case STATE_STAGE_TITLE:
		case STATE_STAGE_ABOUT:
		case STATE_STAGE_GAME_SETTINGS:
		case STATE_STAGE_ERR_MSG:
			break;
	}
}

static void state_show_err_msg(State * state, Str msg) {
	state_stage_cleanup(state);
	state->err_msg = msg;
	state->stage = STATE_STAGE_ERR_MSG;
}

static void state_start_game(State * state) {
	state->game.board = INITIAL_CHESS_BOARD;
	refresh_moves(&state->game.board, state->game.legal_moves);
	if (slider_status(&state->board_rotate_slider)) {
		state->board_view = WHITE_SIDE;
	} else {
		switch ((u32)slider_status(&state->p1_slider) << 1 | (u32)slider_status(&state->p2_slider)) {
			case 0b00:
			case 0b10:
			case 0b11:
				state->board_view = WHITE_SIDE;
				break;
			case 0b01:
				state->board_view = BLACK_SIDE;
				break;
		}
	}
	if (slider_status(&state->p1_slider)) { // human
		player_init_human(&state->game.p1);
	} else {
		UciServer * server = SDL_malloc(sizeof(*server));
		if (!server) {
			state_show_err_msg(state, S("Should not allocate memory for server"));
			return;
		}
		UciClient * client = SDL_malloc(sizeof(*client));
		if (!client) {
			SDL_free(server);
			state_show_err_msg(state, S("Could not allocate memory for client"));
			return;
		}
		const char * cmd[] = { "stockfish", NULL };
		if (!uci_server_start(server, cmd)) {
			SDL_free(server);
			SDL_free(client);
			state_show_err_msg(state, S("Stockfish not found"));
			return;
		}
		uci_client_init(client);
		player_init_bot(&state->game.p1, server, client);
	}
	if (slider_status(&state->p2_slider)) { // human
		player_init_human(&state->game.p2);
	} else {
		UciServer * server = SDL_malloc(sizeof(*server));
		if (!server) {
			state_show_err_msg(state, S("Should not allocate memory for server"));
			return;
		}
		UciClient * client = SDL_malloc(sizeof(*client));
		if (!client) {
			SDL_free(server);
			state_show_err_msg(state, S("Could not allocate memory for client"));
			return;
		}
		const char * cmd[] = { "stockfish", NULL };
		if (!uci_server_start(server, cmd)) {
			SDL_free(server);
			SDL_free(client);
			state_show_err_msg(state, S("Could not start UCI server"));
			return;
		}
		uci_client_init(client);
		player_init_bot(&state->game.p2, server, client);
	}
	state->game.state = GAME_STATE_IDLE;
	state->stage = STATE_STAGE_GAME;
}

static u8 state_mouse_board_idx(State * state) {
	Vec2f a = vec2f_sub(state->mouse_pos, rect2f_pos(BOARD_RECT));
	a = vec2f_div(a, vec2f_new(BOARD_SLOT_WIDTH, BOARD_SLOT_WIDTH));
	if (a.x < 0.0f || a.x >= 8.0 || a.y < 0.0 || a.y >= 8.0)
		return INVALID_PIECE_IDX;
	int x = (int)a.x;
	int y = (int)a.y;
	if (state->board_view == WHITE_SIDE) {
		x = 7 - x;
		y = 7 - y;
	}
	return (u8)(y * 8 + x);
}

static Player * state_current_player(State * state) {
	return state->game.board.side == WHITE_SIDE ? &state->game.p1 : &state->game.p2;
}

void state_game_next_turn(State * state) {
	LegalBoardMoves comp = refresh_moves(&state->game.board, state->game.legal_moves);
	if (comp == 0)
		state->game.state = GAME_STATE_FINISHED;
	if (slider_status(&state->board_rotate_slider)) {
		state->board_view ^= 1;
	}
	state->game.state = GAME_STATE_IDLE;
}

void state_game_make_move(State * state, u8 from, u8 to) {
	Player * p = state_current_player(state);
	BoardMoveResult result = board_make_move(&state->game.board, from, to);
	if (result.promotion) {
		u8 piece = player_request_promotion(state, p, to);
		if (piece == PROMOTION_REQUEST_ERROR) {
			state_show_err_msg(state, S("Client error"));
			return;
		}
		if (piece == PROMOTION_REQUEST_PENDING) {
			return;
		}
		state->game.board.slots[to].piece = piece; // TODO: validate?
	}
	state_game_next_turn(state);
}

static StateUpdateResult state_process_mouse_press(State * state, f32 elapsed_time) {
	if (state_is_menu(state)) {
		if (mouse_in_rect(state, get_slot_rect(QUIT_BUTTON_SLOT))) {
			return STATE_UPDATE_QUIT;
		}
	}
	switch (state->stage) {
	case STATE_STAGE_TITLE:
		if (mouse_in_rect(state, get_slot_rect(ABOUT_BUTTON_SLOT))) { /* ABOUT */
			state->stage = STATE_STAGE_ABOUT;
			state->bg_direction ^= 1;
		} else if (mouse_in_rect(state, get_slot_rect(PLAY_BUTTON_SLOT))) { /* PLAY */
			state->stage = STATE_STAGE_GAME_SETTINGS;
			state->bg_direction ^= 1;
		}
		break;
	case STATE_STAGE_ABOUT:
		if (mouse_in_rect(state, get_slot_rect(BACK_BUTTON_SLOT))) { /* BACK */
			state->stage = STATE_STAGE_TITLE;
			state->bg_direction ^= 1;
		}
		break;
	case STATE_STAGE_GAME_SETTINGS:
		if (mouse_in_rect(state, get_slot_rect(BACK_BUTTON_SLOT))) { /* BACK */
			SDL_zero(state->settings);
			state->stage = STATE_STAGE_TITLE;
			state->bg_direction ^= 1;
		}
		Rect2f rect = get_slot_rect(MENU_TITLE_SLOT + 3);
		Rect2f slider;
		partition_rect_horiz(rect, 0.5, NULL, &slider);
		if (mouse_in_rect(state, slider)) {
			slider_toggle(&state->board_rotate_slider, elapsed_time);
		}

		rect = get_slot_rect(MENU_TITLE_SLOT + 2);
		partition_rect_horiz(rect, 0.5, NULL, &slider);
		if (mouse_in_rect(state, slider)) {
			slider_toggle(&state->p2_slider, elapsed_time);
		}

		rect = get_slot_rect(MENU_TITLE_SLOT + 1);
		partition_rect_horiz(rect, 0.5, NULL, &slider);
		if (mouse_in_rect(state, slider)) {
			slider_toggle(&state->p1_slider, elapsed_time);
		}
		if (mouse_in_rect(state, get_slot_rect(START_BUTTON_SLOT))) {
			state_start_game(state);
		}
		break;
	case STATE_STAGE_GAME: {
		if (state->game.promotion_dialog) {
			ChessPiece piece;
			if (mouse_in_rect(state, DIALOG_KNIGHT_RECT)) {
				piece = CHESS_KNIGHT;
			} else if (mouse_in_rect(state, DIALOG_BISHOP_RECT)) {
				piece = CHESS_BISHOP;
			} else if (mouse_in_rect(state, DIALOG_ROOK_RECT)) {
				piece = CHESS_ROOK;
			} else if (mouse_in_rect(state, DIALOG_QUEEN_RECT)) {
				piece = CHESS_QUEEN;
			} else {
				return STATE_UPDATE_CONTINUE;
			}
			state->game.promotion_dialog = false;
			state->game.board.slots[state->game.promotion_idx].piece = piece; // TODO: validate?
			state_game_next_turn(state);
			return STATE_UPDATE_CONTINUE;
		}
		Player * p = state_current_player(state);
		if (p->type == PLAYER_HUMAN) {
			u8 idx = state_mouse_board_idx(state);
			if (idx != INVALID_PIECE_IDX) {
				SDL_Log("Picked up piece");
				BoardSlot * slot = &state->game.board.slots[idx];
				if (slot->has_piece && slot->side == state->game.board.side) {
					p->as.human.held_idx = idx;
					p->as.human.cursor_offset =
						vec2f_new(
							- state->mouse_pos.x + floor_to_nearest(state->mouse_pos.x, BOARD_SLOT_WIDTH),
							- state->mouse_pos.y + floor_to_nearest(state->mouse_pos.y, BOARD_SLOT_WIDTH));
				}
			}
		}
		break;
	}
	case STATE_STAGE_ERR_MSG:
		if (mouse_in_rect(state, get_slot_rect(BACK_BUTTON_SLOT))) {
			state->stage = STATE_STAGE_TITLE;
		}
		break;
	}
	return STATE_UPDATE_CONTINUE;
}

void player_init_human(Player * player) {
	player->type = PLAYER_HUMAN;
	player->as.human.held_idx = INVALID_PIECE_IDX;
}

void player_init_bot(Player * player, UciServer * server, UciClient * client) {
	player->type = PLAYER_BOT;
	player->as.bot.server = server;
	player->as.bot.client = client;
}

void player_free(Player * player) {
	switch (player->type) {
	case PLAYER_HUMAN:
		break;
	case PLAYER_BOT:
		uci_client_free(player->as.bot.client);
		uci_server_close(player->as.bot.server);
		SDL_free(player->as.bot.client);
		SDL_free(player->as.bot.server);
		break;
	}
}

/* request the player to make the next move */
void player_request_move(State * state, Player * player) {
	switch (player->type) {
	case PLAYER_HUMAN:
		break;
	case PLAYER_BOT:
		player->as.bot.req = (UciMoveRequestData) {
			.board = &state->game.board,
			.timeout_ms = 500,
		};
		uci_client_request_move(player->as.bot.client, &player->as.bot.req);
	}
}

u8 player_request_promotion(State * state, Player * player, u8 to) {
	switch (player->type) {
	case PLAYER_BOT:
		if (!player->as.bot.req.out_did_promo) {
			return PROMOTION_REQUEST_ERROR;
		}
		return player->as.bot.req.out_promo;
	case PLAYER_HUMAN:
		state->game.promotion_dialog = true;
		state->game.promotion_idx = to;
		return PROMOTION_REQUEST_PENDING;
	}
}

PlayerPollResult player_poll(State * state, Player * player) {
	PlayerPollResult ret = { .type = PLAYER_POLL_CONTINUE };
	switch (player->type) {
		case PLAYER_BOT: {
			UciClientPollResult poll = uci_poll_client(player->as.bot.client, player->as.bot.server);
			switch (poll) {
			case UCI_POLL_CLIENT_CONTINUE:
				break;
			case UCI_POLL_CLIENT_QUIT:
				ret.type = PLAYER_POLL_QUIT;
				break;
			case UCI_POLL_CLIENT_OOM:
			case UCI_POLL_CLIENT_INVALID_DATA:
				ret.type = PLAYER_POLL_ERROR;
				break;
			case UCI_POLL_CLIENT_MOVE_RESPONSE:
				ret.type = PLAYER_POLL_MOVED;
				ret.as.moved.from = player->as.bot.req.out_from;
				ret.as.moved.to = player->as.bot.req.out_to;
				break;
			}
			break;
		}
		case PLAYER_HUMAN: {
			if (player->as.human.held_idx != INVALID_PIECE_IDX && !state->mouse_down) { // release detected
				u8 from = player->as.human.held_idx;
				player->as.human.held_idx = INVALID_PIECE_IDX;
				u8 to = state_mouse_board_idx(state);
				if (to != INVALID_PIECE_IDX) {
					ret.type = PLAYER_POLL_MOVED;
					ret.as.moved.from = from;
					ret.as.moved.to = to;
				}
			}
		}
		break;
	}
	return ret;
}

void state_update_game(State * state, f32 elapsed_time) {
	if (state->game.promotion_dialog) {
		// TODO: implement
		return;
	}
	Player * p = state_current_player(state);
	switch (state->game.state) {
	case GAME_STATE_IDLE:
		player_request_move(state, p);
		state->game.state = GAME_STATE_POLLING;
		break;
	case GAME_STATE_POLLING: {
		PlayerPollResult poll = player_poll(state, p);
		switch (poll.type) {
			case PLAYER_POLL_CONTINUE:
				break;
			case PLAYER_POLL_ERROR:
				state_show_err_msg(state, S("Unexpected Client Error"));
				break;
			case PLAYER_POLL_QUIT:
				state_show_err_msg(state, S("Client Disconnect"));
				break;
			case PLAYER_POLL_MOVED: {
				ChessBoard * board = &state->game.board;
				LegalBoardMoves moves = state->game.legal_moves[poll.as.moved.from];
				if (!legal_board_moves_contains_idx(moves, poll.as.moved.to)) {
					if (p->type == PLAYER_BOT) {
						SDL_Log("Invalid move %u, %u", poll.as.moved.from, poll.as.moved.to);
						player_free(p);
						player_init_human(p); // TODO, for debugging only
					}
					break;
				}
				if (p->type == PLAYER_HUMAN) {
					state_game_make_move(state, poll.as.moved.from, poll.as.moved.to);
				} else {
					state->game.animation.initial_time = elapsed_time;
					state->game.animation.time_diff = 0.0;
					state->game.animation.from = poll.as.moved.from;
					state->game.animation.to = poll.as.moved.to;
					state->game.state = GAME_STATE_MOVING;
				}
				break;
			}
			case PLAYER_POLL_PROMOTION:
				state->game.board.slots[poll.as.promo.to].piece = poll.as.promo.piece; // TODO, validate?
				state_game_next_turn(state);
				break;
		}
		break;
	}
	case GAME_STATE_MOVING:
		state->game.animation.time_diff = elapsed_time - state->game.animation.initial_time;
		if (state->game.animation.time_diff >= PIECE_ANIMATION_SPAN) {
			state_game_make_move(state, state->game.animation.from, state->game.animation.to);
			state->game.state = GAME_STATE_IDLE;
		}
		break;
	case GAME_STATE_FINISHED:
		break;
	}
}

StateUpdateResult state_update(State * state, f32 elapsed_time, f32 delta_time) {
	if (state->should_quit) {
		state_stage_cleanup(state);
		return STATE_UPDATE_QUIT;
	}
	if (state_is_menu(state)) {
		f32 accel = (state->bg_direction == 0 ? -1 : 1) * 150;
		state->bg_speed = clampf(state->bg_speed + accel * delta_time, -50, 50);
		state->bg_rect.x += state->bg_speed * delta_time;
		state->bg_rect.y += state->bg_speed * delta_time;
		if (state->bg_rect.x > state->bg_rect.w) {
			state->bg_rect.x = 0;
		} else if (state->bg_rect.x < 0) {
			state->bg_rect.x = state->bg_rect.w;
		}
		if (state->bg_rect.y > state->bg_rect.h) {
			state->bg_rect.y = 0;
		} else if (state->bg_rect.y < 0) {
			state->bg_rect.y = state->bg_rect.h;
		}
	}
	if (state->mouse_press) {
		SDL_Log("Saw mouse press");
		state->mouse_press = false;
		StateUpdateResult update = state_process_mouse_press(state, elapsed_time);
		if (update != STATE_UPDATE_CONTINUE)
			return update;
	}
	switch (state->stage) {
	case STATE_STAGE_GAME_SETTINGS:
		slider_update(&state->p1_slider, elapsed_time);
		slider_update(&state->p2_slider, elapsed_time);
		slider_update(&state->board_rotate_slider, elapsed_time);
		break;
	case STATE_STAGE_GAME: {
		state_update_game(state, elapsed_time);
		break;
	}
	default:
		break;
	}
	return STATE_UPDATE_CONTINUE;
}

bool slider_status(Slider * slider) {
	switch (slider->state) {
	case SLIDER_ON:
	case SLIDER_OFF_TO_ON:
		return true;
	case SLIDER_OFF:
	case SLIDER_ON_TO_OFF:
		return false;
	}
}

void slider_toggle(Slider * slider, f32 elapsed_time) {
	switch (slider->state) {
	case SLIDER_ON:
		slider->state = SLIDER_ON_TO_OFF;
		slider->transition.initial_time = elapsed_time;
		slider->transition.time_diff = 0;
		break;
	case SLIDER_OFF:
		slider->state = SLIDER_OFF_TO_ON;
		slider->transition.initial_time = elapsed_time;
		slider->transition.time_diff = 0;
		break;
	case SLIDER_OFF_TO_ON:
		slider->state = SLIDER_ON_TO_OFF;
		slider->transition.time_diff = SLIDER_MAX_SECONDS - slider->transition.time_diff;
		slider->transition.initial_time = elapsed_time - slider->transition.time_diff;
		break;
	case SLIDER_ON_TO_OFF:
		slider->state = SLIDER_OFF_TO_ON;
		slider->transition.time_diff = SLIDER_MAX_SECONDS - slider->transition.time_diff;
		slider->transition.initial_time = elapsed_time - slider->transition.time_diff;
		break;
	}
}

void slider_update(Slider * slider, f32 elapsed_time) {
	switch (slider->state) {
	case SLIDER_ON:
	case SLIDER_OFF:
		break;
	case SLIDER_ON_TO_OFF:
		slider->transition.time_diff = elapsed_time - slider->transition.initial_time;
		if (slider->transition.time_diff >= SLIDER_MAX_SECONDS) {
			slider->state = SLIDER_OFF;
		}
		break;
	case SLIDER_OFF_TO_ON:
		slider->transition.time_diff = elapsed_time - slider->transition.initial_time;
		if (slider->transition.time_diff >= SLIDER_MAX_SECONDS) {
			slider->state = SLIDER_ON;
		}
		break;
	}
}

void slider_draw(Slider * slider, Display * display, TextureCache * cache, Rect2f rect, Texture * atlas) {
	int frame;
	switch (slider->state) {
	case SLIDER_ON:
		frame = 0;
		break;
	case SLIDER_OFF:
		frame = 4;
		break;
	case SLIDER_ON_TO_OFF:
		frame = (i32)(slider->transition.time_diff / SLIDER_SECONDS_PER_FRAME);
		break;
	case SLIDER_OFF_TO_ON:
		frame = 4 - (i32)(slider->transition.time_diff / SLIDER_SECONDS_PER_FRAME);
		break;
	}
	Rect2f src = rect2f_new(frame * 32, 0.0, 32.0, 12.0);
	Rect2f dest;
	dest.x = rect.x;
	dest.y = rect.y;
	dest.w = floor_to_nearest(rect.w, 32.0);
	dest.h = rect.w * (12.0 / 32.0);
	if (dest.h > rect.h) {
		dest.h = floor_to_nearest(rect.h, 12.0);
		dest.w = dest.h * (32.0 / 12.0);
	}
	SDL_RenderTexture(display->renderer, atlas, &src, &dest);
}

void draw_menu_bg(Display * display, TextureCache * cache, Rect2f rect) {
	const f32 clip_width = 16;
	const f32 border_width = 32;
	Texture
		* t = texture_cache_lookup(cache, TEXTURE_ID_TOP_BORDER),
		* b = texture_cache_lookup(cache, TEXTURE_ID_BOTTOM_BORDER),
		* l = texture_cache_lookup(cache, TEXTURE_ID_LEFT_BORDER),
		* r = texture_cache_lookup(cache, TEXTURE_ID_RIGHT_BORDER),
		* tr = texture_cache_lookup(cache, TEXTURE_ID_TOP_RIGHT_BORDER),
		* tl = texture_cache_lookup(cache, TEXTURE_ID_TOP_LEFT_BORDER),
		* br = texture_cache_lookup(cache, TEXTURE_ID_BOTTOM_RIGHT_BORDER),
		* bl = texture_cache_lookup(cache, TEXTURE_ID_BOTTOM_LEFT_BORDER);
	const Rect2f tl_r = rect2f_new(rect.x - border_width + clip_width, rect.y - border_width + clip_width, border_width, border_width);
	SDL_RenderTexture(display->renderer, tl, NULL, &tl_r);
	const Rect2f tr_r = rect2f_new(rect.x + rect.w - clip_width, rect.y - border_width + clip_width, border_width, border_width);
	SDL_RenderTexture(display->renderer, tr, NULL, &tr_r);
	const Rect2f bl_r = rect2f_new(rect.x - border_width + clip_width, rect.y + rect.h - clip_width, border_width, border_width);
	SDL_RenderTexture(display->renderer, bl, NULL, &bl_r);
	const Rect2f br_r = rect2f_new(rect.x + rect.w - clip_width, rect.y + rect.h - clip_width, border_width, border_width);
	SDL_RenderTexture(display->renderer, br, NULL, &br_r);
	const Rect2f t_r = rect2f_new(rect.x, rect.y - border_width + clip_width, rect.w, border_width);
	SDL_RenderTexture(display->renderer, t, NULL, &t_r);
	const Rect2f b_r = rect2f_new(rect.x, rect.y + rect.h - clip_width, rect.w, border_width);
	SDL_RenderTexture(display->renderer, b, NULL, &b_r);
	const Rect2f l_r = rect2f_new(rect.x - border_width + clip_width, rect.y, border_width, rect.h);
	SDL_RenderTexture(display->renderer, l, NULL, &l_r);
	const Rect2f r_r = rect2f_new(rect.x + rect.w - clip_width, rect.y, border_width, rect.h);
	SDL_RenderTexture(display->renderer, r, NULL, &r_r);
	renderer_set_draw_color(display->renderer, COLOR_WHITE);
	SDL_RenderFillRect(display->renderer, &rect);
}

void draw_text(Str text, Display * display, TextureCache * cache, Rect2f rect) {
	if (str_is_empty(text)) return;
	Texture * atlas = texture_cache_lookup(cache, TEXTURE_ID_LETTERS);
	usize longest_line = 0;
	usize line_count = text.data[text.size - 1] == '\n' ? 0 : 1;
	for (usize i = 0, t = 0;; ++i) {
		if (i == text.size) {
			longest_line = longest_line > t ? longest_line : t;
			break;
		}
		if (text.data[i] == '\n') {
			longest_line = longest_line > t ? longest_line : t;
			++line_count;
			t = 0;
			continue;
		}
		++t;
	}
	f32 text_offset = floor_to_nearest(rect.w / (f32)longest_line, 6.0);
	f32 text_width = text_offset * (5.0 / 6.0);
	f32 text_height = text_width * (12.0 / 5.0);
	if (text_height * (f32)line_count > rect.h) {
		text_height = floor_to_nearest(rect.h / (f32)line_count, 12.0);
		text_width =  text_height * (5.0 / 12.0);
		text_offset = text_width * (6.0 / 5.0);
	}
	usize x = 0;
	usize y = 0;
	for (usize i = 0; i < text.size; ++i) {
		char c = text.data[i];
		if (c == '\n') {
			x = 0;
			++y;
			continue;
		}
		if (c == ' ') {
			++x;
			continue;
		}
		Rect2f src_rect;
		if (SDL_isupper(c)) {
			src_rect.x = (c - 'A') * 5.0;
			src_rect.y = 0;
		} else if (SDL_islower(c)) {
			src_rect.x = (c - 'a') * 5.0;
			src_rect.y = 12.0;
		} else {
			src_rect.y = 24.0;
			if (SDL_isdigit(c)) {
				src_rect.x = (c - '0') * 5.0;
			} else if (c == '?') {
				src_rect.x = 10.0 * 5.0;
			} else if (c == '!') {
				src_rect.x = 11.0 * 5.0;
			} else {
				SDL_assert(c == '.');
				src_rect.x = 12.0 * 5.0;
			}
		}
		src_rect.w = 5.0;
		src_rect.h = 12.0;
		Rect2f dest_rect =
			rect2f_new(
				rect.x + (f32)x * text_offset,
				rect.y + (f32)y * text_height,
				text_width,
				text_height
			);
		SDL_RenderTexture(display->renderer, atlas, &src_rect, &dest_rect);
		++x;
	}
}

void draw_menu_template(State * state, TextureCache * cache, Display * display) {
	Texture * bgtx = texture_cache_lookup(cache, TEXTURE_ID_BOARD);
	SDL_SetTextureScaleMode(bgtx, SDL_SCALEMODE_LINEAR);
	SDL_RenderTextureTiled(display->renderer, bgtx, &state->bg_rect, state->bg_scale, NULL);
	SDL_SetTextureScaleMode(bgtx, SDL_SCALEMODE_NEAREST);
	draw_menu_bg(display, cache, MENU_RECT);
	draw_text(S("QUIT"), display, cache, get_slot_rect(QUIT_BUTTON_SLOT));
}

void state_draw_game_settings(State * state, TextureCache * cache, Display * display) {
	draw_menu_template(state, cache, display);
	Rect2f a, b, c;
	draw_text(S("GAME SETTINGS"), display, cache, get_slot_rect(MENU_TITLE_SLOT));
	a = get_slot_rect(MENU_TITLE_SLOT + 1);
	partition_rect_horiz(a, 0.5, &b, &c);
	draw_text(S("P1"), display, cache, b);
	slider_draw(&state->p1_slider, display, cache,
		c, texture_cache_lookup(cache, TEXTURE_ID_HUMAN_BOT_SLIDER));
	a = get_slot_rect(MENU_TITLE_SLOT + 2);
	partition_rect_horiz(a, 0.5, &b, &c);
	draw_text(S("P2"), display, cache, b);
	slider_draw(&state->p2_slider, display, cache,
		c, texture_cache_lookup(cache, TEXTURE_ID_HUMAN_BOT_SLIDER));
	a = get_slot_rect(MENU_TITLE_SLOT + 3);
	partition_rect_horiz(a, 0.5, &b, &c);
	draw_text(S("Rotate?"), display, cache, b);
	slider_draw(&state->board_rotate_slider, display, cache,
		c, texture_cache_lookup(cache, TEXTURE_ID_SLIDER));
	draw_text(S("BACK"), display, cache, get_slot_rect(BACK_BUTTON_SLOT));
	draw_menu_bg(display, cache, get_slot_rect(START_BUTTON_SLOT));
	draw_text(S("START"), display, cache, get_slot_rect(START_BUTTON_SLOT));
}

void state_draw_game(State * state, TextureCache * cache, Display * display) {
	Texture * board = texture_cache_lookup(cache, TEXTURE_ID_BOARD);
	Rect2f board_rect = rect2f_new(
		SCREEN_WIDTH * 0.1,
		SCREEN_WIDTH * 0.1,
		SCREEN_WIDTH * 0.8,
		SCREEN_WIDTH * 0.8
	);
	f32 slot_width = board_rect.w / 8.0;
	f32 piece_width = slot_width * (8.0 / 9.0);
	SDL_FlipMode mode = state->board_view == WHITE_SIDE ? SDL_FLIP_NONE : SDL_FLIP_VERTICAL;
	SDL_RenderTextureRotated(display->renderer, board, NULL, &board_rect, 0.0, NULL, mode);
	Player * p = state_current_player(state);
	bool p_has_piece = p->type == PLAYER_HUMAN && p->as.human.held_idx != INVALID_PIECE_IDX;
	bool playing_animation = state->game.state == GAME_STATE_MOVING;
	if (p_has_piece) {
		Texture * tx = texture_cache_lookup(cache, TEXTURE_ID_HOVER_SHADOW);
		LegalBoardMoves moves = state->game.legal_moves[p->as.human.held_idx];
		for (int y = 0; y < 8; ++y) {
			for (int x = 0; x < 8; ++x) {
				u8 sx = (u8)x;
				u8 sy = (u8)y;
				if (state->board_view == WHITE_SIDE) {
					sx = 7 - sx;
					sy = 7 - sy;
				}
				u8 idx = sy * 8 + sx;
				if (idx == p->as.human.held_idx ||
					legal_board_moves_contains_idx(moves, idx)) {
					Rect2f rect = rect2f_new(
						board_rect.x + x * slot_width,
						board_rect.y + y * slot_width,
						slot_width,
						slot_width
					);
					SDL_RenderTexture(display->renderer, tx, NULL, &rect);
				}
			}
		}
	}
	for (int y = 0; y < 8; ++y) {
		for (int x = 0; x < 8; ++x) {
			u8 sx = (u8)x;
			u8 sy = (u8)y;
			if (state->board_view == WHITE_SIDE) {
				sx = 7 - sx;
				sy = 7 - sy;
			}
			u8 idx = sy * 8 + sx;
			if (p_has_piece && p->as.human.held_idx == idx)
				continue;
			else if (playing_animation && state->game.animation.from == idx)
				continue;
			BoardSlot * slot = &state->game.board.slots[idx];
			Texture * tx = texture_cache_lookup_slot(cache, slot);
			if (!tx) continue;
			Rect2f rect = rect2f_new(
				board_rect.x + x * slot_width,
				board_rect.y + y * slot_width,
				piece_width,
				piece_width
			);
			SDL_RenderTexture(display->renderer, tx, NULL, &rect);
		}
	}
	u8 idx = INVALID_PIECE_IDX;
	Vec2f pos;
	if (p_has_piece) {
		idx = p->as.human.held_idx;
		pos = vec2f_add(state->mouse_pos,
				p->as.human.cursor_offset);
	} else if (playing_animation) {
		idx = state->game.animation.from;
		u8 idx2 = state->game.animation.to;
		int x = idx % 8;
		int y = idx / 8;
		int x2 = idx2 % 8;
		int y2 = idx2 / 8;
		if (state->board_view == WHITE_SIDE) {
			x = 7 - x;
			y = 7 - y;
			x2 = 7 - x2;
			y2 = 7 - y2;
		}
		f32 dx = ((f32)x2 - (f32)x) * (state->game.animation.time_diff / PIECE_ANIMATION_SPAN);
		f32 dy = ((f32)y2 - (f32)y) * (state->game.animation.time_diff / PIECE_ANIMATION_SPAN);

		f32 fx = ((f32)x + dx) * slot_width + board_rect.x;
		f32 fy = ((f32)y + dy) * slot_width + board_rect.y;
		pos = vec2f_new(fx, fy);
	}
	if (idx != INVALID_PIECE_IDX) {
		BoardSlot * slot = &state->game.board.slots[idx];
		Texture * tx = texture_cache_lookup_slot(cache, slot);
		Rect2f rect = rect2f_new(pos.x, pos.y, piece_width, piece_width);
		SDL_RenderTexture(display->renderer, tx, NULL, &rect);
	}
	if (state->game.promotion_dialog) {
		Texture * tx = texture_cache_lookup(cache, TEXTURE_ID_PROMOTION_DIALOG_BOX);
		Texture * k = texture_cache_lookup(cache, state->game.board.side == BLACK_SIDE ? TEXTURE_ID_WHITE_KNIGHT : TEXTURE_ID_BLACK_KNIGHT);
		Texture * b = texture_cache_lookup(cache, state->game.board.side == BLACK_SIDE ? TEXTURE_ID_WHITE_BISHOP : TEXTURE_ID_BLACK_BISHOP);
		Texture * r = texture_cache_lookup(cache, state->game.board.side == BLACK_SIDE ? TEXTURE_ID_WHITE_ROOK : TEXTURE_ID_BLACK_KNIGHT);
		Texture * q = texture_cache_lookup(cache, state->game.board.side == BLACK_SIDE ? TEXTURE_ID_WHITE_QUEEN : TEXTURE_ID_BLACK_QUEEN);
		SDL_RenderTexture(display->renderer, tx, NULL, &DIALOG_BOX_RECT);
		SDL_RenderTexture(display->renderer, k, NULL, &DIALOG_KNIGHT_RECT);
		SDL_RenderTexture(display->renderer, b, NULL, &DIALOG_BISHOP_RECT);
		SDL_RenderTexture(display->renderer, r, NULL, &DIALOG_ROOK_RECT);
		SDL_RenderTexture(display->renderer, q, NULL, &DIALOG_QUEEN_RECT);
	}
}

void state_draw(State * state, TextureCache * cache, Display * display) {
	if (state_is_menu(state)) {
		draw_menu_template(state, cache, display);
	}
	switch (state->stage) {
		case STATE_STAGE_TITLE:
			draw_text(S("CHESS GAME"), display, cache, get_slot_rect(MENU_TITLE_SLOT));
			draw_text(S("PLAY"), display, cache, get_slot_rect(PLAY_BUTTON_SLOT));
			draw_text(S("ABOUT"), display, cache, get_slot_rect(ABOUT_BUTTON_SLOT));
			break;
		case STATE_STAGE_ABOUT:
			draw_text(S("Made by"), display, cache, get_slot_rect(MENU_TITLE_SLOT));
			draw_text(S("Alex Adewole"), display, cache, get_slot_rect(MENU_TITLE_SLOT + 1));
			draw_text(S("BACK"), display, cache, get_slot_rect(BACK_BUTTON_SLOT));
			break;
		case STATE_STAGE_GAME_SETTINGS: {
			state_draw_game_settings(state, cache, display);
			break;
		}
		case STATE_STAGE_GAME: {
			state_draw_game(state, cache, display);
			break;
		case STATE_STAGE_ERR_MSG:
			draw_text(S("BACK"), display, cache, get_slot_rect(BACK_BUTTON_SLOT));
			draw_text(state->err_msg, display, cache, get_slot_rect(MENU_TITLE_SLOT + 1));
			break;
		}
	}
}
