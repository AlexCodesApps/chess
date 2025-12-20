#include "include/str.h"
#include <SDL3/SDL_stdinc.h>

Str str_new(const char * data, usize size) {
	return (Str){ data, size };
}

Str str_from_cstr(const char * cstr) {
	return str_new(cstr, SDL_strlen(cstr));
}

bool str_is_empty(Str str) {
	return str.size == 0;
}

bool str_starts_with(Str str, Str prefix) {
	if (prefix.size > str.size)
		return false;
	if (prefix.size == 0)
		return true;
	return SDL_memcmp(str.data, prefix.data, prefix.size) == 0;
}

bool str_equal(Str a, Str b) {
	if (a.size != b.size)
		return false;
	if (a.size == 0)
		return true;
	return SDL_memcmp(a.data, b.data, a.size) == 0;
}

Str str_substr(Str str, usize begin, usize end) {
	begin = str.size < begin ? str.size : end;
	end = str.size < end ? str.size : end;
	return str_new(str.data + begin, end - begin);
}

Str str_substr_start(Str str, usize begin) {
	if (str.size <= begin)
		return STR_EMPTY;
	return str_new(str.data + begin, str.size - begin);
}

Str str_substr_end(Str str, usize end) {
	end = str.size < end ? str.size : end;
	return str_new(str.data, end);
}

isize str_find_char(Str str, char c) {
	for (usize i = 0; i < str.size; ++i) {
		if (str.data[i] == c)
			return (isize)i;
	}
	return STR_NPOS;
}

isize str_find_char_in(Str str, Str chars) {
	for (usize i = 0; i < str.size; ++i) {
		if (str_find_char(chars, str.data[i]) != STR_NPOS) {
			return (isize)i;
		}
	}
	return STR_NPOS;
}

isize str_find_str(Str str, Str needle) {
	isize end = (isize)str.size - (isize)needle.size;
	for (isize i = 0; i < end; ++i) {
		if (SDL_memcmp(str.data + i, needle.data, needle.size) == 0) {
			return i;
		}
	}
	return STR_NPOS;
}

StrBuilder str_builder_new() {
	return (StrBuilder){0};
}

void str_builder_init(StrBuilder * builder) {
	SDL_zero(*builder);
}

void str_builder_free(StrBuilder * builder) {
	SDL_free(builder->data);
}

void str_builder_clear(StrBuilder * builder) {
	str_builder_free(builder);
	str_builder_init(builder);
}

bool str_builder_append_char(StrBuilder * builder, char c) {
	if (builder->size + 1 >= builder->capacity) {
		usize new_cap = builder->capacity == 0 ? 8 : builder->capacity * 2;
		char * ndata = SDL_realloc(builder->data, new_cap);
		if (!ndata)
			return false;
		builder->data = ndata;
		builder->capacity = new_cap;
	}
	builder->data[builder->size++] = c;
	builder->data[builder->size] = '\0';
	return true;
}

bool str_builder_append_str(StrBuilder * builder, Str str) {
	if (str.size == 0)
		return true;
	usize new_size = builder->size + str.size;
	if (new_size + 1 >= builder->capacity) {
		usize new_cap = new_size * 2;
		char * ndata = SDL_realloc(builder->data, new_cap);
		if (!ndata)
			return false;
		builder->data = ndata;
		builder->capacity = new_cap;
	}
	SDL_memcpy(builder->data + builder->size, str.data, str.size);
	builder->data[new_size] = '\0';
	builder->size = new_size;
	return true;
}

bool str_builder_append_isize(StrBuilder * builder, isize i) {
	if (i < 0) {
		if (!str_builder_append_char(builder, '-'))
			return false;
		i = -i;
	}
	return str_builder_append_usize(builder, (usize)i);
}

bool str_builder_append_usize(StrBuilder * builder, usize u) {
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
