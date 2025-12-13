#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <poll.h>

typedef enum : Uint8 {
	CHESS_PAWN,
	CHESS_KNIGHT,
	CHESS_BISHOP,
	CHESS_ROOK,
	CHESS_QUEEN,
	CHESS_KING,
} ChessPiece;

typedef enum : bool {
	PLAYER_WHITE,
	PLAYER_BLACK,
} ChessPlayer;

typedef struct {
	bool has_piece : 1;
	ChessPlayer side : 1;
	ChessPiece piece : 6;
} BoardSlot;

typedef struct {
	BoardSlot slots[64];
} ChessBoard;

#define EMPTY_SLOT ((BoardSlot) { .has_piece = 0 })

#define W(PIECE) (BoardSlot) { \
	.has_piece = 1, \
	.side = PLAYER_WHITE, \
	.piece = CHESS_ ## PIECE, \
}
#define E EMPTY_SLOT
#define B(PIECE) (BoardSlot) { \
	.has_piece = 1, \
	.side = PLAYER_BLACK, \
	.piece = CHESS_ ## PIECE, \
}

static const ChessBoard beginning_board = {
	.slots = {
		W(ROOK), W(KNIGHT), W(BISHOP), W(KING), W(QUEEN), W(BISHOP), W(KNIGHT), W(ROOK),
		W(PAWN), W(PAWN), W(PAWN), W(PAWN), W(PAWN), W(PAWN), W(PAWN), W(PAWN),
		E, E, E, E, E, E, E, E,
		E, E, E, E, E, E, E, E,
		E, E, E, E, E, E, E, E,
		E, E, E, E, E, E, E, E,
		B(PAWN), B(PAWN), B(PAWN), B(PAWN), B(PAWN), B(PAWN), B(PAWN), B(PAWN),
		B(ROOK), B(KNIGHT), B(BISHOP), B(KING), B(QUEEN), B(BISHOP), B(KNIGHT), B(ROOK),
	}
};

static Uint8 beginning_piece_idxs[32] = {
	/* White pieces */
	0, 1, 2, 3, 4, 5, 6, 7,
	8, 9, 10, 11, 12, 13, 14, 15,
	/* Black pieces */
	48, 49, 50, 51, 52, 53, 54, 55,
	56, 57, 58, 59, 60, 61, 62, 63
};

#undef W
#undef E
#undef B

typedef struct {
	SDL_Texture * board,
				* white_pawn,
				* white_knight,
				* white_bishop,
				* white_rook,
				* white_queen,
				* white_king;
	SDL_Texture * black_pawn,
				* black_knight,
				* black_bishop,
				* black_rook,
				* black_queen,
				* black_king;
} TextureCache;

SDL_Texture * lookup_board_slot_texture(TextureCache * cache, const BoardSlot * slot) {
	if (!slot->has_piece) return NULL;
	switch (slot->side) {
	case PLAYER_WHITE:
		switch (slot->piece) {
			case CHESS_PAWN: return cache->white_pawn; break;
			case CHESS_KNIGHT: return cache->white_knight; break;
			case CHESS_BISHOP: return cache->white_bishop; break;
			case CHESS_ROOK: return cache->white_rook; break;
			case CHESS_QUEEN: return cache->white_queen; break;
			case CHESS_KING: return cache->white_king; break;
		}
	break;
	case PLAYER_BLACK:
		switch (slot->piece) {
			case CHESS_PAWN: return cache->black_pawn; break;
			case CHESS_KNIGHT: return cache->black_knight; break;
			case CHESS_BISHOP: return cache->black_bishop; break;
			case CHESS_ROOK: return cache->black_rook; break;
			case CHESS_QUEEN: return cache->black_queen; break;
			case CHESS_KING: return cache->black_king; break;
		}
	break;
	}
}

SDL_Surface * copy_surface(SDL_Surface * sf) {
	return SDL_CreateSurfaceFrom(sf->w, sf->h, sf->format, sf->pixels, sf->pitch);
}

