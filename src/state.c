#include "include/state.h"
#include "include/str.h"

void state_init(State * state, const Display * display) {
	f32 w = (f32)mini(display->win_dims.x, display->win_dims.y);
	state->bg_scale = w / (8 * 16 * 2);
	state->bg_rect = rect2f_new(0, 0, 8 * 8, 8 * 8);
	state->bg_speed = -1;
	state->bg_direction = 0;
}

void state_process_event(State * state, const Event * event) {
	switch (event->type) {
	case QUIT_EVENT:
		state->should_quit = true;
		break;
	case MOUSE_DOWN_EVENT:
		state->bg_direction ^= 1;
		break;
	default:
		break;
	}
}

StateUpdateResult state_update(State * state, f32 delta_time) {
	if (state->should_quit)
		return STATE_UPDATE_QUIT;
	f32 accel = (state->bg_direction == 0 ? -1 : 1) * 100;
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

void draw_menu_bg(Display * display, TextureCache * cache, Rect2f rect, f32 border_width) {
	Texture
		* t = texture_cache_lookup(cache, TEXTURE_ID_TOP_BORDER),
		* b = texture_cache_lookup(cache, TEXTURE_ID_BOTTOM_BORDER),
		* l = texture_cache_lookup(cache, TEXTURE_ID_LEFT_BORDER),
		* r = texture_cache_lookup(cache, TEXTURE_ID_RIGHT_BORDER),
		* tr = texture_cache_lookup(cache, TEXTURE_ID_TOP_RIGHT_BORDER),
		* tl = texture_cache_lookup(cache, TEXTURE_ID_TOP_LEFT_BORDER),
		* br = texture_cache_lookup(cache, TEXTURE_ID_BOTTOM_RIGHT_BORDER),
		* bl = texture_cache_lookup(cache, TEXTURE_ID_BOTTOM_LEFT_BORDER);
	renderer_set_draw_color(display->renderer, COLOR_WHITE);
	SDL_RenderFillRect(display->renderer, &rect);
	const Rect2f tl_r = rect2f_new(rect.x - border_width, rect.y - border_width, border_width, border_width);
	SDL_RenderTexture(display->renderer, tl, NULL, &tl_r);
	const Rect2f tr_r = rect2f_new(rect.x + rect.w, rect.y - border_width, border_width, border_width);
	SDL_RenderTexture(display->renderer, tr, NULL, &tr_r);
	const Rect2f bl_r = rect2f_new(rect.x - border_width, rect.y + rect.h, border_width, border_width);
	SDL_RenderTexture(display->renderer, bl, NULL, &bl_r);
	const Rect2f br_r = rect2f_new(rect.x + rect.w, rect.y + rect.h, border_width, border_width);
	SDL_RenderTexture(display->renderer, br, NULL, &br_r);
	const Rect2f t_r = rect2f_new(rect.x, rect.y - border_width, rect.w, border_width);
	SDL_RenderTexture(display->renderer, t, NULL, &t_r);
	const Rect2f b_r = rect2f_new(rect.x, rect.y + rect.h, rect.w, border_width);
	SDL_RenderTexture(display->renderer, b, NULL, &b_r);
	const Rect2f l_r = rect2f_new(rect.x - border_width, rect.y, border_width, rect.h);
	SDL_RenderTexture(display->renderer, l, NULL, &l_r);
	const Rect2f r_r = rect2f_new(rect.x + rect.w, rect.y, border_width, rect.h);
	SDL_RenderTexture(display->renderer, r, NULL, &r_r);
}

Vec2f get_text_dims(Str text, f32 font_width) {
	const Vec2f tex_pixel_dims = {
		5.0 / 5.0,
		12.0 / 5.0
	};
	const Vec2f letter_dims = vec2f_new(tex_pixel_dims.x * font_width,
								tex_pixel_dims.y * font_width);
	f32 maxl = 0;
	f32 iter = 0;
	f32 lc = 1;
	for (usize i = 0; i < text.size; ++i) {
		if (text.data[i] == '\n') {
			maxl = SDL_max(maxl, iter);
			iter = 0;
			++lc;
		} else {
			++iter;
		}
	}
	return vec2f_mul(letter_dims, vec2f_new(maxl, lc));
}

Rect2f get_letter_src_dims(char c) {
	int x, y;
	if (SDL_isupper(c)) {
		y = 0;
		x = c - 'A';
	} else if (SDL_islower(c)) {
		y = 1;
		x = c - 'a';
	} else {
		SDL_assert(false && "unknown character");
	}
	f32 fx = (f32)x * 5;
	f32 fy = (f32)y * 12;
	return rect2f_new(fx, fy, 5, 12);
}

void draw_text(Str text, Display * display, TextureCache * cache, f32 font_width, Vec2f offset) {
	Texture * atlas = texture_cache_lookup(cache, TEXTURE_ID_LETTERS);
	f32 xo = offset.x, yo = offset.y;
	f32 dx = font_width * (6.0 / 5.0);
	f32 dy = font_width * (12.0 / 5.0);
	for (usize i = 0; i < text.size; ++i) {
		char c = text.data[i];
		if (c == '\n') {
			xo = offset.x;
			yo += dy;
			continue;
		} else if (c != ' ') {
			Rect2f src_rect = get_letter_src_dims(c);
			Rect2f dest_rect = rect2f_new(xo, yo, font_width, dy);
			SDL_RenderTexture(display->renderer, atlas, &src_rect, &dest_rect);
		}
		xo += dx;
	}
}

void draw_text_with_max_width(Str text, Display * display, TextureCache * cache, f32 full_width, Vec2f offset) {
	draw_text(text, display, cache, full_width / (f32)text.size * (5.0 / 6.0), offset);
}

void state_draw(State * state, TextureCache * cache, Display * display) {
	Texture * bgtx = texture_cache_lookup(cache, TEXTURE_ID_BOARD);
	SDL_SetTextureScaleMode(bgtx, SDL_SCALEMODE_LINEAR);
	SDL_RenderTextureTiled(display->renderer, bgtx, &state->bg_rect, state->bg_scale, NULL);
	SDL_SetTextureScaleMode(bgtx, SDL_SCALEMODE_NEAREST);
	Rect2f rect = rect2f_new(
		display->win_dims.x * 0.25,
		display->win_dims.y * 0.25,
		display->win_dims.x * 0.5,
		display->win_dims.y * 0.5
	);
	draw_menu_bg(display, cache, rect, 48);
	Str title = S("CHESS GAME");
	draw_text_with_max_width(title, display, cache, rect.w, rect2f_pos(rect));
	draw_text_with_max_width(S("PLAY"), display, cache, rect.w, vec2f_new(rect.x, rect.y + 50));
}
