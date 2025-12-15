#include "include/uci.h"
#include <SDL3/SDL_log.h>

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

void msg_queue_destroy(MsgQueue * queue) {
	SDL_DestroyCondition(queue->items_avail_cond);
	SDL_DestroyCondition(queue->space_avail_cond);
	SDL_DestroyMutex(queue->lock);
}

static u8 next(u8 i) {
	return (i + 1) % UCI_SERVER_QUEUE_MAX;
}

bool msg_queue_push(MsgQueue * queue, void * line) {
	SDL_LockMutex(queue->lock);
	if (queue->closed != 0) {
		return false;
	}
	if (queue->head == next(queue->tail)) { // full
		SDL_WaitCondition(queue->space_avail_cond, queue->lock);
		if (queue->closed != 0) {
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
	if (queue->closed != 0) {
		SDL_UnlockMutex(queue->lock);
		return NULL;
	}
	if (queue->head == queue->tail) { // empty
		if (!block) {
			SDL_UnlockMutex(queue->lock);
			return NULL;
		}
		SDL_WaitCondition(queue->items_avail_cond, queue->lock);
		if (queue->closed != 0) {
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
	for (u8 i = queue->head; i != queue->tail; i = next(i)) {
		SDL_free(queue->buf[i]);
		queue->buf[i] = NULL;
	}
	SDL_UnlockMutex(queue->lock);
	SDL_SignalCondition(queue->items_avail_cond);
	SDL_SignalCondition(queue->space_avail_cond);
}

int producer_thread(void * arg) {
	UciServer * server = arg;
	char * data = NULL;
	size_t size = 0;
	size_t capacity = 0;
	SDL_IOStream * in = SDL_GetProcessOutput(server->process);
	while (SDL_GetAtomicInt(&server->cancel) == 0) {
		u8 c;
		while (SDL_ReadU8(in, &c)) {
			if (c == '\n')
				break;
			if (size + 2 >= capacity) {
				size_t new_cap = capacity == 0 ? 8 : capacity * 2;
				char * new_data = SDL_realloc(data, new_cap);
				if (!new_data) {
					SDL_free(data);
					SDL_SetAtomicInt(&server->eof, 1);
					return -1;
				}
				data = new_data;
				capacity = new_cap;
			}
			data[size++] = c;
			data[size] = '\0';
		}
		msg_queue_push(&server->output, data);
		if (SDL_GetIOStatus(in) == SDL_IO_STATUS_EOF) {
			break;
		}
		data = NULL;
		size = 0;
		capacity = 0;
	}
	SDL_SetAtomicInt(&server->eof, 1);
	return 0;
}

int consumer_thread(void * arg) {
	UciServer * server = arg;
	SDL_IOStream * out = SDL_GetProcessInput(server->process);
	char * line;
	while (SDL_GetAtomicInt(&server->cancel) == 0
			&& (line = msg_queue_pop(&server->input, true))) {
		SDL_WriteIO(out, line, strlen(line));
		SDL_WriteU8(out, '\n');
	}
	return 0;
}

bool uci_server_start(UciServer * server, const char * const * args) {
	if (!msg_queue_open(&server->input))
		return false;
	if (!msg_queue_open(&server->output))
		goto destroy_input;
	SDL_Process * process = SDL_CreateProcess(args, true);
	if (!process)
		goto destroy_output;
	server->process = process;
	server->cancel.value = 0;
	server->eof.value = 0;
	SDL_Thread * consumer = SDL_CreateThread(consumer_thread, NULL, server);
	if (!consumer)
		goto destroy_process;
	SDL_Thread * producer = SDL_CreateThread(producer_thread, NULL, server);
	if (!producer)
		goto destroy_consumer;
	server->consumer = consumer;
	server->producer = producer;
	return true;
destroy_consumer:
	SDL_SetAtomicInt(&server->cancel, 1);
	SDL_WaitThread(consumer, NULL);
destroy_process:
	SDL_KillProcess(process, false);
	SDL_DestroyProcess(process);
destroy_output:
	msg_queue_close(&server->output);
destroy_input:
	msg_queue_close(&server->input);
	return false;
}

char * uci_server_poll_line(UciServer * server) {
	return msg_queue_pop(&server->output, false);
}

void uci_server_send_line(UciServer * server, char * line) {
	msg_queue_push(&server->input, line);
}

int uci_server_shutdown(UciServer * server) {
	int status;
	msg_queue_close(&server->input);
	msg_queue_close(&server->output);
	SDL_SetAtomicInt(&server->cancel, 1);
	SDL_KillProcess(server->process, false);
	SDL_WaitProcess(server->process, true, &status);
	SDL_DestroyProcess(server->process);
	return status;
}

int uci_server_close(UciServer * server) {
	int status = uci_server_shutdown(server);
	SDL_WaitThread(server->consumer, NULL);
	SDL_WaitThread(server->producer, NULL);
	msg_queue_destroy(&server->input);
	msg_queue_destroy(&server->output);
	return status;
}

bool uci_server_eof(UciServer * server) {
	return SDL_GetAtomicInt(&server->eof) == 1;
}

static bool c_is_ws(u8 c) {
	switch (c) {
	case ' ':
	case '\r':
	case '\t':
		return true;
	default:
		return false;
	}
}

static char * strtok_ws_r(char * line, char ** save_ptr) {
	char * begin = line ? line : *save_ptr;
	char * iter = begin;
	while (*iter != '\0') {
		if (c_is_ws(*iter)) {
			do {
				*iter = '\0';
				++iter;
			} while (c_is_ws(*iter));
			break;
		}
		++iter;
	}
	*save_ptr = iter;
	return begin;
}

static bool try_parse_id(char * line) {
	char * save_ptr = NULL;
	char * next = strtok_ws_r(line, &save_ptr);
	if (strcmp(next, "id") != 0) {
		return false;
	}
	while ((next = strtok_ws_r(NULL, &save_ptr))) {
		if (strcmp(next, "name") == 0) {
		}
		if (strcmp(next, "author") == 0) {
		}
	}
	return true;
}

UciClientPollResult uci_poll_client(UciClient * client, UciServer * server) {
	if (uci_server_eof(server)) {
		return UCI_POLL_CLIENT_QUIT;
	}
	char * line;
	switch (client->state) {
	case UCI_CLIENT_NEW:
		uci_server_send_line(server, "uci");
		client->state = UCI_CLIENT_EXPECTING_ID;
	case UCI_CLIENT_EXPECTING_ID:
		line = uci_server_poll_line(server);
		if (line) {
		}
		SDL_free(line);
		return UCI_POLL_CLIENT_CONTINUE;
	case UCI_CLIENT_EXPECTING_UCIOK:
		line = uci_server_poll_line(server);
		if (line) {
			if (strcmp(line, "uciok") == 0) {
				client->state = UCI_CLIENT_EXPECTING_UCIOK;
			}
		}
		SDL_free(line);
		return UCI_POLL_CLIENT_CONTINUE;
	}
}
