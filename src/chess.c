#include "include/chess.h"
#include "include/maths.h"
#include <SDL3/SDL_assert.h>
#include <SDL3/SDL_log.h>

static bool rel_pos_in_bounds(Vec2i relpos) {
	return relpos.x >= 0 && relpos.x < 8
		&& relpos.y >= 0 && relpos.y < 8;
}

static Vec2i idx_to_rel_pos(u8 idx, ChessSide side) {
	int x = idx % 8;
	int y = idx / 8;
	if (side == BLACK_SIDE) {
		x =  7 - x;
		y = 7 - y;
	}
	return vec2i_new(x, y);
}

static u8 rel_pos_to_idx(Vec2i relpos, ChessSide side) {
	if (side == BLACK_SIDE) {
		relpos.x = 7 - relpos.x;
		relpos.y = 7 - relpos.y;
	}
	return relpos.y * 8 + relpos.x;
}

static void transfer_to_slot(ChessBoard * board, u8 src, u8 dest) {
	board->slots[dest] = board->slots[src];
	board->slots[src] = EMPTY_SLOT;
}

static bool is_promoting_pawn_at_idx(ChessBoard * board, u8 idx) {
	const u8 white_promoting_y = 7;
	const u8 black_promoting_y = 0;
	BoardSlot * slot = &board->slots[idx];
	if (slot->piece != CHESS_PAWN) {
		return false;
	}
	u8 piece_y = idx / 8;
	if (slot->side == WHITE_SIDE) {
		return piece_y == white_promoting_y;
	} else {
		return piece_y == black_promoting_y;
	}
}

static bool is_free_idx(ChessBoard * board, u8 idx) {
	return !board->slots[idx].has_piece;
}

static bool is_enemy_idx(ChessBoard * board, u8 idx, ChessSide side) {
	return board->slots[idx].has_piece && board->slots[idx].side != side;
}

static bool is_free_or_enemy_idx(ChessBoard * board, u8 idx, ChessSide side) {
	return !board->slots[idx].has_piece || board->slots[idx].side != side;
}

/* INVARIANT: from is a CHESS_PAWN */
static bool is_pawn_enpassant_move(ChessBoard * board, u8 from, u8 to) {
	int diff = absi((int)from - (int)to);
	return diff != 16 && diff != 8 && !board->slots[to].has_piece;
}

static bool king_at_idx(ChessBoard * board, u8 idx, ChessSide side) {
	BoardSlot * slot = &board->slots[idx];
	return slot->piece == CHESS_KING && slot->side == side;
}