SDL_Texture * load_pixel_art_texture(SDL_Renderer * renderer, const char * path) {
	SDL_Texture * tex = IMG_LoadTexture(renderer, path);
	if (!tex) {
		SDL_Log("loading texture [%s] failed", path);
		return NULL;
	}
	if (!SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST)) {
		SDL_Log("couldn't set texture scale mode");
		SDL_DestroyTexture(tex);
		return NULL;
	}
	return tex;
}

bool load_texture_cache(SDL_Renderer * renderer, TextureCache * cache) {
	cache->board = load_pixel_art_texture(renderer, "assets/board.png");
	if (!cache->board) return NULL;
	cache->white_pawn = load_pixel_art_texture(renderer, "assets/white_pawn.png");
	if (!cache->white_pawn) goto destroy_board;
	cache->white_knight = load_pixel_art_texture(renderer, "assets/white_knight.png");
	if (!cache->white_knight) goto destroy_pawn;
	cache->white_bishop = load_pixel_art_texture(renderer, "assets/white_bishop.png");
	if (!cache->white_bishop) goto destroy_knight;
	cache->white_rook = load_pixel_art_texture(renderer, "assets/white_rook.png");
	if (!cache->white_rook) goto destroy_bishop;
	cache->white_queen = load_pixel_art_texture(renderer, "assets/white_queen.png");
	if (!cache->white_queen) goto destroy_rook;
	cache->white_king = load_pixel_art_texture(renderer, "assets/white_king.png");
	if (!cache->white_king) goto destroy_queen;
	cache->black_pawn = load_pixel_art_texture(renderer, "assets/black_pawn.png");
	if (!cache->black_pawn) goto destroy_king;
	cache->black_knight = load_pixel_art_texture(renderer, "assets/black_knight.png");
	if (!cache->black_knight) goto destroy_black_pawn;
	cache->black_bishop = load_pixel_art_texture(renderer, "assets/black_bishop.png");
	if (!cache->black_bishop) goto destroy_black_knight;
	cache->black_rook = load_pixel_art_texture(renderer, "assets/black_rook.png");
	if (!cache->black_rook) goto destroy_black_bishop;
	cache->black_queen = load_pixel_art_texture(renderer, "assets/black_queen.png");
	if (!cache->black_queen) goto destroy_black_rook;
	cache->black_king = load_pixel_art_texture(renderer, "assets/black_king.png");
	if (!cache->black_king) goto destroy_black_queen;
	return true;
destroy_black_king:
	SDL_DestroyTexture(cache->black_king);
destroy_black_queen:
	SDL_DestroyTexture(cache->black_queen);
destroy_black_rook:
	SDL_DestroyTexture(cache->black_rook);
destroy_black_bishop:
	SDL_DestroyTexture(cache->black_bishop);
destroy_black_knight:
	SDL_DestroyTexture(cache->black_knight);
destroy_black_pawn:
	SDL_DestroyTexture(cache->black_pawn);
destroy_king:
	SDL_DestroyTexture(cache->white_king);
destroy_queen:
	SDL_DestroyTexture(cache->white_queen);
destroy_rook:
	SDL_DestroyTexture(cache->white_rook);
destroy_bishop:
	SDL_DestroyTexture(cache->white_bishop);
destroy_knight:
	SDL_DestroyTexture(cache->white_knight);
destroy_pawn:
	SDL_DestroyTexture(cache->white_pawn);
destroy_board:
	SDL_DestroyTexture(cache->board);
	return false;
}

void free_texture_cache(TextureCache * cache) {
	SDL_DestroyTexture(cache->board);
	SDL_DestroyTexture(cache->white_pawn);
	SDL_DestroyTexture(cache->white_knight);
	SDL_DestroyTexture(cache->white_bishop);
	SDL_DestroyTexture(cache->white_rook);
	SDL_DestroyTexture(cache->white_queen);
	SDL_DestroyTexture(cache->white_king);
	SDL_DestroyTexture(cache->black_pawn);
	SDL_DestroyTexture(cache->black_knight);
	SDL_DestroyTexture(cache->black_bishop);
	SDL_DestroyTexture(cache->black_rook);
	SDL_DestroyTexture(cache->black_queen);
	SDL_DestroyTexture(cache->black_king);
}

