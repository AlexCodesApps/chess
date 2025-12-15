#include "../src/include/uci.h"
#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_log.h>

int main(void) {
	UciServer server;
	const char * const args[] = { "cat", "main.c", NULL };
	if (!uci_server_start(&server, args)) {
		SDL_Log("%s", SDL_GetError());
		return 1;
	}
	while (!uci_server_eof(&server)) {
		char * line;
		while ((line = uci_server_poll_line(&server))) {
			SDL_Log("%s", line);
			SDL_free(line);
		}
	}
	uci_server_close(&server);
}
