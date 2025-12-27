#pragma once
#include "chess.h"
#include "display.h"

typedef SDL_Texture Texture;

typedef enum {
	TEXTURE_ID_BOARD,
	TEXTURE_ID_HOVER_SHADOW,
	TEXTURE_ID_PROMOTION_DIALOG_BOX,
	TEXTURE_ID_LETTERS,
	TEXTURE_ID_RIGHT_BORDER,
	TEXTURE_ID_LEFT_BORDER,
	TEXTURE_ID_TOP_BORDER,
	TEXTURE_ID_BOTTOM_BORDER,
	TEXTURE_ID_TOP_RIGHT_BORDER,
	TEXTURE_ID_TOP_LEFT_BORDER,
	TEXTURE_ID_BOTTOM_RIGHT_BORDER,
	TEXTURE_ID_BOTTOM_LEFT_BORDER,
	TEXTURE_ID_SLIDER,
	TEXTURE_ID_HUMAN_BOT_SLIDER,
	TEXTURE_ID_WHITE_PAWN,
	TEXTURE_ID_WHITE_KNIGHT,
	TEXTURE_ID_WHITE_BISHOP,
	TEXTURE_ID_WHITE_ROOK,
	TEXTURE_ID_WHITE_QUEEN,
	TEXTURE_ID_WHITE_KING,
	TEXTURE_ID_BLACK_PAWN,
	TEXTURE_ID_BLACK_KNIGHT,
	TEXTURE_ID_BLACK_BISHOP,
	TEXTURE_ID_BLACK_ROOK,
	TEXTURE_ID_BLACK_QUEEN,
	TEXTURE_ID_BLACK_KING
} TextureId;

#define TEXTURE_ID_COUNT (TEXTURE_ID_BLACK_KING + 1)

const char * texture_id_to_asset_path(TextureId id);

typedef struct {
	void * internal[TEXTURE_ID_COUNT];
} TextureCache;

bool texture_cache_init(TextureCache * cache, Renderer * renderer);
Texture * texture_cache_lookup(TextureCache * cache, TextureId id);
Texture * texture_cache_lookup_slot(TextureCache * cache, const BoardSlot * slot);
void texture_cache_free(TextureCache * cache);
