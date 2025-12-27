#pragma once

#include "chess.h"
#include "ints.h"
#include "str.h"
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
bool msg_queue_push(MsgQueue * queue, void * line, bool block);
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
/* call uci_server_poll_line in a loop bc this may trigger prematurely */
bool uci_server_eof(UciServer * server);
bool uci_server_send_line(UciServer * server, char * line);
int uci_server_shutdown(UciServer * server);
int uci_server_close(UciServer * server);

typedef enum {
	UCI_CLIENT_NEW,
	UCI_CLIENT_EXPECTING_UCIOK,
	UCI_CLIENT_EXPECTING_READYOK,
	UCI_CLIENT_MAIN_LOOP,
	UCI_CLIENT_EXPECTING_MOVE_REQ_RESP
} UciClientState;

typedef enum {
	UCI_POLL_CLIENT_CONTINUE,
	UCI_POLL_CLIENT_QUIT,
	UCI_POLL_CLIENT_MOVE_RESPONSE,
	UCI_POLL_CLIENT_OOM,
	UCI_POLL_CLIENT_INVALID_DATA,
} UciClientPollResult;

typedef struct {
	u8 out_to;
	u8 out_from;
	bool out_did_promo;
	ChessPiece out_promo;
	u32 timeout_ms;
	const ChessBoard * board;
} UciMoveRequestData;

typedef struct {
	UciClientState state;
	char * backed_up_ptr;
	UciMoveRequestData * move_request;
} UciClient;

/* neccesary for communication */
bool fen_parse_board(Str str, ChessBoard * board);
bool fen_encode_board(StrBuilder * builder, const ChessBoard * board);

void uci_client_init(UciClient * client);
void uci_client_free(UciClient * client);
UciClientPollResult uci_poll_client(UciClient * client, UciServer * server);
/* to_idx must live until the request is completed or cancelled or the client quits */
void uci_client_request_move(UciClient * client, UciMoveRequestData * data);
