#include "ints.h"
#include <SDL3/SDL.h>

typedef struct {
	bool is_exe : 1;
	bool line_ready : 1;
	bool eof : 1;
	union {
		SDL_Process * exe;
		SDL_Thread  * internal;
	} as;
	SDL_Mutex * mutex;
	SDL_IOStream * in;
	SDL_IOStream * out;
	SDL_IOStream * async_writer;
	char buff[1024];
	usize cnt;
} BotClient;

bool bot_client_open_internal(BotClient * client);
bool bot_client_open_exe(BotClient * client, const char * args);
bool bot_client_is_eof(const BotClient * client);
/**
 * Polls client for input, returning a line if available.
 * lifetime of the line ends at the next call
 */
const char * bot_poll_for_line(BotClient * client);
SDL_IOStream * bot_client_async_writer(void);
void bot_client_close(BotClient * client);

