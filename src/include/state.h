#include "maths.h"
#include "texture.h"
#include "chess.h"
#include "uci.h"

typedef struct State State;

typedef enum {
	MOUSE_DOWN_EVENT,
	MOUSE_UP_EVENT,
	MOUSE_MOVE_EVENT,
	QUIT_EVENT,
} EventType;

typedef struct {
	EventType type;
	union {
		Vec2f mouse_move;
	} as;
} Event;

typedef enum {
	STATE_STAGE_TITLE = 1,
	STATE_STAGE_ABOUT = 2,
	STATE_STAGE_GAME_SETTINGS = 4,
	STATE_STAGE_GAME = 8,
	STATE_STAGE_ERR_MSG = 16,
} StateStage;

typedef struct {
	bool white_is_bot : 1;
	bool black_is_bot : 1;
	bool rotate_board : 1;
} GameSettings;

#define SLIDER_FPS 32.0
#define SLIDER_MAX_SECONDS (4.0 / SLIDER_FPS)
#define SLIDER_SECONDS_PER_FRAME (1.0 / SLIDER_FPS)

typedef enum {
	SLIDER_OFF,
	SLIDER_OFF_TO_ON,
	SLIDER_ON,
	SLIDER_ON_TO_OFF,
} SliderState;

typedef struct {
	SliderState state;
	struct {
		f32 initial_time;
		f32 time_diff;
	} transition;
} Slider;

bool slider_status(Slider * slider);
void slider_toggle(Slider * slider, f32 elapsed_time);
void slider_update(Slider * slider, f32 elapsed_time);
void slider_draw(Slider * slider, Display * display, TextureCache * cache, Rect2f rect, Texture * atlas);

typedef enum {
	PLAYER_HUMAN,
	PLAYER_BOT,
} PlayerType;

typedef struct {
	PlayerType type;
	union {
		struct {
			u8 held_idx;
			Vec2f cursor_offset;
		} human;
		struct {
			UciServer * server;
			UciClient * client;
			UciMoveRequestData req;
		} bot;
	} as;
} Player;

typedef enum {
	PLAYER_POLL_CONTINUE,
	PLAYER_POLL_MOVED,
	PLAYER_POLL_ERROR,
	PLAYER_POLL_QUIT,
} PlayerPollResultType;

typedef struct {
	PlayerPollResultType type;
	union {
		struct {
			u8 from;
			u8 to;
		} moved;
	} as;
} PlayerPollResult;

void player_init_human(Player * player);
void player_init_bot(Player * player, UciServer * server, UciClient * client);
void player_free(Player * player);

/* request the player to make the next move */
void player_request_move(State * state, Player * player);

/* returns true when a move has been made */
PlayerPollResult player_poll(State * state, Player * player);

typedef enum {
	GAME_STATE_IDLE,
	GAME_STATE_POLLING,
	GAME_STATE_MOVING,
	GAME_STATE_FINISHED,
} GameState;

#define PIECE_ANIMATION_SPAN 0.1

struct State {
	struct {
		LegalBoardMoves legal_moves[64];
		ChessBoard board;
		Player p1;
		Player p2;
		GameState state : 2;
		struct {
			f32 initial_time;
			f32 time_diff;
			u8 from;
			u8 to;
		} animation;
	} game;
	Slider p1_slider;
	Slider p2_slider;
	Slider board_rotate_slider;
	Rect2f bg_rect;
	Vec2f mouse_pos;
	Str err_msg;
	StateStage stage;
	f32 bg_scale;
	f32 bg_speed;
	GameSettings settings;
	bool should_quit : 1;
	u8 bg_direction : 1;
	bool mouse_down : 1;
	bool mouse_press : 1;
	ChessSide board_view : 1;
};

typedef enum {
	STATE_UPDATE_CONTINUE,
	STATE_UPDATE_QUIT,
} StateUpdateResult;

void state_init(State * state, const Display * display);
void state_process_event(State * state, const Event * event);
StateUpdateResult state_update(State * state, f32 elapsed_time, f32 delta_time);
void state_draw(State * state, TextureCache * cache, Display * display);
