#include <stdio.h>
#include "../src/include/uci.h"
#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_log.h>

__ssize_t getline(char **restrict lineptr, size_t *restrict n,
			   FILE *restrict stream);

int main(void) {
	// NOLINTBEGIN
	UciServer server;
	const char * const args[] = { "cat", NULL };
	if (!uci_server_start(&server, args)) {
		SDL_Log("%s", SDL_GetError());
		return 1;
	}
	bool write_done = false;
	char * data = NULL; size_t size = 0;
	while (!uci_server_eof(&server)) {
		if (!write_done) {
			if (!data) {
				long wr = getline(&data, &size, stdin);
				if (wr == -1) {
					write_done = true;
					msg_queue_close(&server.input);
					continue;
				}
				if (data[size - 1] == '\n')
					data[size - 1] = '\0';
			}
			if (uci_server_send_line(&server, data)) {
				data = NULL;
			}
		}
		char * line;
		while ((line = uci_server_poll_line(&server))) {
			SDL_Log("%s", line);
			SDL_free(line);
		}
	}
	uci_server_close(&server);
	// NOLINTEND
}
