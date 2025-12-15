#pragma once

#include "ints.h"
#include <SDL3/SDL_asyncio.h>
#include <SDL3/SDL_mutex.h>
#include <SDL3/SDL_process.h>
#define UCI_SERVER_QUEUE_MAX 8

typedef struct {
	u8 closed : 1;
	u8 head : 3;
	u8 tail : 4;
	SDL_Mutex * lock;
	SDL_Condition * space_avail_cond;
	SDL_Condition * items_avail_cond;
	void * buf[UCI_SERVER_QUEUE_MAX];
} MsgQueue;

bool msg_queue_open(MsgQueue * queue);
bool msg_queue_push(MsgQueue * queue, void * line);
void * msg_queue_pop(MsgQueue * queue, bool block);
void msg_queue_close(MsgQueue * queue);
void msg_queue_destroy(MsgQueue * queue);

typedef struct {
	SDL_Process * process;
	MsgQueue input;
	MsgQueue output;
	SDL_Thread * producer;
	SDL_Thread * consumer;
	SDL_AtomicInt cancel;
	SDL_AtomicInt eof;
} UciServer;

/* Primitives */
bool uci_server_start(UciServer * server, const char * const * args);
char * uci_server_poll_line(UciServer * server);
bool uci_server_eof(UciServer * server);
void uci_server_send_line(UciServer * server, char * line);
int uci_server_shutdown(UciServer * server);
int uci_server_close(UciServer * server);

typedef enum {
	UCI_CLIENT_NEW,
	UCI_CLIENT_EXPECTING_ID,
	UCI_CLIENT_EXPECTING_UCIOK,
} UciClientState;

typedef enum {
	UCI_POLL_CLIENT_CONTINUE,
	UCI_POLL_CLIENT_QUIT,
} UciClientPollResult;

typedef struct {
	UciClientState state;
} UciClient;

UciClientPollResult uci_poll_client(UciClient * client, UciServer * server);
