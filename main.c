#include "src/include/texture.h"
#include "src/include/state.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>


int entry(void) {
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
		SDL_Log("Init Error: %s", SDL_GetError());
		return 1;
	}
	SDL_Log("Initialized SDL subsystems");
	Display display;
	if (!display_open(&display)) {
		SDL_Log("Display Error: %s", SDL_GetError());
		SDL_Quit();
		return 1;
	}
	SDL_Log("Opened display window");
	TextureCache cache;
	if (!texture_cache_init(&cache, display.renderer)) {
		SDL_Log("Texture Cache Error: %s", SDL_GetError());
		display_close(&display);
		SDL_Quit();
		return 1;
	}
	SDL_Log("Loaded all textures");
	State state;
	state_init(&state);
	bool running = true;
	SDL_Log("Entering main loop");
	u64 initial_ticks = SDL_GetTicks();
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
				if (!SDL_ConvertEventToRenderCoordinates(display.renderer, &_event)) {
					SDL_Log("%s", SDL_GetError());
					running = false;
					continue;
				}
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
		u64 current_ticks = SDL_GetTicks();
		f32 elapsed_time = (f32)current_ticks / SDL_MS_PER_SECOND;
		f32 delta_time = (f32)(current_ticks - initial_ticks) / SDL_MS_PER_SECOND;
		initial_ticks = current_ticks;
		StateUpdateResult result = state_update(&state, elapsed_time, delta_time);
		switch (result) {
		case STATE_UPDATE_QUIT:
			running = false;
			break;
		case STATE_UPDATE_CONTINUE:
			break;
		}
		state_draw(&state, &cache, &display);
		display_flip(&display);
	}
	texture_cache_free(&cache);
	SDL_Log("Freed textures");
	display_close(&display);
	SDL_Log("Closed display");
	SDL_Quit();
	SDL_Log("Closed SDL Subsystems");
	return 0;
}

int main(int argc, char ** argv) {
	int result = entry();
	SDL_Log("App finished with code %d", result);
	return result;
}
