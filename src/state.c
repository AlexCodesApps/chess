#include "include/state.h"
#include "include/str.h"

void state_init(State * state, const Display * display) {
	SDL_zerop(state);
	state->stage = STATE_STAGE_TITLE;
	state->bg_scale = (f32)SCREEN_WIDTH / (8 * 16 * 2);
	state->bg_rect = rect2f_new(0, 0, 8 * 8, 8 * 8);
	state->bg_speed = -50;
	state->bg_direction = 0;
	state->menu_rect = rect2f_new(
		SCREEN_WIDTH * 0.25,
		SCREEN_WIDTH * 0.25,
		SCREEN_WIDTH * 0.5,
		SCREEN_WIDTH * 0.5
	);
	Rect2f tmp, tmp2;
	partition_rect_vert(state->menu_rect, 0.5, &state->menu_title_rect, &tmp);
	partition_rect_vert(tmp, 1.0 / 3.0, &state->play_button_rect, &tmp2);
	partition_rect_vert(tmp2, 0.5, &state->about_button_rect, &state->quit_button_rect);
	state->mouse_pos = vec2f_new(0, 0);
}

void state_process_event(State * state, const Event * event) {
	switch (event->type) {
	case QUIT_EVENT:
		state->should_quit = true;
		break;
	case MOUSE_DOWN_EVENT:
		state->bg_direction ^= 1;
		if (SDL_PointInRectFloat(&state->mouse_pos, &state->quit_button_rect)) {
			state->should_quit = true;
		} else if (SDL_PointInRectFloat(&state->mouse_pos, &state->about_button_rect)) {
			state->stage = STATE_STAGE_ABOUT;
			SDL_Log("Entering about state");
		}
		break;
	case MOUSE_MOVE_EVENT:
		state->mouse_pos = event->as.mouse_move;
		break;
	case MOUSE_UP_EVENT:
		break;
	}
}

StateUpdateResult state_update(State * state, f32 delta_time) {
	if (state->should_quit)
		return STATE_UPDATE_QUIT;
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
	return STATE_UPDATE_CONTINUE;
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
		} else {
			SDL_assert(SDL_islower(c));
			src_rect.x = (c - 'a') * 5.0;
			src_rect.y = 12.0;
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

void state_draw(State * state, TextureCache * cache, Display * display) {
	Texture * bgtx = texture_cache_lookup(cache, TEXTURE_ID_BOARD);
	SDL_SetTextureScaleMode(bgtx, SDL_SCALEMODE_LINEAR);
	SDL_RenderTextureTiled(display->renderer, bgtx, &state->bg_rect, state->bg_scale, NULL);
	SDL_SetTextureScaleMode(bgtx, SDL_SCALEMODE_NEAREST);
	draw_menu_bg(display, cache, state->menu_rect);
	if (state->stage == STATE_STAGE_TITLE) {
		draw_text(S("Chess Game"), display, cache, state->menu_title_rect);
		draw_text(S("PLAY"), display, cache, state->play_button_rect);
		draw_text(S("ABOUT"), display, cache, state->about_button_rect);
		draw_text(S("QUIT"), display, cache, state->quit_button_rect);
	} else {
		draw_text(S("Made by\nAlex Adewole"), display, cache, state->menu_title_rect);
	}
}
