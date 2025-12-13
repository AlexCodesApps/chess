#include "include/state.h"

Rect2f get_board_dims(const Display * display) {
	Rect2f board_dims = {
		.x = (f32)display->window_dimensions.x * 0.05,
		.y = (f32)display->window_dimensions.y * 0.05,
		.w = (f32)display->window_dimensions.x * 0.9,
		.h = (f32)display->window_dimensions.y * 0.9,
	};
	return board_dims;
}

f32 get_slot_width(const Rect2f * board_rect) {
	return board_rect->w / 8.0f;
}

f32 get_piece_width(f32 slot_width) {
	return slot_width * (15.0f / 16.0f);
}

void state_reset_board_rect(State * state, Rect2f new_rect) {
	state->board_rect = new_rect;
	state->slot_width = get_slot_width(&new_rect);
	state->piece_width = get_piece_width(state->slot_width);
}

void promotion_dialog_init(PromotionDialog * dialog, const Rect2f * board_rect, f32 slot_width) {
	dialog->choice = PROMOTION_DIALOG_NONE;
	f32 margin = 1;
	f32 pad = 1;
	dialog->rect = rect2f_new(
		board_rect->x,
		board_rect->y,
		(slot_width + pad) * 4 + margin,
		slot_width + margin * 2);
		dialog->knight_rect =
		rect2f_new(dialog->rect.x + margin, dialog->rect.y + margin,
					slot_width, slot_width);
		dialog->bishop_rect =
			rect2f_new(dialog->rect.x + margin + slot_width + pad,
					dialog->rect.y + margin, slot_width, slot_width);
		dialog->rook_rect =
			rect2f_new(dialog->rect.x + margin + (slot_width + pad) * 2,
					dialog->rect.y + margin, slot_width, slot_width);
		dialog->queen_rect =
			rect2f_new(dialog->rect.x + margin + (slot_width + pad) * 3,
					dialog->rect.y + margin, slot_width, slot_width);
}

void gen_legal_board_moves_array(LegalBoardMoves moves[32], ChessBoard * board) {
	for (u8 i = 0; i < board->piece_count; ++i) {
		u8 idx = board->piece_idxs[i];
		moves[i] = board_get_legal_moves_for_piece(board, idx, i);
	}
}

void state_init(State * state, const Display * display) {
	state->board = INITIAL_CHESS_BOARD;
	gen_legal_board_moves_array(state->legal_moves, &state->board);
	state->board_rect = get_board_dims(display);
	state->slot_width = get_slot_width(&state->board_rect);
	promotion_dialog_init(&state->promotion_dialog, &state->board_rect, state->slot_width);
	state->piece_width = get_piece_width(state->slot_width);
	SDL_GetMouseState(&state->mouse_location.x, &state->mouse_location.y);
	state->promoting = false;
	state->mouse_is_down = false;
	state->did_quit = false;
	state->view = WHITE_SIDE;
	state->side = WHITE_SIDE;
	state->held_piece_idx = INVALID_PIECE_IDX;
	state->promotion_dialog.pad = 1;
	state->promotion_dialog.margin = 1;
}

/* X and Y are relative to screen, not board! */
Rect2f get_piece_rect(State * state, int rx, int ry) {
	Rect2f rect = rect2f_new(
		state->board_rect.x + (f32)rx * state->slot_width,
		state->board_rect.y + (f32)ry * state->slot_width,
		state->piece_width,
		state->piece_width);
	return rect;
}

bool mouse_in_board_rect(State * rect) {
	return SDL_PointInRectFloat(&rect->mouse_location, &rect->board_rect);
}

int board_idx_at_mouse(State * state, int * rx, int * ry) {
	const Vec2f m_to_b_offset =
		vec2f_sub(state->mouse_location, rect2f_pos(state->board_rect));
	const Vec2f posf = vec2f_div(m_to_b_offset, vec2f_new(state->slot_width, state->slot_width));
	if (posf.x >= 0.0f && posf.x < 8.0f && posf.y >= 0.0f && posf.y < 8.0f) {
		const Vec2i pos = vec2i_from_vec2f(posf);
		if (rx) *rx = pos.x;
		if (ry) *ry = pos.y;
		if (state->view == WHITE_SIDE) {
			return (7 - pos.y) * 8 + (7 - pos.x);
		} else {
			return pos.y * 8 + pos.x;
		}
	}
	return INVALID_PIECE_IDX;
}

