#include "include/uci.h"
#include "include/str.h"
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

bool msg_queue_push(MsgQueue * queue, void * line, bool block) {
	SDL_LockMutex(queue->lock);
	if (queue->closed != 0) {
		return false;
	}
	if (queue->head == next(queue->tail)) { /* full */
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
	if (queue->closed != 0) {
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
	for (u8 i = queue->head; i != queue->tail; i = next(i)) {
	}
	SDL_UnlockMutex(queue->lock);
	SDL_SignalCondition(queue->items_avail_cond);
	SDL_SignalCondition(queue->space_avail_cond);
}

int producer_thread(void * arg) {
	UciServer * server = arg;
	StrBuilder builder = str_builder_new();
	SDL_IOStream * in = SDL_GetProcessOutput(server->process);
	while (SDL_GetAtomicInt(&server->cancel) == 0) {
		u8 c;
		for (;;) {
			if (!SDL_ReadU8(in, &c)) {
				if (SDL_GetIOStatus(in) == SDL_IO_STATUS_NOT_READY) {
					continue;
				}
				break;
			}
			if (c == '\n') {
				break;
			}
			if (!str_builder_append_char(&builder, c)) {
				str_builder_free(&builder);
				SDL_SetAtomicInt(&server->eof, 1);
				return -1;
			}
		}
		if (!str_builder_ensure_null_term(&builder)) {
			str_builder_free(&builder);
			SDL_SetAtomicInt(&server->eof, 1);
			return -1;
		}
		if (!msg_queue_push(&server->output, builder.data, true)) {
			SDL_free(builder.data);
			break;
		}
		str_builder_init(&builder);
		if (SDL_GetIOStatus(in) == SDL_IO_STATUS_EOF) {
			break;
		}
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
		SDL_WriteIO(out, line, SDL_strlen(line));
		SDL_WriteU8(out, '\n');
		SDL_free(line);
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
	/* Consumer should be constructed before producer because
	   If the producer is constructed first and constructing the consumer fails,
	   then allocated lines the producer processed from the process may leak.
	*/
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
	char * line = msg_queue_pop(&server->output, false);
	if (line) {
		SDL_Log("Chess bot: %s", line);
	}
	return line;
}

bool uci_server_send_line(UciServer * server, char * line) {
	return msg_queue_push(&server->input, line, false);
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
	for (u8 i = server->input.head; i != server->input.tail; i = next(i)) {
		SDL_Log("Unread message from server : %s\n", (char *)server->input.buf[i]);
		SDL_free(server->input.buf[i]);
	}
	for (u8 i = server->output.head; i != server->output.tail; i = next(i)) {
		SDL_Log("Unread message to server : %s\n", (char *)server->output.buf[i]);
		SDL_free(server->output.buf[i]);
	}
	msg_queue_destroy(&server->input);
	msg_queue_destroy(&server->output);
	return status;
}

bool uci_server_eof(UciServer * server) {
	return SDL_GetAtomicInt(&server->eof) == 1
		&& server->output.head == server->output.tail;
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

static Str next_token(char ** out) {
	char * begin = *out;
	char * iter = begin;
	char * end = iter;
	while (*iter != '\0') {
		if (c_is_ws(*iter)) {
			do {
				++iter;
			} while (c_is_ws(*iter));
			break;
		}
		++iter;
		end = iter;
	}
	*out = iter;
	return str_new(begin, end - begin);
}

static bool recognized_cmd(Str cmd) {
	if (str_equal(cmd, S("id"))) {
		return true;
	}
	if (str_equal(cmd, S("info"))) {
		return true;
	}
	if (str_equal(cmd, S("uciok"))) {
		return true;
	}
	if (str_equal(cmd, S("readyok"))) {
		return true;
	}
	if (str_equal(cmd, S("bestmove"))) {
		return true;
	}
	return false;
}

static Str get_cmd_name(char ** line) {
	while (c_is_ws(**line))
		++(*line);
	Str cmd = next_token(line);
	while (!recognized_cmd(cmd)) {
		if (str_is_empty(cmd))
			break;
		cmd = next_token(line);
	}
	return cmd;
}

static bool cmd_name_equal(char * line, Str name) {
	return str_equal(get_cmd_name(&line), name);
}

static UciClientPollResult start_send_request(UciClient * client, UciServer * server, char * line) {
	if (!uci_server_send_line(server, line)) {
		client->backed_up_ptr = line;
	}
	return UCI_POLL_CLIENT_CONTINUE;
}

static UciClientPollResult start_send_request_lit(UciClient * client, UciServer * server, const char * lit) {
	char * line = SDL_strdup(lit);
	if (!line)
		return UCI_POLL_CLIENT_OOM;
	if (!uci_server_send_line(server, line)) {
		client->backed_up_ptr = line;
	}
	return UCI_POLL_CLIENT_CONTINUE;
}

void uci_client_init(UciClient * client) {
	SDL_zerop(client);
}

void uci_client_free(UciClient * client) {
	SDL_free(client->backed_up_ptr);
}

u8 text_pos_to_idx(const char pos[static 2]) {
	char a = pos[0];
	char b = pos[1];
	if (a < 'a' || a > 'h')
		return INVALID_PIECE_IDX;
	u8 x = 7 - (a - 'a');
	if (b < '1' || b > '9')
		return INVALID_PIECE_IDX;
	u8 y = b - '1';
	return y * 8 + x;
}

bool parse_move(Str move, UciMoveRequestData * out) {
	if (move.size < 4 || move.size > 5)
		return false;
	out->out_from = text_pos_to_idx(move.data);
	out->out_to = text_pos_to_idx(move.data + 2);
	if (out->out_from == INVALID_PIECE_IDX || out->out_to == INVALID_PIECE_IDX) {
		return false;
	}
	if (move.size == 5) {
		ChessPiece piece;
		switch (move.data[4]) {
		case 'n':
			piece = CHESS_KNIGHT;
			break;
		case 'b':
			piece = CHESS_BISHOP;
			break;
		case 'r':
			piece = CHESS_ROOK;
			break;
		case 'q':
			piece = CHESS_QUEEN;
			break;
		default:
			return false;
		}
		out->out_did_promo = true;
		out->out_promo = piece;
	}
	return true;
}

UciClientPollResult uci_poll_client(UciClient * client, UciServer * server) {
	if (uci_server_eof(server)) {
		return UCI_POLL_CLIENT_QUIT;
	}
	if (client->backed_up_ptr) {
		if (!uci_server_send_line(server, client->backed_up_ptr)) {
			return UCI_POLL_CLIENT_CONTINUE;
		}
		client->backed_up_ptr = NULL;
	}
	char * line;
	switch (client->state) {
	case UCI_CLIENT_NEW:
		client->state = UCI_CLIENT_EXPECTING_UCIOK;
		return start_send_request_lit(client, server, "uci");
	case UCI_CLIENT_EXPECTING_UCIOK:
		line = uci_server_poll_line(server);
		if (!line)
			return UCI_POLL_CLIENT_CONTINUE;
		{
			UciClientPollResult r = UCI_POLL_CLIENT_CONTINUE;
			char * iter = line;
			Str cmd = get_cmd_name(&iter);
			if (str_equal(cmd, S("option"))
				&& str_equal(next_token(&iter), S("name"))
				&& str_equal(next_token(&iter), S("OwnBook"))) {
				r = start_send_request_lit(client, server, "setoption name OwnBook value true");
			} else if (str_equal(cmd, S("uciok"))) {
				r = start_send_request_lit(client, server, "ucinewgame\nisready");
				client->state = UCI_CLIENT_EXPECTING_READYOK;
			}
			SDL_free(line);
			return r;
		}
	case UCI_CLIENT_EXPECTING_READYOK:
		line = uci_server_poll_line(server);
		if (!line)
			return UCI_POLL_CLIENT_CONTINUE;
		if (cmd_name_equal(line, S("readyok"))) {
			client->state = UCI_CLIENT_MAIN_LOOP;
		}
		SDL_free(line);
		return UCI_POLL_CLIENT_CONTINUE;
	case UCI_CLIENT_MAIN_LOOP:
		if (!client->move_request)
			return UCI_POLL_CLIENT_CONTINUE;
		client->state = UCI_CLIENT_EXPECTING_MOVE_REQ_RESP;
		{
			StrBuilder builder = str_builder_new();
			if (!str_builder_append_str(&builder, S("position fen "))) {
				return UCI_POLL_CLIENT_OOM;
			}
			if (!fen_encode_board(&builder, client->move_request->board)) {
				goto oom;
			}
			if (!str_builder_append_str(&builder, S("\ngo movetime "))) {
				goto oom;
			}
			if (!str_builder_append_usize(&builder, client->move_request->timeout_ms)) {
				goto oom;
			}
			SDL_Log("Launched request [\n%s\n]", builder.data);
			return start_send_request(client, server, builder.data);
		oom:
			str_builder_free(&builder);
			return UCI_POLL_CLIENT_OOM;
		}
	case UCI_CLIENT_EXPECTING_MOVE_REQ_RESP:
		line = uci_server_poll_line(server);
		if (!line)
			return UCI_POLL_CLIENT_CONTINUE;
		{
			UciClientPollResult r = UCI_POLL_CLIENT_CONTINUE;
			char * iter = line;
			Str cmd = get_cmd_name(&iter);
			if (str_equal(cmd, S("bestmove"))) {
				SDL_Log("Request fulfilled");
				UciMoveRequestData * req = client->move_request;
				Str bestmove = next_token(&iter);
				if (!parse_move(bestmove, req)) {
					r = UCI_POLL_CLIENT_INVALID_DATA;
				} else {
					client->move_request = NULL;
					client->state = UCI_CLIENT_MAIN_LOOP;
					r = UCI_POLL_CLIENT_MOVE_RESPONSE;
				}
			}
			SDL_free(line);
			return r;
		}
	}
}

void uci_client_request_move(UciClient * client, UciMoveRequestData * data) {
	client->move_request = data;
}