typedef struct {
	SDL_FPoint piece_slot_offset;
	SDL_Point screen_dims;
	SDL_FPoint board_position;
	float slot_width;
	float piece_width;
	float board_width;
} DisplayInfo;

bool get_display_information(DisplayInfo * info) {
	SDL_DisplayID id = SDL_GetPrimaryDisplay();
	if (id == 0) {
		return false;
	}
	SDL_Rect display_bounds;
	if (!SDL_GetDisplayUsableBounds(id, &display_bounds)) {
		return false;
	}
	int min_bound = display_bounds.w < display_bounds.h ? display_bounds.w : display_bounds.h;
	int screen_width = (min_bound * 3) / 4;
	float board_width = (float)screen_width;
	float slot_width = (float)board_width / 8;
	if (slot_width < 9.0f) {
		return SDL_SetError("Display bounds are too small for application [ x = %d, y = %d, w = %d, h = %d ]",
			display_bounds.x, display_bounds.y, display_bounds.w, display_bounds.h);
	}
	info->screen_dims = (SDL_Point){ screen_width, screen_width };
	info->board_position.x = 0;
	info->board_position.y = 0;
	info->board_width = board_width;
	info->slot_width = slot_width;
	info->piece_width = (slot_width * 15) / 16;
	info->piece_slot_offset.x = 0;
	info->piece_slot_offset.y = slot_width / 19;
	return true;
}

SDL_FRect get_piece_display_rect(int piece_idx, const DisplayInfo * info) {
	int x = 7 - piece_idx % 8;
	int y = 7 - piece_idx / 8;
	SDL_FRect point = {
		.x = (float)x * info->slot_width + info->piece_slot_offset.x,
		.y = (float)y * info->slot_width + info->piece_slot_offset.y,
		.w = info->piece_width,
		.h = info->piece_width,
	};
	return point;
}

typedef struct {
	ChessBoard board;
	SDL_FPoint mouse_pos;
	SDL_FPoint moused_piece_offset;
	Uint8 piece_idxs[32];
	Uint8 n_piece_idxs;
	Sint8 moused_slot_idx;
	ChessPlayer current_player : 1;
	bool mouse_selected : 1;
	bool mouse_release_event : 1;
} GameState;

void set_stdin_nonblock(void) {
	int flags = fcntl(STDIN_FILENO, F_GETFL);
	if (flags == -1) {
		perror("fcntl");
		exit(1);
	}
	flags |= O_NONBLOCK;
	if (fcntl(STDIN_FILENO, F_SETFL, flags) == -1) {
		perror("fcntl");
		exit(1);
	}
}

