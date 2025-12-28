#include "include/display.h"
#include "include/ints.h"
#include "include/maths.h"

bool display_open(Display * display) {
	SDL_DisplayID id = SDL_GetPrimaryDisplay();
	if (id == 0)
		return false;
	SDL_Rect bounds;
	if (!SDL_GetDisplayUsableBounds(id, &bounds)) {
		return false;
	}
	SDL_Log("Display bounds : (%d %d)", bounds.w, bounds.h);
	int minwidth = bounds.w < bounds.h ? bounds.w : bounds.h;
	f32 approx_win_width = (f32)minwidth / 2.0 * 1.5;
	int win_width = round_to_nearest(approx_win_width, 16);
	f32 scale = (f32)win_width / (f32)SCREEN_WIDTH;
	if (!SDL_CreateWindowAndRenderer("chess", win_width, win_width,
						0, &display->window, &display->renderer)) {
		return false;
	}
	if (!SDL_SetRenderVSync(display->renderer, SDL_RENDERER_VSYNC_ADAPTIVE)) {
		SDL_DestroyRenderer(display->renderer);
		SDL_DestroyWindow(display->window);
		return false;
	}
	if (!SDL_SetRenderScale(display->renderer, scale, scale)) {
		SDL_DestroyRenderer(display->renderer);
		SDL_DestroyWindow(display->window);
		return false;
	}
	display->scale = scale;
	return true;
}

void display_flip(Display * display) {
	SDL_RenderPresent(display->renderer);
	renderer_set_draw_color(display->renderer, COLOR_BLACK);
	SDL_RenderClear(display->renderer);
}

void display_close(Display * display) {
	SDL_DestroyRenderer(display->renderer);
	SDL_DestroyWindow(display->window);
}
