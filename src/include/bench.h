#pragma once
#include "ints.h"
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_timer.h>

typedef struct {
	u64 begin;
	u64 begin_milliseconds;
	u64 end;
	u64 end_milliseconds;
} BenchMarkStats;

static void benchmark_begin(BenchMarkStats * stats) {
	stats->begin_milliseconds = SDL_GetTicks();
	stats->begin = SDL_GetPerformanceCounter();
}

static void benchmark_end(BenchMarkStats * stats) {
	stats->end = SDL_GetPerformanceCounter();
	stats->end_milliseconds = SDL_GetTicks();
}

static u64 benchmark_elapsed_counter(BenchMarkStats * stats) {
	return stats->end - stats->begin;
}
