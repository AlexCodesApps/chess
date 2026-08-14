#include "include/display.h"
#include "include/ints.h"
#include "include/maths.h"
#include <SDL3/SDL_platform.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>

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
	if (!SDL_CreateWindowAndRenderer(
			"chess", win_width, win_width,
			SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE,
			&display->window, &display->renderer)) {
		return false;
	}
	if (SDL_SetRenderVSync(display->renderer, SDL_RENDERER_VSYNC_ADAPTIVE)) {
		SDL_Log("Set VSYNC");
	}
	if (!SDL_SetRenderLogicalPresentation(
				display->renderer,
				SCREEN_WIDTH, SCREEN_WIDTH,
				SDL_LOGICAL_PRESENTATION_LETTERBOX)) {
		goto error;
	}
	SDL_Log("Set Logical Presentation");
	display_clear(display);
	return true;
error:
	SDL_DestroyRenderer(display->renderer);
	SDL_DestroyWindow(display->window);
	return false;
}

void display_clear(Display * display) {
	renderer_set_draw_color(display->renderer, COLOR_BLACK);
	SDL_RenderClear(display->renderer);
}

void display_flip(Display * display) {
	SDL_RenderPresent(display->renderer);
	display_clear(display);
}

void display_close(Display * display) {
	SDL_DestroyRenderer(display->renderer);
	SDL_DestroyWindow(display->window);
}
