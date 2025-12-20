#include "include/display.h"

bool display_open(Display * display) {
	SDL_DisplayID id = SDL_GetPrimaryDisplay();
	if (id == 0)
		return false;
	SDL_Rect bounds;
	if (!SDL_GetDisplayUsableBounds(id, &bounds)) {
		return false;
	}
	int minwidth = bounds.w < bounds.h ? bounds.w : bounds.h;
	int win_width = (minwidth * 3) / 4;
	if (!SDL_CreateWindowAndRenderer("chess", win_width, win_width,
						0, &display->window, &display->renderer)) {
		return false;
	}
	if (!SDL_GetWindowSize(display->window, &display->win_dims.x, &display->win_dims.y)) {
		SDL_DestroyRenderer(display->renderer);
		SDL_DestroyWindow(display->window);
		return false;
	}
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
