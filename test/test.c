#include "test.h"

static unsigned long passed = 0;
static unsigned long failed = 0;

void test_report_assertion(bool condition, const char * file, const char * function, int line, const char * fmt, ...) {
	const char * header = condition ?
		"\x1b[32mOK\x1b[0m" : "\x1b[31mFAIL\x1b[0m";
	if (condition) {
		++passed;
	} else {
		++failed;
	}
	SDL_IOStream * stream = SDL_IOFromDynamicMem();
	SDL_assert(stream);
	bool ok = SDL_IOprintf(stream, "[%s] in %s:%s:%d: ", header, file, function, line) != 0;
	va_list va;
	va_start(va, fmt);
	ok = ok && SDL_IOvprintf(stream, fmt, va) != 0;
	va_end(va);
	ok = ok && SDL_WriteU8(stream, 0);
	SDL_assert(ok);
	SDL_PropertiesID id = SDL_GetIOProperties(stream);
	SDL_assert(id != 0);
	char * msg = SDL_GetPointerProperty(id, SDL_PROP_IOSTREAM_DYNAMIC_MEMORY_POINTER, NULL);
	SDL_Log("%s", msg);
	SDL_CloseIO(stream);
}

int main(void) {
	test_move_counts();
	test_fen_parse_and_encode();
	SDL_Log("PASSED = %lu, FAILED = %lu", passed, failed);
}
