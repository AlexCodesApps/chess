#include "ints.h"
#include <SDL3/SDL.h>

#define SCREEN_WIDTH 360

typedef SDL_Renderer Renderer;
typedef SDL_Window Window;
typedef SDL_Color Color;

typedef struct {
	Renderer * renderer;
	Window * window;
	f32 scale;
} Display;

bool display_open(Display * display);
void display_flip(Display * display);
void display_close(Display * display);

#define COLOR(r, g, b, a) ((Color){ r, g, b, a})
#define COLOR_RED COLOR(255, 0, 0, 255)
#define COLOR_BLUE COLOR(0, 0, 255, 255)
#define COLOR_GREEN COLOR(0, 255, 0, 255)
#define COLOR_BLACK COLOR(0, 0, 0, 255)
#define COLOR_WHITE COLOR(255, 255, 255, 255)

static void renderer_set_draw_color(Renderer * renderer, Color color) {
	SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
}
