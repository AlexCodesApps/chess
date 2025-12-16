#pragma once

#include "ints.h"

typedef struct {
	const char * data;
	usize size;
} Str;

#define S(lit) ((Str){ lit, sizeof(lit) - 1 })
#define STR_EMPTY ((Str){0})

Str str_new(const char * data, usize size);
Str str_from_cstr(const char * cstr);

bool str_is_empty(Str str);
bool str_starts_with(Str str, Str prefix);
bool str_equal(Str a, Str b);

Str str_substr(Str str, usize begin, usize end);
Str str_substr_start(Str str, usize begin);
Str str_substr_end(Str str, usize end);

#define STR_NPOS ((isize)-1)

isize str_find_char(Str str, char c);
isize str_find_char_in(Str str, Str chars);
isize str_find_str(Str str, Str needle);

typedef struct {
	char * data;
	usize size;
	usize capacity;
} StrBuilder;

StrBuilder str_builder_new();
void str_builder_init(StrBuilder * builder);
void str_builder_free(StrBuilder * builder);
void str_builder_clear(StrBuilder * builder);

bool str_builder_append_char(StrBuilder * builder, char c);
bool str_builder_append_str(StrBuilder * builder, Str str);
bool str_builder_append_isize(StrBuilder * builder, isize i);
bool str_builder_append_usize(StrBuilder * builder, usize u);