// for debug purposes! not robust!
void process_stdin(GameState * state, DisplayInfo * info) {
	char line[256];
	ssize_t size;
	if ((size = read(STDIN_FILENO, line, 255)) <= 0) {
		return;
	}
	if (line[size - 1] == '\n')
		--size;
	line[size] = '\0';
	char var[32];
	float value;
	float value2;
	bool ran_cmd = true;
	if (strcmp(line, "dump") == 0) {
		SDL_Log("screen_dims = (%d, %d)", info->screen_dims.x, info->screen_dims.y);
		SDL_Log("board_position = (%f, %f)", info->board_position.x, info->board_position.y);
		SDL_Log("piece_slot_offset = (%f, %f)", info->piece_slot_offset.x, info->piece_slot_offset.y);
		SDL_Log("board_width = %f", info->board_width);
		SDL_Log("piece_width = %f", info->piece_width);
		SDL_Log("slot_width = %f", info->slot_width);
	} else if (sscanf(line, "get %31s", var) == 1) {
		if (strcmp(var, "screen_dims") == 0) {
			SDL_Log("screen_dims = (%d, %d)", info->screen_dims.x, info->screen_dims.y);
		} if (strcmp(var, "board_position") == 0) {
			SDL_Log("board_position = (%f, %f)", info->board_position.x, info->board_position.y);
		} else if (strcmp(var, "piece_slot_offset") == 0) {
			SDL_Log("piece_slot_offset = (%f, %f)", info->piece_slot_offset.x, info->piece_slot_offset.y);
		} else if (strcmp(var, "board_width") == 0) {
			SDL_Log("board_width = %f", info->board_width);
		} else if (strcmp(var, "piece_width") == 0) {
			SDL_Log("piece_width = %f", info->piece_width);
		} else if (strcmp(var, "slot_width") == 0) {
			SDL_Log("slot_width = %f", info->slot_width);
		} else {
			SDL_Log("unknown option '%s'", var);
			ran_cmd = false;
		}
	} else if (sscanf(line, "set %31s %f %f", var, &value, &value2) == 3) {
		if (strcmp(var, "screen_dims") == 0) {
			SDL_Log("cannot set screen dims");
		} else if (strcmp(var, "board_position") == 0) {
			info->board_position.x = value;
			info->board_position.y = value2;
		} else if (strcmp(var, "piece_slot_offset") == 0) {
			info->piece_slot_offset.x = value;
			info->piece_slot_offset.y = value2;
		} else {
			SDL_Log("unknown vec2 option '%s'", var);
			ran_cmd = false;
		}
	} else if (sscanf(line, "set %31s %f", var, &value) == 2) {
		if (strcmp(var, "board_width") == 0) {
			info->board_width = value;
		} else if (strcmp(var, "piece_width") == 0) {
			info->piece_width = value;
		} else if (strcmp(var, "slot_width") == 0) {
			info->slot_width = value;
		} else if (strcmp(var, "board_position.x") == 0) {
			info->board_position.x = value;
		} else if (strcmp(var, "board_position.y") == 0) {
			info->board_position.y = value;
		} else if (strcmp(var, "piece_slot_offset.x") == 0) {
			info->piece_slot_offset.x = value;
		} else if (strcmp(var, "piece_slot_offset.y") == 0) {
			info->piece_slot_offset.y = value;
		} else {
			SDL_Log("unknown float option '%s'", var);
			ran_cmd = false;
		}
	} else {
		SDL_Log("unknown command syntax '%s'", line);
		ran_cmd = false;
	}
	if (ran_cmd) {
		SDL_Log("ran command successfully");
	} else {
		SDL_Log("couldn't run command '%s'", line);
	}
}

int mouse_pos_to_board_idx(const GameState * state, const DisplayInfo * info, SDL_FPoint * nullable_tile_pos) {
	int _x = (int)(state->mouse_pos.x / info->slot_width);
	int _y = (int)(state->mouse_pos.y / info->slot_width);
	if (_x >= 0 && _x < 8 && _y >= 0 && _y < 8) {
		if (nullable_tile_pos) {
			nullable_tile_pos->x = (float)_x * info->slot_width;
			nullable_tile_pos->y = (float)_y * info->slot_width;
		}
		return 8 * (7 - _y) + (7 - _x);
	}
	return -1;
}

Uint8 * find_piece_idx(int piece_idx, GameState * state) {
	for (int i = 0; i < state->n_piece_idxs; ++i) {
		if (state->piece_idxs[i] == piece_idx) {
			return state->piece_idxs + i;
		}
	}
	return NULL;
}

void print_error(void) {
	SDL_Log("%s", SDL_GetError());
}

