#pragma once
#include "ints.h"
#include "maths.h"

#define INVALID_PIECE_IDX ((u8)255)

typedef enum : u8 {
	CHESS_PAWN,
	CHESS_KNIGHT,
	CHESS_BISHOP,
	CHESS_ROOK,
	CHESS_QUEEN,
	CHESS_KING,
} ChessPiece;

typedef enum : bool {
	WHITE_SIDE,
	BLACK_SIDE,
} ChessSide;

typedef struct {
	bool has_piece : 1;
	ChessSide side : 1;
	ChessPiece piece : 6;
} BoardSlot;

typedef struct {
	bool capture : 1;
	bool castle : 1;
	bool promotion : 1;
	bool en_passant : 1; /* offsets the piece restoration slot when unmaking move */
	bool cancelled_qs_castle : 1;
	bool cancelled_ks_castle : 1;
	ChessPiece piece : 3;
	u8 from;
	u8 to;
	u8 captured;
	u8 last_opt_pawn;
} BoardMoveResult;

typedef struct {
	BoardSlot slots[64];
	usize fifty_mv_rule;
	usize turn_count;
	u8 opt_pawn;
	/* determines whether the possibility of castling is still there */
	ChessSide side : 1;
	struct {
		bool qs_castle_ok : 1;
		bool ks_castle_ok : 1;
		u8 king_idx;
	} sides[2];
} ChessBoard;

#define EMPTY_SLOT ((BoardSlot) { .has_piece = 0 })

#define W(PIECE) (BoardSlot) { \
	.has_piece = 1, \
	.side = WHITE_SIDE, \
	.piece = CHESS_ ## PIECE, \
}
#define E EMPTY_SLOT
#define B(PIECE) (BoardSlot) { \
	.has_piece = 1, \
	.side = BLACK_SIDE, \
	.piece = CHESS_ ## PIECE, \
}

static const ChessBoard INITIAL_CHESS_BOARD = {
	.slots = {
		W(ROOK), W(KNIGHT), W(BISHOP), W(KING), W(QUEEN), W(BISHOP), W(KNIGHT), W(ROOK),
		W(PAWN), W(PAWN), W(PAWN), W(PAWN), W(PAWN), W(PAWN), W(PAWN), W(PAWN),
		E, E, E, E, E, E, E, E,
		E, E, E, E, E, E, E, E,
		E, E, E, E, E, E, E, E,
		E, E, E, E, E, E, E, E,
		B(PAWN), B(PAWN), B(PAWN), B(PAWN), B(PAWN), B(PAWN), B(PAWN), B(PAWN),
		B(ROOK), B(KNIGHT), B(BISHOP), B(KING), B(QUEEN), B(BISHOP), B(KNIGHT), B(ROOK),
	},
	.turn_count = 1,
	.fifty_mv_rule = 0,
	.opt_pawn = INVALID_PIECE_IDX,
	.side = WHITE_SIDE,
	.sides = {
		[0] = {
			.ks_castle_ok = true,
			.qs_castle_ok = true,
			.king_idx = 3,
		},
		[1] = {
			.ks_castle_ok = true,
			.qs_castle_ok = true,
			.king_idx = 59,
		},
	},
};

#undef W
#undef E
#undef B

/* These are only valid when the kings haven't moved */
#define INITIAL_WHITE_KING_IDX 3
#define INITIAL_BLACK_KING_IDX 59

/* The rook indexes here aren't guaranteed to be constant,
 * but they are only used to determine castling eligibility
 * which can't be regained
 * so their validity is to be book kept anyways
 */
#define INITIAL_WHITE_KING_SIDE_ROOK_IDX 0
#define INITIAL_WHITE_QUEEN_SIDE_ROOK_IDX 7

#define INITIAL_BLACK_KING_SIDE_ROOK_IDX 56
#define INITIAL_BLACK_QUEEN_SIDE_ROOK_IDX 63

#define WHITE_KING_SIDE_CASTLE_IDX 1
#define WHITE_QUEEN_SIDE_CASTLE_IDX 5

#define BLACK_KING_SIDE_CASTLE_IDX 57
#define BLACK_QUEEN_SIDE_CASTLE_IDX 61

static const Vec2i knight_offsets[8] = {
	{ 2, 1 }, { 2, -1 }, { -2, 1 }, { -2, -1 },
	{ 1, 2 }, { 1, -2 }, { -1, 2 }, { -1, -2 },
};

static const Vec2i king_offsets[8] = {
	{ 1, 1 }, { 1, -1 }, { -1, 1 }, { -1, -1 },
	{ 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 },
};

typedef u64 LegalBoardMoves;

static void legal_board_moves_add_index(LegalBoardMoves * moves, u8 idx) {
	*moves |= ((u64)1 << idx);
}

static bool legal_board_moves_contains_idx(LegalBoardMoves moves, u8 idx) {
	return (moves & ((u64)1 << idx)) != 0;
}

bool board_has_checks(ChessBoard * board, ChessSide side);

/* INVARIANT: from != to */
BoardMoveResult board_make_move_internal(ChessBoard * board, u8 from, u8 to);
/* INVARIANT: from != to */
BoardMoveResult board_make_move(ChessBoard * board, u8 from, u8 to);

void board_set_promotion_type(ChessBoard * board, ChessPiece piece);

void board_unmake_move_internal(ChessBoard * board, BoardMoveResult last_move);

/* INVARIANT: Index must be to actual piece */
/* INVARIANT: Kings should never be capturable or corruption of state occurs */
LegalBoardMoves board_get_legal_moves_for_piece(ChessBoard * board, u8 idx);

const char * chess_piece_str(ChessPiece piece);