BoardMoveResult board_make_move_internal(ChessBoard * board, u8 from, u8 to) {
	BoardMoveResult result = {
		.from = from,
		.to = to,
		.last_opt_pawn = board->opt_pawn,
		.captured = to,
	};
	board->opt_pawn = INVALID_PIECE_IDX;
	if (board->slots[from].piece == CHESS_KING) {
		board->sides[board->side].king_idx = to;
	}
	if (king_at_idx(board, from, WHITE_SIDE) && from == INITIAL_WHITE_KING_IDX) {
		if (to == WHITE_KING_SIDE_CASTLE_IDX) {
			u8 new_rook_idx = WHITE_KING_SIDE_CASTLE_IDX + 1;
			transfer_to_slot(board, INITIAL_WHITE_KING_SIDE_ROOK_IDX, new_rook_idx);
			result.castle = true;
		} else if (to == WHITE_QUEEN_SIDE_CASTLE_IDX) {
			u8 new_rook_idx = WHITE_QUEEN_SIDE_CASTLE_IDX - 1;
			transfer_to_slot(board, INITIAL_WHITE_QUEEN_SIDE_ROOK_IDX, new_rook_idx);
			result.castle = true;
		}
		if (board->sides[WHITE_SIDE].ks_castle_ok) {
			board->sides[WHITE_SIDE].ks_castle_ok = false;
			result.cancelled_ks_castle = true;
		}
		if (board->sides[WHITE_SIDE].qs_castle_ok) {
			board->sides[WHITE_SIDE].qs_castle_ok = false;
			result.cancelled_qs_castle = true;
		}
	} else if (king_at_idx(board, from, BLACK_SIDE) && from == INITIAL_BLACK_KING_IDX) {
		if (to == BLACK_KING_SIDE_CASTLE_IDX) {
			u8 new_rook_idx = BLACK_KING_SIDE_CASTLE_IDX + 1;
			transfer_to_slot(board, INITIAL_BLACK_KING_SIDE_ROOK_IDX, new_rook_idx);
			result.castle = true;
		} else if (to == BLACK_QUEEN_SIDE_CASTLE_IDX) {
			u8 new_rook_idx = BLACK_QUEEN_SIDE_CASTLE_IDX - 1;
			transfer_to_slot(board, INITIAL_BLACK_QUEEN_SIDE_ROOK_IDX, new_rook_idx);
			result.castle = true;
		}
		if (board->sides[BLACK_SIDE].ks_castle_ok) {
			board->sides[BLACK_SIDE].ks_castle_ok = false;
			result.cancelled_ks_castle = true;
		}
		if (board->sides[BLACK_SIDE].qs_castle_ok) {
			board->sides[BLACK_SIDE].qs_castle_ok = false;
			result.cancelled_qs_castle = true;
		}
	} else if (board->sides[WHITE_SIDE].ks_castle_ok && from == INITIAL_WHITE_KING_SIDE_ROOK_IDX) {
		board->sides[WHITE_SIDE].ks_castle_ok = false;
		result.cancelled_ks_castle = true;
	} else if (board->sides[WHITE_SIDE].qs_castle_ok && from == INITIAL_WHITE_QUEEN_SIDE_ROOK_IDX) {
		board->sides[WHITE_SIDE].qs_castle_ok = false;
		result.cancelled_qs_castle = true;
	} else if (board->sides[BLACK_SIDE].ks_castle_ok && from == INITIAL_BLACK_KING_SIDE_ROOK_IDX) {
		board->sides[BLACK_SIDE].ks_castle_ok = false;
		result.cancelled_ks_castle = true;
	} else if (board->sides[BLACK_SIDE].qs_castle_ok && from == INITIAL_BLACK_QUEEN_SIDE_ROOK_IDX) {
		board->sides[BLACK_SIDE].qs_castle_ok = false;
		result.cancelled_qs_castle = true;
	} else if (board->slots[from].piece == CHESS_PAWN) {
		if (is_pawn_enpassant_move(board, from, to)) {
			result.en_passant = true;
			ChessSide side = board->side;
			Vec2i to_pos = idx_to_rel_pos(to, side);
			to_pos.y -= 1;
			result.captured = rel_pos_to_idx(to_pos, side);
		} else if (absi(from - to) == 16) {
			board->opt_pawn = to;
		}
	}
	if (board->slots[result.captured].has_piece) {
		result.capture = true;
		result.piece = board->slots[to].piece;
		board->slots[result.captured] = EMPTY_SLOT;
	}
	transfer_to_slot(board, from, to);
	if (is_promoting_pawn_at_idx(board, to)) {
		result.promotion = true;
	}
	return result;
}

BoardMoveResult board_make_move(ChessBoard * board, u8 from, u8 to) {
	BoardMoveResult result = board_make_move_internal(board, from, to);
	if (board->side == BLACK_SIDE)
		++board->turn_count;
	if (result.capture || board->slots[result.to].piece == CHESS_PAWN)
		board->fifty_mv_rule = 0;
	else
		++board->fifty_mv_rule;
	board->side ^= 1;
	return result;
}

void board_unmake_move_internal(ChessBoard * board, BoardMoveResult last_move) {
	board->opt_pawn = last_move.last_opt_pawn;
	transfer_to_slot(board, last_move.to, last_move.from);
	if (board->slots[last_move.from].piece == CHESS_KING) {
		board->sides[board->side].king_idx = last_move.from;
	}
	if (last_move.capture) {
		ChessSide side = board->side == WHITE_SIDE ? BLACK_SIDE : WHITE_SIDE;
		board->slots[last_move.captured] = (BoardSlot){
			.side = side,
			.has_piece = true,
			.piece = last_move.piece,
		};
	}
	if (last_move.promotion) {
		board->slots[last_move.from].piece = CHESS_PAWN;
	}
	if (last_move.castle) {
		if (last_move.to == WHITE_KING_SIDE_CASTLE_IDX) {
			transfer_to_slot(board, WHITE_KING_SIDE_CASTLE_IDX + 1, INITIAL_WHITE_KING_SIDE_ROOK_IDX);
		} else if (last_move.to == WHITE_QUEEN_SIDE_CASTLE_IDX) {
			transfer_to_slot(board, WHITE_QUEEN_SIDE_CASTLE_IDX - 1, INITIAL_WHITE_QUEEN_SIDE_ROOK_IDX);
		} else if (last_move.to == BLACK_KING_SIDE_CASTLE_IDX) {
			transfer_to_slot(board, BLACK_KING_SIDE_CASTLE_IDX + 1, INITIAL_BLACK_KING_SIDE_ROOK_IDX);
		} else if (last_move.to == BLACK_QUEEN_SIDE_CASTLE_IDX) {
			transfer_to_slot(board, BLACK_QUEEN_SIDE_CASTLE_IDX - 1, INITIAL_BLACK_QUEEN_SIDE_ROOK_IDX);
		}
	}
	if (last_move.cancelled_ks_castle) {
		board->sides[board->side].ks_castle_ok = true;
	}
	if (last_move.cancelled_qs_castle) {
		board->sides[board->side].qs_castle_ok = true;
	}
}

