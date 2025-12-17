#include "src/include/texture.h"
#include "src/include/state.h"

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
