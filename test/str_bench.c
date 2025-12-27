#include "../src/include/str.h"
#include "../src/include/bench.h"
#include <SDL3/SDL.h>

bool str_builder_append_usize_1(StrBuilder * builder, usize u) {
	SDL_COMPILE_TIME_ASSERT(a, sizeof(usize) == 8);
	char buf[20]; /* maximum neccesary buffer size */
	unsigned bufl = 0;
	usize exp = 1;
	for (usize u2 = u / 10; u2 != 0; u2 /= 10) {
		exp *= 10;
	}
	do {
		u8 digit = (u / exp) % 10 + '0';
		buf[bufl++] = digit;
		exp /= 10;
	} while (exp);
	return str_builder_append_str(builder, str_new(buf, bufl));
}

bool str_builder_append_usize_2(StrBuilder * builder, usize u) {
	usize exp = 1;
	for (usize u2 = u / 10; u2 != 0; u2 /= 10) {
		exp *= 10;
	}
	do {
		u8 digit = (u / exp) % 10 + '0';
		if (!str_builder_append_char(builder, digit)) {
			return false;
		}
		exp /= 10;
	} while (exp);
	return true;
}

int main(void) {
	usize total_1 = 0;
	usize total_2 = 0;
	StrBuilder builder = str_builder_new();
	for (usize i = 0; i < (2<<4); ++i) {
		SDL_Log("ITERATION");
		BenchMarkStats bench;
		benchmark_begin(&bench);
		for (usize i = 0; i < (2<<15); ++i) {
			bool not_oom = str_builder_append_usize_1(&builder, i);
			SDL_assert(not_oom);
		}
		benchmark_end(&bench);
		str_builder_clear(&builder);
		SDL_Log("str_builder_append_usize_1 took %zu cycles\n",
			benchmark_elapsed_counter(&bench));
		total_1 += benchmark_elapsed_counter(&bench);
		benchmark_begin(&bench);
		for (usize i = 0; i < (2<<15); ++i) {
			bool not_oom = str_builder_append_usize_2(&builder, i);
			SDL_assert(not_oom);
		}
		benchmark_end(&bench);
		str_builder_clear(&builder);
		SDL_Log("str_builder_append_usize_2 took %zu cycles\n",
			benchmark_elapsed_counter(&bench));
		total_2 += benchmark_elapsed_counter(&bench);
	}
	SDL_Log("RESULTS");
	SDL_Log("str_builder_append_usize_1 took %zu cycles total\n", total_1);
	SDL_Log("str_builder_append_usize_2 took %zu cycles total\n", total_2);
	if (total_1 < total_2) {
		SDL_Log("option 1 wins");
	} else if (total_1 > total_2) {
		SDL_Log("option 2 wins");
	} else {
		SDL_Log("options 1 and 2 tied");
	}
}