static bool king_in_bishop_LOS(const ChessBoard * const board, const Vec2i * kpos, const Vec2i * dpos, ChessSide side) {
	if (absi(dpos->x) != absi(dpos->y))
		return false;
	for (int i = 1; i < absi(dpos->x); ++i) {
		int x = i * getsigni(dpos->x) + kpos->x;
		int y = i * getsigni(dpos->y) + kpos->y;
		u8 path_idx = rel_pos_to_idx(vec2i_new(x, y), side);
		if (board->slots[path_idx].has_piece) {
			return false;
		}
	}
	return true;
}

static bool king_in_rook_LOS(const ChessBoard * const board, const Vec2i * kpos, const Vec2i * dpos, ChessSide side) {
	if (dpos->x == 0) {
		for (int i = 1; i < absi(dpos->y); ++i) {
			int x = kpos->x;
			int y = i * getsigni(dpos->y) + kpos->y;
			u8 path_idx = rel_pos_to_idx(vec2i_new(x, y), side);
			if (board->slots[path_idx].has_piece) {
				return false;
			}
		}
		return true;
	}
	if (dpos->y == 0) {
		for (int i = 1; i < absi(dpos->x); ++i) {
			int y = kpos->y;
			int x = i * getsigni(dpos->x) + kpos->x;
			u8 path_idx = rel_pos_to_idx(vec2i_new(x, y), side);
			if (board->slots[path_idx].has_piece) {
				return false;
			}
		}
		return true;
	}
	return false;
}

bool board_has_checks(ChessBoard * const board, ChessSide const side) {
	u8 idx = board->sides[side].king_idx;
	const Vec2i kpos = idx_to_rel_pos(idx, side);
	for (u8 i = 0; i < 64; ++i) {
		BoardSlot * slot = &board->slots[i];
		if (!slot->has_piece)
			continue;
		if (slot->side == side)
			continue;
		const Vec2i opos = idx_to_rel_pos(i, side);
		const Vec2i dpos = vec2i_sub(opos, kpos);
		switch (slot->piece) {
		case CHESS_PAWN:
			if (dpos.y == 1 && absi(dpos.x) == 1)
				return true;
			goto no_check;
		case CHESS_KNIGHT: {
			for (u8 i = 0; i < 8; ++i) {
				const Vec2i ksquare = vec2i_add(opos, knight_offsets[i]);
				if (vec2i_equal(ksquare, kpos)) {
					return true;
				}
			}
			goto no_check;
		}
		case CHESS_BISHOP:
			if (king_in_bishop_LOS(board, &kpos, &dpos, side))
				return true;
			goto no_check;
		case CHESS_ROOK:
			if (king_in_rook_LOS(board, &kpos, &dpos, side))
				return true;
			goto no_check;
		case CHESS_QUEEN:
			if (king_in_bishop_LOS(board, &kpos, &dpos, side)
				|| king_in_rook_LOS(board, &kpos, &dpos, side))
				return true;
			goto no_check;
		case CHESS_KING:
			if (absi(dpos.x <= 1) && absi(dpos.y <= 1))
				return true;
			goto no_check;
		}
no_check:
		continue;
	}
	return false;
}

static void try_add_move(ChessBoard * board, LegalBoardMoves * moves, u8 from, u8 to, ChessSide side) {
	BoardMoveResult move = board_make_move_internal(board, from, to);
	bool king_in_danger = board_has_checks(board, side);
	board_unmake_move_internal(board, move);
	if (!king_in_danger) {
		legal_board_moves_add_index(moves, to);
	}
}

