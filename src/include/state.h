#include "chess.h"
#include "linalg.h"
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

typedef struct {
} State;

typedef enum {
	STATE_UPDATE_CONTINUE,
	STATE_UPDATE_QUIT,
} StateUpdateResult;

void state_init(State * state, const Display * display);
void state_process_event(State * state, const Event * event);
StateUpdateResult state_update(State * state);
void state_draw(State * state, TextureCache * cache, Display * display);