void state_process_event(State * state, const Event * event) {
	switch (event->type) {
	case QUIT_EVENT:
		state->did_quit = true;
		break;
	case MOUSE_DOWN_EVENT: {
		int rx, ry;
		state->mouse_is_down = true;
		if (state->promoting) {
			if (SDL_PointInRectFloat(&state->mouse_location, &state->promotion_dialog.knight_rect)) {
				state->promotion_dialog.choice = PROMOTION_DIALOG_KNIGHT;
			} else if (SDL_PointInRectFloat(&state->mouse_location, &state->promotion_dialog.bishop_rect)) {
				state->promotion_dialog.choice = PROMOTION_DIALOG_BISHOP;
			} else if (SDL_PointInRectFloat(&state->mouse_location, &state->promotion_dialog.rook_rect)) {
				state->promotion_dialog.choice = PROMOTION_DIALOG_ROOK;
			} else if (SDL_PointInRectFloat(&state->mouse_location, &state->promotion_dialog.queen_rect)) {
				state->promotion_dialog.choice = PROMOTION_DIALOG_QUEEN;
			}
			break;
		}
		u8 idx = board_idx_at_mouse(state, &rx, &ry);
		if (idx == INVALID_PIECE_IDX)
			break;
		BoardSlot * slot = &state->board.slots[idx];
		if (!slot->has_piece)
			break;
		for (u8 i = 0; i < state->board.piece_count; ++i) {
			if (state->board.piece_idxs[i] == idx) {
				state->held_piece_idx = idx;
				state->held_piece_idx_idx = i;
				Rect2f piece_rect = get_piece_rect(state, rx, ry);
				state->held_piece_offset =
					vec2f_sub(state->mouse_location, rect2f_pos(piece_rect));
				break;
			}
		}
		break;
	}
	case MOUSE_UP_EVENT:
		state->mouse_is_down = false;
		break;
	case MOUSE_MOVE_EVENT:
		state->mouse_location = event->as.mouse_move;
		break;
	}
}

StateUpdateResult state_update(State * state) {
	if (state->did_quit) return STATE_UPDATE_QUIT;
	if (state->promoting) {
		ChessPiece piece;
		switch (state->promotion_dialog.choice) {
		case PROMOTION_DIALOG_NONE:
			return STATE_UPDATE_CONTINUE;
		case PROMOTION_DIALOG_KNIGHT:
			piece = CHESS_KNIGHT;
			break;
		case PROMOTION_DIALOG_BISHOP:
			piece = CHESS_BISHOP;
			break;
		case PROMOTION_DIALOG_ROOK:
			piece = CHESS_ROOK;
			break;
		case PROMOTION_DIALOG_QUEEN:
			piece = CHESS_QUEEN;
			break;
		}
		state->promoting = false;
		state->board.slots[state->held_piece_idx].piece = piece;
		gen_legal_board_moves_array(state->legal_moves, &state->board);
		SDL_Log("Promoted to %s", chess_piece_str(piece));
		state->held_piece_idx = INVALID_PIECE_IDX;
		state->promotion_dialog.choice = PROMOTION_DIALOG_NONE;
	} else if (!state->mouse_is_down && state->held_piece_idx != INVALID_PIECE_IDX) {
		/* selected piece just been dropped */
		u8 idx = board_idx_at_mouse(state, NULL, NULL);
		if (idx != INVALID_PIECE_IDX
			&& idx != state->held_piece_idx
			&& legal_board_moves_contains_idx(state->legal_moves[state->held_piece_idx_idx], idx)) {
			BoardMoveResult result =
				board_make_move(&state->board,
					state->held_piece_idx, state->held_piece_idx_idx, idx);
			state->side ^= 1;
			if (result.promotion) {
				state->held_piece_idx = idx;
				state->promoting = true;
				return STATE_UPDATE_CONTINUE;
			}
			gen_legal_board_moves_array(state->legal_moves, &state->board);
			if (check_board_for_checks(&state->board, WHITE_SIDE)) {
				SDL_Log("White king in check");
			}
			if (check_board_for_checks(&state->board, BLACK_SIDE)) {
				SDL_Log("Black king in check");
			}
		}
		state->held_piece_idx = INVALID_PIECE_IDX;
	}
	return STATE_UPDATE_CONTINUE;
}