LegalBoardMoves pawn_moves(ChessBoard * board, u8 from) {
	ChessSide side = board->slots[from].side;
	Vec2i rel_pos = idx_to_rel_pos(from, side);
	Vec2i fwd_pos = vec2i_new(rel_pos.x, rel_pos.y + 1);
	LegalBoardMoves moves = 0;
	if (rel_pos_in_bounds(fwd_pos)) {
		u8 to = rel_pos_to_idx(fwd_pos, side);
		if (is_free_idx(board, to)) {
			try_add_move(board, &moves, from, to, side);
			if (rel_pos.y == 1) { /* pawn hasn't moved yet */
				u8 to = rel_pos_to_idx(vec2i_new(rel_pos.x, rel_pos.y + 2), side);
				if (is_free_idx(board, to)) {
					try_add_move(board, &moves, from, to, side);
				}
			}
		}
		if (to % 8 != 0) {
			u8 ep_idx = rel_pos_to_idx(rel_pos, side) - 1;
			if (ep_idx == board->opt_pawn || is_enemy_idx(board, to - 1, side)) {
				try_add_move(board, &moves, from, to - 1, side);
			}
		}
		if (to % 8 != 7) {
			u8 ep_idx = rel_pos_to_idx(rel_pos, side) + 1;
			if (ep_idx == board->opt_pawn || is_enemy_idx(board, to + 1, side)) {
				try_add_move(board, &moves, from, to + 1, side);
			}
		}
	}
	return moves;
}

LegalBoardMoves knight_moves(ChessBoard * board, u8 from) {
	LegalBoardMoves moves = 0;
	ChessSide side = board->slots[from].side;
	Vec2i rel_pos = idx_to_rel_pos(from, side);
	for (int i = 0; i < 8; ++i) {
		Vec2i new_pos = vec2i_add(rel_pos, knight_offsets[i]);
		if (!rel_pos_in_bounds(new_pos))
			continue;
		u8 to = rel_pos_to_idx(new_pos, side);
		if (is_free_or_enemy_idx(board, to, side)) {
			try_add_move(board, &moves, from, to, side);
		}
	}
	return moves;
}

LegalBoardMoves bishop_moves(ChessBoard * board, u8 from) {
	LegalBoardMoves moves = 0;
	ChessSide side = board->slots[from].side;
	Vec2i rel_pos = idx_to_rel_pos(from, side);
	for (int x = rel_pos.x - 1, y = rel_pos.y - 1; x >= 0 && y >= 0; --x, --y) {
		u8 to = rel_pos_to_idx(vec2i_new(x, y), side);
		if (is_free_idx(board, to)) {
			try_add_move(board, &moves, from, to, side);
			continue;
		}
		if (is_enemy_idx(board, to, side)) {
			try_add_move(board, &moves, from, to, side);
		}
		break;
	}
	for (int x = rel_pos.x + 1, y = rel_pos.y - 1; x < 8 && y >= 0; ++x, --y) {
		u8 to = rel_pos_to_idx(vec2i_new(x, y), side);
		if (is_free_idx(board, to)) {
			try_add_move(board, &moves, from, to, side);
			continue;
		}
		if (is_enemy_idx(board, to, side)) {
			try_add_move(board, &moves, from, to, side);
		}
		break;
	}
	for (int x = rel_pos.x - 1, y = rel_pos.y + 1; x >= 0 && y < 8; --x, ++y) {
		u8 to = rel_pos_to_idx(vec2i_new(x, y), side);
		if (is_free_idx(board, to)) {
			try_add_move(board, &moves, from, to, side);
			continue;
		}
		if (is_enemy_idx(board, to, side)) {
			try_add_move(board, &moves, from, to, side);
		}
		break;
	}
	for (int x = rel_pos.x + 1, y = rel_pos.y + 1; x < 8 && y < 8; ++x, ++y) {
		u8 to = rel_pos_to_idx(vec2i_new(x, y), side);
		if (is_free_idx(board, to)) {
			try_add_move(board, &moves, from, to, side);
			continue;
		}
		if (is_enemy_idx(board, to, side)) {
			try_add_move(board, &moves, from, to, side);
		}
		break;
	}
	return moves;
}

LegalBoardMoves rook_moves(ChessBoard * board, u8 from) {
	LegalBoardMoves moves = 0;
	ChessSide side = board->slots[from].side;
	Vec2i rel_pos = idx_to_rel_pos(from, side);
	for (int x = rel_pos.x - 1; x >= 0; --x) {
		u8 to = rel_pos_to_idx(vec2i_new(x, rel_pos.y), side);
		if (is_free_idx(board, to)) {
			try_add_move(board, &moves, from, to, side);
			continue;
		}
		if (is_enemy_idx(board, to, side)) {
			try_add_move(board, &moves, from, to, side);
		}
		break;
	}
	for (int x = rel_pos.x + 1; x < 8; ++x) {
		u8 to = rel_pos_to_idx(vec2i_new(x, rel_pos.y), side);
		if (is_free_idx(board, to)) {
			try_add_move(board, &moves, from, to, side);
			continue;
		}
		if (is_enemy_idx(board, to, side)) {
			try_add_move(board, &moves, from, to, side);
		}
		break;
	}
	for (int y = rel_pos.y - 1; y >= 0; --y) {
		u8 to = rel_pos_to_idx(vec2i_new(rel_pos.x, y), side);
		if (is_free_idx(board, to)) {
			try_add_move(board, &moves, from, to, side);
			continue;
		}
		if (is_enemy_idx(board, to, side)) {
			try_add_move(board, &moves, from, to, side);
		}
		break;
	}
	for (int y = rel_pos.y + 1; y < 8; ++y) {
		u8 to = rel_pos_to_idx(vec2i_new(rel_pos.x, y), side);
		if (is_free_idx(board, to)) {
			try_add_move(board, &moves, from, to, side);
			continue;
		}
		if (is_enemy_idx(board, to, side)) {
			try_add_move(board, &moves, from, to, side);
		}
		break;
	}
	return moves;
}

