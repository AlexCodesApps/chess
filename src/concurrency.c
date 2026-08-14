#include "include/concurrency.h"

static u8 next(u8 i) {
	return (i + 1) % UCI_MSG_QUEUE_MAX;
}

bool msg_queue_open(MsgQueue * queue) {
	queue->head = 0;
	queue->tail = 0;
	queue->closed = 0;
	queue->lock = SDL_CreateMutex();
	if (!queue->lock)
		return false;
	queue->space_avail_cond = SDL_CreateCondition();
	if (!queue->space_avail_cond) {
		SDL_DestroyMutex(queue->lock);
		return false;
	}
	queue->items_avail_cond = SDL_CreateCondition();
	if (!queue->items_avail_cond) {
		SDL_DestroyCondition(queue->space_avail_cond);
		SDL_DestroyMutex(queue->lock);
		return false;
	}
	return true;
}

void msg_queue_destroy(MsgQueue * queue, void(*freefn)(void *)) {
	SDL_DestroyCondition(queue->items_avail_cond);
	SDL_DestroyCondition(queue->space_avail_cond);
	SDL_DestroyMutex(queue->lock);
	if (freefn) {
		for (u8 i = queue->head; i != queue->tail; i = next(i)) {
			freefn(queue->buf[i]);
		}
	}
}

bool msg_queue_push(MsgQueue * queue, void * line, bool block) {
	SDL_LockMutex(queue->lock);
	if (queue->closed) {
		return false;
	}
	if (next(queue->tail) == queue->head) { /* full */
		if (!block) {
			SDL_UnlockMutex(queue->lock);
			return false;
		}
		SDL_WaitCondition(queue->space_avail_cond, queue->lock);
		if (queue->closed) {
			SDL_UnlockMutex(queue->lock);
			return false;
		}
	}
	queue->buf[queue->tail] = line;
	queue->tail = next(queue->tail);
	SDL_UnlockMutex(queue->lock);
	SDL_SignalCondition(queue->items_avail_cond);
	return true;
}

void * msg_queue_pop(MsgQueue * queue, bool block) {
	SDL_LockMutex(queue->lock);
	if (queue->closed) {
		SDL_UnlockMutex(queue->lock);
		return NULL;
	}
	if (queue->head == queue->tail) { /* empty */
		if (!block) {
			SDL_UnlockMutex(queue->lock);
			return NULL;
		}
		SDL_WaitCondition(queue->items_avail_cond, queue->lock);
		if (queue->closed) {
			SDL_UnlockMutex(queue->lock);
			return NULL;
		}
	}
	char * line = queue->buf[queue->head];
	queue->head = next(queue->head);
	SDL_UnlockMutex(queue->lock);
	SDL_SignalCondition(queue->space_avail_cond);
	return line;
}

void msg_queue_close(MsgQueue * queue) {
	SDL_LockMutex(queue->lock);
	queue->closed = 1;
	SDL_UnlockMutex(queue->lock);
	SDL_SignalCondition(queue->items_avail_cond);
	SDL_SignalCondition(queue->space_avail_cond);
}

