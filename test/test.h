#pragma once
#include <SDL3/SDL.h>

#define LOG SDL_Log

void test_report_assertion(bool condition, const char * file, const char * function, int line, const char * fmt, ...) SDL_PRINTF_VARARG_FUNC(5);

#define ASSERT(condition, ...) test_report_assertion(condition, SDL_FILE, SDL_FUNCTION, SDL_LINE, __VA_ARGS__)
#define ASSERT_FAIL(...) test_report_assertion(false, SDL_FILE, SDL_FUNCTION, SDL_LINE, __VA_ARGS__)
#define ASSERT_OK(...) test_report_assertion(true, SDL_FILE, SDL_FUNCTION, SDL_LINE, __VA_ARGS__)
#define OOM_CHECK(...) if (!(__VA_ARGS__)) { SDL_Log("OOM"); SDL_TriggerBreakpoint(); }

void test_move_counts(void);
void test_fen_parse_and_encode(void);