LegalBoardMoves queen_moves(ChessBoard * board, u8 from) {
	return bishop_moves(board, from) | rook_moves(board, from);
}

LegalBoardMoves king_castle_moves(ChessBoard * board, u8 from, ChessSide side) {
	const u8 initial_ks_rook_idx = side == WHITE_SIDE ? INITIAL_WHITE_KING_SIDE_ROOK_IDX : INITIAL_BLACK_KING_SIDE_ROOK_IDX;
	const u8 initial_qs_rook_idx = side == WHITE_SIDE ? INITIAL_WHITE_QUEEN_SIDE_ROOK_IDX : INITIAL_BLACK_QUEEN_SIDE_ROOK_IDX;
	const u8 ks_castle_idx = side == WHITE_SIDE ? WHITE_KING_SIDE_CASTLE_IDX : BLACK_KING_SIDE_CASTLE_IDX;
	const u8 qs_castle_idx = side == WHITE_SIDE ? WHITE_QUEEN_SIDE_CASTLE_IDX : BLACK_QUEEN_SIDE_CASTLE_IDX;
	LegalBoardMoves moves = 0;
	if (board->sides[side].ks_castle_ok
		&& is_free_idx(board, initial_ks_rook_idx + 1)
		&& is_free_idx(board, initial_ks_rook_idx + 2)
		&& !board_has_checks(board, side)) {
		try_add_move(board, &moves, from, ks_castle_idx, side);
	}
	if (board->sides[side].qs_castle_ok
		&& is_free_idx(board, initial_qs_rook_idx - 1)
		&& is_free_idx(board, initial_qs_rook_idx - 2)
		&& is_free_idx(board, initial_qs_rook_idx - 3)
		&& !board_has_checks(board, side)) {
		try_add_move(board, &moves, from, qs_castle_idx, side);
	}
	return moves;
}

LegalBoardMoves king_moves(ChessBoard * board, u8 from) {
	LegalBoardMoves moves = 0;
	ChessSide side = board->slots[from].side;
	Vec2i rel_pos = idx_to_rel_pos(from, side);
	for (int i = 0; i < 8; ++i) {
		Vec2i new_pos = vec2i_add(rel_pos, king_offsets[i]);
		if (!rel_pos_in_bounds(new_pos))
			continue;
		u8 to = rel_pos_to_idx(new_pos, side);
		if (is_free_or_enemy_idx(board, to, side)) {
			try_add_move(board, &moves, from, to, side);
		}
	}
	moves |= king_castle_moves(board, from, side);
	return moves;
}

LegalBoardMoves board_get_legal_moves_for_piece(ChessBoard * board, u8 idx) {
	SDL_assert(idx != INVALID_PIECE_IDX);
	BoardSlot * slot = &board->slots[idx];
	SDL_assert(slot->has_piece == true);
	switch (slot->piece) {
		case CHESS_PAWN:
			return pawn_moves(board, idx);
		case CHESS_KNIGHT:
			return knight_moves(board, idx);
		case CHESS_BISHOP:
			return bishop_moves(board, idx);
		case CHESS_ROOK:
			return rook_moves(board, idx);
		case CHESS_QUEEN:
			return queen_moves(board, idx);
		case CHESS_KING:
			return king_moves(board, idx);
	}
}

const char * chess_piece_str(ChessPiece piece) {
	switch (piece) {
        case CHESS_PAWN:
			return "CHESS_PAWN";
        case CHESS_KNIGHT:
			return "CHESS_KNIGHT";
        case CHESS_BISHOP:
			return "CHESS_BISHOP";
        case CHESS_ROOK:
			return "CHESS_ROOK";
        case CHESS_QUEEN:
			return "CHESS_QUEEN";
        case CHESS_KING:
			return "CHESS_KING";
	}
}