int main(int argc, char ** argv) {
	(void)argc;
	(void)argv;
	set_stdin_nonblock();
	SDL_Log("unblocked stdin");
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
	DisplayInfo info;
	if (!get_display_information(&info)) {
		print_error();
		return 1;
	}
	SDL_Log("obtained display information");
	SDL_Window * window;
	SDL_Renderer * renderer;
	if (!SDL_CreateWindowAndRenderer("Chess Game", info.screen_dims.x, info.screen_dims.y, 0, &window, &renderer)) {
		print_error();
		return 1;
	}
	SDL_Log("initialized window and renderer");
	TextureCache cache;
	if (!load_texture_cache(renderer, &cache)) {
		print_error();
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		return 1;
	}
	SDL_Log("loaded texture cache");
	GameState state = {};
	state.board = beginning_board;
	memcpy(state.piece_idxs, beginning_piece_idxs, sizeof beginning_piece_idxs);
	state.n_piece_idxs = 32;
	state.moused_slot_idx = -1;
	SDL_Log("entering main loop");
	bool running = true;
	while (running) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_EVENT_QUIT:
				running = false;
				break;
			case SDL_EVENT_MOUSE_BUTTON_UP:
			case SDL_EVENT_MOUSE_BUTTON_DOWN:
				state.mouse_selected = event.button.down;
				if (event.button.down) {
					state.moused_slot_idx = -1;
					for (int i = 0; i < state.n_piece_idxs; ++i) {
						int piece_idx = state.piece_idxs[i];
						SDL_FRect rect = get_piece_display_rect(piece_idx, &info);
						if (SDL_PointInRectFloat(&state.mouse_pos, &rect)) {
							state.moused_slot_idx = piece_idx;
							state.moused_piece_offset = (SDL_FPoint) {
								state.mouse_pos.x - rect.x,
								state.mouse_pos.y - rect.y,
							};
							break;
						}
					}
				} else {
					state.mouse_release_event = true;
				}
				break;
			case SDL_EVENT_MOUSE_MOTION:
				state.mouse_pos.x = event.motion.x;
				state.mouse_pos.y = event.motion.y;
				break;
			default:
				break;
			}
		}

		if (state.mouse_release_event) {
			state.mouse_release_event = false;
			int mouse_idx = mouse_pos_to_board_idx(&state, &info, NULL);
			if (mouse_idx != -1) {
				Uint8 * idx_ptr = find_piece_idx(state.moused_slot_idx, &state);
				if (idx_ptr && mouse_idx != state.moused_slot_idx) {
					BoardSlot * src = &state.board.slots[state.moused_slot_idx];
					BoardSlot * dest = &state.board.slots[mouse_idx];
					Uint8 * dest_idx_ptr = find_piece_idx(mouse_idx, &state);
					if (dest_idx_ptr) { // capture
						*dest_idx_ptr = state.piece_idxs[state.n_piece_idxs - 1];
						--state.n_piece_idxs;
					}
					*dest = *src;
					*src = EMPTY_SLOT;
					*idx_ptr = mouse_idx;
				}
			}
			state.moused_slot_idx = -1;
		}

		if (state.moused_slot_idx != -1) {
			SDL_FPoint bg_pos;
			int bg_idx = mouse_pos_to_board_idx(&state, &info, &bg_pos);
			if (bg_idx != -1) {
				BoardSlot * bg_slot = &state.board.slots[bg_idx];
				SDL_FRect bg = {
					bg_pos.x,
					bg_pos.y,
					info.slot_width,
					info.slot_width,
				};
				SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
				if (bg_idx != state.moused_slot_idx && bg_slot->has_piece) {
					SDL_SetRenderDrawColor(renderer, 255, 0, 0, 127);
				} else {
					SDL_SetRenderDrawColor(renderer, 255, 255, 255, 220);
				}
				SDL_RenderFillRect(renderer, &bg);
			}
		}

		for (int _y = 0; _y < 8; ++_y) {
			for (int _x = 0; _x < 8; ++_x) {
				int piece_idx = _y * 8 + _x;
				if (piece_idx == state.moused_slot_idx)
					continue;
				const BoardSlot * slot = &state.board.slots[piece_idx];
				int x = 7 - _x;
				int y = 7 - _y;
				SDL_Texture * tex = lookup_board_slot_texture(&cache, slot);
				if (!tex) continue;
				SDL_FRect rect = { x * info.slot_width, y * info.slot_width + info.piece_slot_offset.y, info.piece_width,  info.piece_width };
				SDL_RenderTexture(renderer, tex, NULL, &rect);
			}
		}
		if (state.moused_slot_idx != -1) {
			SDL_FRect rect = {
				state.mouse_pos.x - state.moused_piece_offset.x,
				state.mouse_pos.y - state.moused_piece_offset.y,
				info.piece_width,
				info.piece_width,
			};
			BoardSlot * slot = &state.board.slots[state.moused_slot_idx];
			SDL_Texture * tex = lookup_board_slot_texture(&cache, slot);
			SDL_RenderTexture(renderer, tex, NULL, &rect);
		}
		SDL_RenderPresent(renderer);
		SDL_RenderTexture(renderer, cache.board, NULL, NULL);
		process_stdin(&state, &info);
	}
	free_texture_cache(&cache);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}
