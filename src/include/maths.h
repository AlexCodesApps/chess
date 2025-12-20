#pragma once
#include "ints.h"
#include <SDL3/SDL.h>

typedef SDL_Point Vec2i;
typedef SDL_FPoint Vec2f;
typedef SDL_Rect Rect2i;
typedef SDL_FRect Rect2f;

static i32 absi(i32 i) {
  i32 sign = i >> 31;
  return (i ^ sign) - sign;
}

static i32 getsigni(i32 i) {
	return i >= 0 ? 1 : -1;
}

static i32 mini(i32 a, i32 b) {
	return a < b ? a : b;
}

static i32 maxi(i32 a, i32 b) {
	return a > b ? a : b;
}

static f32 clampf(f32 value, f32 min, f32 max) {
	if (value < min)
		return min;
	if (value > max)
		return max;
	return value;
}

static Vec2i vec2i_new(i32 x, i32 y) {
	return (Vec2i) { x, y };
}

static Vec2i vec2i_from_vec2f(Vec2f v) {
	return vec2i_new((i32)v.x, (i32)v.y);
}

static bool vec2i_equal(Vec2i a, Vec2i b) {
	return a.x == b.x && a.y == b.y;
}

static Vec2f vec2f_new(f32 x, f32 y) {
	return (Vec2f) { x, y };
}

static Vec2i vec2i_add(Vec2i a, Vec2i b) {
	return vec2i_new(a.x + b.x, a.y + b.y);
}

static Vec2i vec2i_sub(Vec2i a, Vec2i b) {
	return vec2i_new(a.x - b.x, a.y - b.y);
}

static Vec2i vec2i_mul(Vec2i a, Vec2i b) {
	return vec2i_new(a.x * b.x, a.y * b.y);
}

static Vec2i vec2i_div(Vec2i a, Vec2i b) {
	return vec2i_new(a.x / b.x, a.y / b.y);
}

static bool vec2f_equal(Vec2f a, Vec2f b) {
	return a.x == b.x && a.y == b.y;
}

static Vec2f vec2f_add(Vec2f a, Vec2f b) {
	return vec2f_new(a.x + b.x, a.y + b.y);
}

static Vec2f vec2f_sub(Vec2f a, Vec2f b) {
	return vec2f_new(a.x - b.x, a.y - b.y);
}

static Vec2f vec2f_mul(Vec2f a, Vec2f b) {
	return vec2f_new(a.x * b.x, a.y * b.y);
}

static Vec2f vec2f_div(Vec2f a, Vec2f b) {
	return vec2f_new(a.x / b.x, a.y / b.y);
}

static Rect2i rect2i_new(i32 x, i32 y, i32 w, i32 h) {
	return (Rect2i) { x, y, w, h };
}

static Rect2f rect2f_new(f32 x, f32 y, f32 w, f32 h) {
	return (Rect2f) { x, y, w, h };
}

static Rect2i rect2i_from_rect2f(Rect2f v) {
	return rect2i_new((i32)v.x, (i32)v.y, (i32)v.w, (i32)v.h);
}

static Vec2i rect2i_pos(Rect2i r) {
	return vec2i_new(r.x, r.y);
}

static Vec2f rect2f_pos(Rect2f r) {
	return vec2f_new(r.x, r.y);
}