void state_draw(State * state, TextureCache * cache, Display * display) {
	Texture * board_tx = texture_cache_lookup(cache, TEXTURE_ID_BOARD);
	SDL_RenderTexture(display->renderer, board_tx, NULL, &state->board_rect);
	for (int y = 0; y < 8; ++y) {
		for (int x = 0; x < 8; ++x) {
			int idx = y * 8 + x;
			if (!state->promoting && idx == state->held_piece_idx)
				continue;
			int rx = x;
			int ry = y;
			if (state->view == WHITE_SIDE) {
				rx = 7 - x;
				ry = 7 - y;
			}
			BoardSlot * slot = &state->board.slots[idx];
			Texture * tx = texture_cache_lookup_slot(cache, slot);
			if (!tx) continue;
			Rect2f rect = get_piece_rect(state, rx, ry);
			SDL_RenderTexture(display->renderer, tx, NULL, &rect);
		}
	}
	if (state->mouse_is_down && state->held_piece_idx != INVALID_PIECE_IDX && !state->promoting) {
		LegalBoardMoves moves = board_get_legal_moves_for_piece(&state->board, state->held_piece_idx, state->held_piece_idx_idx);
		u8 count = 0;
		for (u64 i = 0; i < 64; ++i) {
			if ((moves & ((u64)1 << i)) == 0)
				continue;
			++count;
			int rx = i % 8;
			int ry = i / 8;
			if (state->view == WHITE_SIDE) {
				rx = 7 - rx;
				ry = 7 - ry;
			}
			Texture * shadow_tx = texture_cache_lookup(cache, TEXTURE_ID_HOVER_SHADOW);
			Rect2f rect = get_piece_rect(state, rx, ry);
			SDL_RenderTexture(display->renderer, shadow_tx, NULL, &rect);
		}
		if (mouse_in_board_rect(state)) {
			Rect2f shadow_rect = rect2f_new(
				SDL_floorf((state->mouse_location.x - state->board_rect.x) / state->slot_width) * state->slot_width + state->board_rect.x,
				SDL_floorf((state->mouse_location.y - state->board_rect.y) / state->slot_width) * state->slot_width + state->board_rect.y,
				state->slot_width,
				state->slot_width
			);
			Texture * shadow_tx =
				texture_cache_lookup(cache, TEXTURE_ID_HOVER_SHADOW);
			SDL_RenderTexture(display->renderer, shadow_tx, NULL, &shadow_rect);
		}
		Rect2f rect = rect2f_new(
			state->mouse_location.x - state->held_piece_offset.x,
			state->mouse_location.y - state->held_piece_offset.y,
			state->piece_width,
			state->piece_width
		);
		Texture * tx =
			texture_cache_lookup_slot(cache,
				&state->board.slots[state->held_piece_idx]);
		SDL_RenderTexture(display->renderer, tx, NULL, &rect);
	}
	if (state->promoting) {
		Texture * knight_tx,
				* bishop_tx,
				* rook_tx,
				* queen_tx;
		Texture * promo_bg_tx = texture_cache_lookup(cache, TEXTURE_ID_PROMOTION_DIALOG_BOX);
		SDL_assert(state->held_piece_idx != INVALID_PIECE_IDX);
		if (state->board.slots[state->held_piece_idx].side == WHITE_SIDE) {
			knight_tx = texture_cache_lookup(cache, TEXTURE_ID_WHITE_KNIGHT);
			bishop_tx = texture_cache_lookup(cache, TEXTURE_ID_WHITE_BISHOP);
			rook_tx = texture_cache_lookup(cache, TEXTURE_ID_WHITE_ROOK);
			queen_tx = texture_cache_lookup(cache, TEXTURE_ID_WHITE_QUEEN);
		} else {
			knight_tx = texture_cache_lookup(cache, TEXTURE_ID_BLACK_KNIGHT);
			bishop_tx = texture_cache_lookup(cache, TEXTURE_ID_BLACK_BISHOP);
			rook_tx = texture_cache_lookup(cache, TEXTURE_ID_BLACK_ROOK);
			queen_tx = texture_cache_lookup(cache, TEXTURE_ID_BLACK_QUEEN);
		}
		SDL_RenderTexture(display->renderer, promo_bg_tx, NULL, &state->promotion_dialog.rect);
		SDL_RenderTexture(display->renderer, knight_tx, NULL, &state->promotion_dialog.knight_rect);
		SDL_RenderTexture(display->renderer, bishop_tx, NULL, &state->promotion_dialog.bishop_rect);
		SDL_RenderTexture(display->renderer, rook_tx, NULL, &state->promotion_dialog.rook_rect);
		SDL_RenderTexture(display->renderer, queen_tx, NULL, &state->promotion_dialog.queen_rect);
	}
}
