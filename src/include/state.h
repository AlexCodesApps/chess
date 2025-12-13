#include "chess.h"
#include "linalg.h"
#include "texture.h"

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
		ChessPiece promoting_piece;
	} as;
} Event;

typedef enum {
	PROMOTION_DIALOG_NONE,
	PROMOTION_DIALOG_KNIGHT,
	PROMOTION_DIALOG_BISHOP,
	PROMOTION_DIALOG_ROOK,
	PROMOTION_DIALOG_QUEEN,
} PromotionDialogChoice;

typedef struct {
	Rect2f rect;
	Rect2f knight_rect;
	Rect2f bishop_rect;
	Rect2f rook_rect;
	Rect2f queen_rect;
	f32 margin;
	f32 pad;
	PromotionDialogChoice choice;
} PromotionDialog;

typedef struct {
	ChessBoard board;
	LegalBoardMoves legal_moves[32];
	PromotionDialog promotion_dialog;
	Rect2f board_rect;
	f32 slot_width;
	f32 piece_width;
	Vec2f mouse_location;
	Vec2f held_piece_offset; /* access when mouse is down and not promoting */
	u8 held_piece_idx; /* has value when mouse isn't down which indicates the mouse was just released */
	u8 held_piece_idx_idx; /* access when mouse is down and not promoting */
	ChessSide view : 1;
	ChessSide side : 1;
	bool promoting : 1; /* turned off when promoting piece is specified */
	bool mouse_is_down : 1;
	bool did_quit : 1;
} State;

typedef enum {
	STATE_UPDATE_CONTINUE,
	STATE_UPDATE_QUIT,
} StateUpdateResult;

void state_init(State * state, const Display * display);
void state_reset_board_rect(State * state, Rect2f new_rect);
void state_process_event(State * state, const Event * event);
StateUpdateResult state_update(State * stateplay);
void state_draw(State * state, TextureCache * cache, Display * display);
