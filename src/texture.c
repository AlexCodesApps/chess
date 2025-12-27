#include <SDL3_image/SDL_image.h>
#include "include/texture.h"

const char * texture_id_to_asset_path(TextureId id) {
	switch (id) {
		case TEXTURE_ID_BOARD:
			return "assets/board.png";
		case TEXTURE_ID_HOVER_SHADOW:
			return "assets/hover_shadow.png";
		case TEXTURE_ID_PROMOTION_DIALOG_BOX:
			return "assets/promotion_dialog_box.png";
		case TEXTURE_ID_LETTERS:
			return "assets/letters.png";
		case TEXTURE_ID_RIGHT_BORDER:
			return "assets/right_border.png";
		case TEXTURE_ID_LEFT_BORDER:
			return "assets/left_border.png";
		case TEXTURE_ID_TOP_BORDER:
			return "assets/top_border.png";
		case TEXTURE_ID_BOTTOM_BORDER:
			return "assets/bottom_border.png";
		case TEXTURE_ID_TOP_RIGHT_BORDER:
			return "assets/top_right_border.png";
		case TEXTURE_ID_TOP_LEFT_BORDER:
			return "assets/top_left_border.png";
		case TEXTURE_ID_BOTTOM_RIGHT_BORDER:
			return "assets/bottom_right_border.png";
		case TEXTURE_ID_BOTTOM_LEFT_BORDER:
			return "assets/bottom_left_border.png";
		case TEXTURE_ID_SLIDER:
			return "assets/slider.png";
		case TEXTURE_ID_HUMAN_BOT_SLIDER:
			return "assets/human_bot_slider.png";
		case TEXTURE_ID_WHITE_PAWN:
			return "assets/white_pawn.png";
		case TEXTURE_ID_WHITE_KNIGHT:
			return "assets/white_knight.png";
		case TEXTURE_ID_WHITE_BISHOP:
			return "assets/white_bishop.png";
		case TEXTURE_ID_WHITE_ROOK:
			return "assets/white_rook.png";
		case TEXTURE_ID_WHITE_QUEEN:
			return "assets/white_queen.png";
		case TEXTURE_ID_WHITE_KING:
			return "assets/white_king.png";
		case TEXTURE_ID_BLACK_PAWN:
			return "assets/black_pawn.png";
		case TEXTURE_ID_BLACK_KNIGHT:
			return "assets/black_knight.png";
		case TEXTURE_ID_BLACK_BISHOP:
			return "assets/black_bishop.png";
		case TEXTURE_ID_BLACK_ROOK:
			return "assets/black_rook.png";
		case TEXTURE_ID_BLACK_QUEEN:
			return "assets/black_queen.png";
		case TEXTURE_ID_BLACK_KING:
			return "assets/black_king.png";
	}
}

static bool surface_lock_rw(SDL_Surface ** ptr) {
	SDL_Surface * sf = *ptr;
	if (sf->format != SDL_PIXELFORMAT_RGBA32) {
		SDL_Surface * nsf = SDL_ConvertSurface(sf, SDL_PIXELFORMAT_RGBA32);
		if (!nsf) {
			SDL_Log("Couldn't convert surface to RGBA32 encoding %p", sf);
			return false;
		}
		SDL_DestroySurface(sf);
		sf = nsf;
		*ptr = nsf;
	}
	if (!SDL_LockSurface(sf)) {
		SDL_Log("Couldn't lock surface at %p", sf);
		return false;
	}
	return true;
}

static void surface_unlock_rw(SDL_Surface * sf) {
	SDL_UnlockSurface(sf);
}

bool texture_cache_init(TextureCache * cache, Renderer * renderer) {
	const char * base = SDL_GetBasePath();
	int i;
	for (i = 0; i < TEXTURE_ID_COUNT; ++i) {
		const char * rel_path = texture_id_to_asset_path((TextureId)i);
		char * path;
		if (SDL_asprintf(&path, "%s/%s", base, rel_path) < 0) {
			SDL_Log("Couldn't allocate memory for path '%s/%s'", base, rel_path);
			goto error;
		}
		SDL_Surface * sf = IMG_Load(path);
		SDL_free(path);
		if (!sf) {
			SDL_Log("Couldn't load image at [%s/%s]", base, rel_path);
			goto error;
		}
		if (i == TEXTURE_ID_LETTERS) {
			if (!surface_lock_rw(&sf)) {
				SDL_DestroySurface(sf);
				goto error;
			}
			u32 * pixels = sf->pixels;
			isize area = sf->w * sf->h;
			for (isize i = 0; i < area; ++i) {
				u32 * pixel = &pixels[i];
				if (*pixel != 0) {
					*(SDL_Color *)pixel = COLOR_BLACK;
				}
			}
			surface_unlock_rw(sf);
		}
		Texture * tx = SDL_CreateTextureFromSurface(renderer, sf);
		SDL_DestroySurface(sf);
		if (!tx)
			goto error;
		if (!SDL_SetTextureScaleMode(tx, SDL_SCALEMODE_NEAREST)) {
			SDL_DestroyTexture(tx);
			goto error;
		}
		cache->internal[i] = tx;
	}
	return true;
error:
	while (--i >= 0) {
		SDL_DestroyTexture(cache->internal[i]);
	}
	return false;
}

Texture * texture_cache_lookup(TextureCache * cache, TextureId id) {
	SDL_assert(0 <= id && id < TEXTURE_ID_COUNT);
	return cache->internal[id];
}

Texture * texture_cache_lookup_slot(TextureCache * cache, const BoardSlot * slot) {
	if (!slot->has_piece) return NULL;
	switch (slot->side) {
		case WHITE_SIDE:
			switch (slot->piece) {
			case CHESS_PAWN:
				return texture_cache_lookup(cache, TEXTURE_ID_WHITE_PAWN);
			case CHESS_KNIGHT:
				return texture_cache_lookup(cache, TEXTURE_ID_WHITE_KNIGHT);
			case CHESS_BISHOP:
				return texture_cache_lookup(cache, TEXTURE_ID_WHITE_BISHOP);
			case CHESS_ROOK:
				return texture_cache_lookup(cache, TEXTURE_ID_WHITE_ROOK);
			case CHESS_QUEEN:
				return texture_cache_lookup(cache, TEXTURE_ID_WHITE_QUEEN);
			case CHESS_KING:
				return texture_cache_lookup(cache, TEXTURE_ID_WHITE_KING);
			}
		case BLACK_SIDE:
			switch (slot->piece) {
				case CHESS_PAWN:
					return texture_cache_lookup(cache, TEXTURE_ID_BLACK_PAWN);
				case CHESS_KNIGHT:
					return texture_cache_lookup(cache, TEXTURE_ID_BLACK_KNIGHT);
				case CHESS_BISHOP:
					return texture_cache_lookup(cache, TEXTURE_ID_BLACK_BISHOP);
				case CHESS_ROOK:
					return texture_cache_lookup(cache, TEXTURE_ID_BLACK_ROOK);
				case CHESS_QUEEN:
					return texture_cache_lookup(cache, TEXTURE_ID_BLACK_QUEEN);
				case CHESS_KING:
					return texture_cache_lookup(cache, TEXTURE_ID_BLACK_KING);
			}
	}
}

void texture_cache_free(TextureCache * cache) {
	for (int i = 0; i < TEXTURE_ID_COUNT; ++i) {
		SDL_DestroyTexture(cache->internal[i]);
	}
}
