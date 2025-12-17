#include "include/state.h"

void state_init(State * state, const Display * display);
void state_process_event(State * state, const Event * event);
StateUpdateResult state_update(State * state);
void state_draw(State * state, TextureCache * cache, Display * display);
