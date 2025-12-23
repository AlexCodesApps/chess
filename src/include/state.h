#include "maths.h"
#include "texture.h"

typedef enum {
	MOUSE_DOWN_EVENT,
	MOUSE_UP_EVENT,
	MOUSE_MOVE_EVENT,
	QUIT_EVENT,
} EventType;

typedef struct {
	EventType type;
	union {
		Vec2f mouse_move;
	} as;
} Event;

typedef enum {
	STATE_STAGE_TITLE,
	STATE_STAGE_ABOUT,
	STATE_STAGE_GAME,
	STATE_STAGE_PAUSE,
} StateStage;

typedef struct {
	Rect2f bg_rect;
	Rect2f menu_rect;
	Rect2f menu_title_rect;
	Rect2f play_button_rect;
	Rect2f about_button_rect;
	Rect2f quit_button_rect;
	Vec2f mouse_pos;
	StateStage stage;
	f32 bg_scale;
	f32 bg_speed;
	bool should_quit : 1;
	u8 bg_direction : 1;
} State;

typedef enum {
	STATE_UPDATE_CONTINUE,
	STATE_UPDATE_QUIT,
} StateUpdateResult;

void state_init(State * state, const Display * display);
void state_process_event(State * state, const Event * event);
StateUpdateResult state_update(State * state, f32 delta_time);
void state_draw(State * state, TextureCache * cache, Display * display);
