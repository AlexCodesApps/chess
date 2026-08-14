#include "ints.h"
#include <SDL3/SDL.h>

#define UCI_MSG_QUEUE_MAX 8

typedef struct {
	u8 closed : 1;
	u8 head : 3;
	u8 tail : 4;
	SDL_Mutex * lock;
	SDL_Condition * space_avail_cond;
	SDL_Condition * items_avail_cond;
	void * buf[UCI_MSG_QUEUE_MAX];
} MsgQueue;

bool msg_queue_open(MsgQueue * queue);
bool msg_queue_push(MsgQueue * queue, void * line, bool block);
void * msg_queue_pop(MsgQueue * queue, bool block);
void msg_queue_close(MsgQueue * queue);
void msg_queue_destroy(MsgQueue * queue, void(*freefn)(void *));

