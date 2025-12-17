#include "include/chess.h"
#include "include/linalg.h"
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

u8 board_lookup_idx_idx(ChessBoard * board, u8 idx) {
	for (u8 i = 0; i < board->piece_count; ++i) {
		if (board->piece_idxs[i] == idx)
			return i;
	}
	return INVALID_PIECE_IDX;
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

BoardMoveResult board_make_move_internal(ChessBoard * board, u8 from, u8 * from_idx, u8 to) {
	BoardMoveResult result = {
		.from = from,
		.to = to,
		.piece_idx = *from_idx,
		.last_opt_pawn = board->opt_pawn,
		.captured = to,
	};
	board->opt_pawn = INVALID_PIECE_IDX;
	u8 to_idx = board_lookup_idx_idx(board, to);
	u8 captured_idx = to_idx;
	if (*from_idx == WHITE_KING_IDX_IDX && from == INITIAL_WHITE_KING_IDX) {
		if (to == WHITE_KING_SIDE_CASTLE_IDX) {
			u8 new_rook_idx = WHITE_KING_SIDE_CASTLE_IDX + 1;
			board->piece_idxs[INITIAL_WHITE_KING_SIDE_ROOK_IDX_IDX] = new_rook_idx;
			transfer_to_slot(board, INITIAL_WHITE_KING_SIDE_ROOK_IDX, new_rook_idx);
			result.castle = true;
		} else if (to == WHITE_QUEEN_SIDE_CASTLE_IDX) {
			u8 new_rook_idx = WHITE_QUEEN_SIDE_CASTLE_IDX - 1;
			board->piece_idxs[INITIAL_WHITE_QUEEN_SIDE_ROOK_IDX_IDX] = new_rook_idx;
			transfer_to_slot(board, INITIAL_WHITE_QUEEN_SIDE_ROOK_IDX, new_rook_idx);
			result.castle = true;
		}
		if (board->white_king_side_castle_ok) {
			board->white_king_side_castle_ok = false;
			result.cancelled_castle_king = true;
		}
		if (board->white_queen_side_castle_ok) {
			board->white_queen_side_castle_ok = false;
			result.cancelled_castle_queen = true;
		}
	} else if (*from_idx == BLACK_KING_IDX_IDX && from == INITIAL_BLACK_KING_IDX) {
		if (to == BLACK_KING_SIDE_CASTLE_IDX) {
			u8 new_rook_idx = BLACK_KING_SIDE_CASTLE_IDX + 1;
			board->piece_idxs[INITIAL_BLACK_KING_SIDE_ROOK_IDX_IDX] = new_rook_idx;
			transfer_to_slot(board, INITIAL_BLACK_KING_SIDE_ROOK_IDX, new_rook_idx);
			result.castle = true;
		} else if (to == BLACK_QUEEN_SIDE_CASTLE_IDX) {
			u8 new_rook_idx = BLACK_QUEEN_SIDE_CASTLE_IDX - 1;
			board->piece_idxs[INITIAL_BLACK_QUEEN_SIDE_ROOK_IDX_IDX] = new_rook_idx;
			transfer_to_slot(board, INITIAL_BLACK_QUEEN_SIDE_ROOK_IDX, new_rook_idx);
			result.castle = true;
		}
		if (board->black_king_side_castle_ok) {
			board->black_king_side_castle_ok = false;
			result.cancelled_castle_king = true;
		}
		if (board->black_queen_side_castle_ok) {
			board->black_queen_side_castle_ok = false;
			result.cancelled_castle_queen = true;
		}
	} else if (board->white_king_side_castle_ok && *from_idx == INITIAL_WHITE_KING_SIDE_ROOK_IDX_IDX) {
		board->white_king_side_castle_ok = false;
		result.cancelled_castle_king = true;
	} else if (board->white_queen_side_castle_ok && *from_idx == INITIAL_WHITE_QUEEN_SIDE_ROOK_IDX_IDX) {
		board->white_queen_side_castle_ok = false;
		result.cancelled_castle_queen = true;
	} else if (board->black_king_side_castle_ok && *from_idx == INITIAL_BLACK_KING_SIDE_ROOK_IDX_IDX) {
		board->black_king_side_castle_ok = false;
		result.cancelled_castle_king = true;
	} else if (board->black_queen_side_castle_ok && *from_idx == INITIAL_BLACK_QUEEN_SIDE_ROOK_IDX_IDX) {
		board->black_queen_side_castle_ok = false;
		result.cancelled_castle_queen = true;
	} else if (board->slots[from].piece == CHESS_PAWN) {
		if (is_pawn_enpassant_move(board, from, to)) {
			result.en_passant = true;
			ChessSide side = board->side;
			Vec2i to_pos = idx_to_rel_pos(to, side);
			to_pos.y -= 1;
			u8 acc_idx = rel_pos_to_idx(to_pos, side);
			captured_idx = board_lookup_idx_idx(board, acc_idx);
			SDL_assert(captured_idx != INVALID_PIECE_IDX);
			board->slots[acc_idx] = EMPTY_SLOT;
			result.captured = acc_idx;
		} else if (absi(from - to) == 16) {
			board->opt_pawn = to;
		}
	}
	board->piece_idxs[*from_idx] = to;
	if (captured_idx != INVALID_PIECE_IDX) {
		result.capture = true;
		result.piece = board->slots[to].piece;
		result.captured_idx = captured_idx;
		board->piece_idxs[captured_idx] = board->piece_idxs[--board->piece_count];
		if (board->piece_count == *from_idx) {
			*from_idx = to_idx;
		}
	}
	transfer_to_slot(board, from, to);
	if (is_promoting_pawn_at_idx(board, to)) {
		result.promotion = true;
	}
	return result;
}

BoardMoveResult board_make_move(ChessBoard * board, u8 from, u8 * from_idx, u8 to) {
	BoardMoveResult result = board_make_move_internal(board, from, from_idx, to);
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
	BoardSlot * from_slot = &board->slots[last_move.from];
	BoardSlot * to_slot = &board->slots[last_move.to];
	*from_slot = *to_slot;
	*to_slot = EMPTY_SLOT;
	if (last_move.capture) {
		BoardSlot * captured_slot = &board->slots[last_move.captured];
		ChessSide side = from_slot->side == WHITE_SIDE ? BLACK_SIDE : WHITE_SIDE;
		*captured_slot = (BoardSlot){
			.side = side,
			.has_piece = true,
			.piece = last_move.piece,
		};
		board->piece_idxs[last_move.captured_idx] = last_move.captured;
		++board->piece_count;
	}
	board->piece_idxs[last_move.piece_idx] = last_move.from;
	if (last_move.promotion) {
		from_slot->piece = CHESS_PAWN;
	}
	if (last_move.castle) {
		if (last_move.to == WHITE_KING_SIDE_CASTLE_IDX) {
			board->piece_idxs[INITIAL_WHITE_KING_SIDE_ROOK_IDX_IDX] = INITIAL_WHITE_KING_SIDE_ROOK_IDX;
			transfer_to_slot(board, WHITE_KING_SIDE_CASTLE_IDX + 1, INITIAL_WHITE_KING_SIDE_ROOK_IDX);
		} else if (last_move.to == WHITE_QUEEN_SIDE_CASTLE_IDX) {
			board->piece_idxs[INITIAL_WHITE_QUEEN_SIDE_ROOK_IDX_IDX] = INITIAL_WHITE_QUEEN_SIDE_ROOK_IDX;
			transfer_to_slot(board, WHITE_QUEEN_SIDE_CASTLE_IDX - 1, INITIAL_WHITE_QUEEN_SIDE_ROOK_IDX);
		} else if (last_move.to == BLACK_KING_SIDE_CASTLE_IDX) {
			board->piece_idxs[INITIAL_BLACK_KING_SIDE_ROOK_IDX_IDX] = INITIAL_BLACK_KING_SIDE_ROOK_IDX;
			transfer_to_slot(board, BLACK_KING_SIDE_CASTLE_IDX + 1, INITIAL_BLACK_KING_SIDE_ROOK_IDX);
		} else if (last_move.to == BLACK_QUEEN_SIDE_CASTLE_IDX) {
			board->piece_idxs[INITIAL_BLACK_QUEEN_SIDE_ROOK_IDX_IDX] = INITIAL_BLACK_QUEEN_SIDE_ROOK_IDX;
			transfer_to_slot(board, BLACK_QUEEN_SIDE_CASTLE_IDX - 1, INITIAL_BLACK_QUEEN_SIDE_ROOK_IDX);
		}
	}
	if (last_move.cancelled_castle_king) {
		if (from_slot->side == WHITE_SIDE) {
			board->white_king_side_castle_ok = true;
		} else {
			board->black_king_side_castle_ok = true;
		}
	}
	if (last_move.cancelled_castle_queen) {
		if (from_slot->side == WHITE_SIDE) {
			board->white_queen_side_castle_ok = true;
		} else {
			board->black_queen_side_castle_ok = true;
		}
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
	const u8 idx_idx = side == WHITE_SIDE ? WHITE_KING_IDX_IDX : BLACK_KING_IDX_IDX;
	const u8 idx = board->piece_idxs[idx_idx];
	const Vec2i kpos = idx_to_rel_pos(idx, side);
	for (u8 i = 0; i < board->piece_count; ++i) {
		u8 o_idx = board->piece_idxs[i];
		BoardSlot * slot = &board->slots[o_idx];
		if (slot->side == side)
			continue;
		const Vec2i opos = idx_to_rel_pos(o_idx, side);
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

static void try_add_move(ChessBoard * board, LegalBoardMoves * moves, u8 from, u8 from_idx, u8 to, ChessSide side) {
	u8 from_idx_cpy = from_idx;
	BoardMoveResult move = board_make_move_internal(board, from, &from_idx_cpy, to);
	bool king_in_danger = board_has_checks(board, side);
	board_unmake_move_internal(board, move);
	if (!king_in_danger) {
		legal_board_moves_add_index(moves, to);
	}
}

LegalBoardMoves pawn_moves(ChessBoard * board, u8 from, u8 from_idx) {
	ChessSide side = board->slots[from].side;
	Vec2i rel_pos = idx_to_rel_pos(from, side);
	Vec2i fwd_pos = vec2i_new(rel_pos.x, rel_pos.y + 1);
	LegalBoardMoves moves = 0;
	if (rel_pos_in_bounds(fwd_pos)) {
		u8 to = rel_pos_to_idx(fwd_pos, side);
		if (is_free_idx(board, to)) {
			try_add_move(board, &moves, from, from_idx, to, side);
			if (rel_pos.y == 1) { /* pawn hasn't moved yet */
				u8 to = rel_pos_to_idx(vec2i_new(rel_pos.x, rel_pos.y + 2), side);
				if (is_free_idx(board, to)) {
					try_add_move(board, &moves, from, from_idx, to, side);
				}
			}
		}
		if (to % 8 != 0) {
			u8 ep_idx = rel_pos_to_idx(rel_pos, side) - 1;
			if (ep_idx == board->opt_pawn || is_enemy_idx(board, to - 1, side)) {
				try_add_move(board, &moves, from, from_idx, to - 1, side);
			}
		}
		if (to % 8 != 7) {
			u8 ep_idx = rel_pos_to_idx(rel_pos, side) + 1;
			if (ep_idx == board->opt_pawn || is_enemy_idx(board, to + 1, side)) {
				try_add_move(board, &moves, from, from_idx, to + 1, side);
			}
		}
	}
	return moves;
}

LegalBoardMoves knight_moves(ChessBoard * board, u8 from, u8 from_idx) {
	LegalBoardMoves moves = 0;
	ChessSide side = board->slots[from].side;
	Vec2i rel_pos = idx_to_rel_pos(from, side);
	for (int i = 0; i < 8; ++i) {
		Vec2i new_pos = vec2i_add(rel_pos, knight_offsets[i]);
		if (!rel_pos_in_bounds(new_pos))
			continue;
		u8 to = rel_pos_to_idx(new_pos, side);
		if (is_free_or_enemy_idx(board, to, side)) {
			try_add_move(board, &moves, from, from_idx, to, side);
		}
	}
	return moves;
}

LegalBoardMoves bishop_moves(ChessBoard * board, u8 from, u8 from_idx) {
	LegalBoardMoves moves = 0;
	ChessSide side = board->slots[from].side;
	Vec2i rel_pos = idx_to_rel_pos(from, side);
	for (int x = rel_pos.x - 1, y = rel_pos.y - 1; x >= 0 && y >= 0; --x, --y) {
		u8 to = rel_pos_to_idx(vec2i_new(x, y), side);
		if (is_free_idx(board, to)) {
			try_add_move(board, &moves, from, from_idx, to, side);
			continue;
		}
		if (is_enemy_idx(board, to, side)) {
			try_add_move(board, &moves, from, from_idx, to, side);
		}
		break;
	}
	for (int x = rel_pos.x + 1, y = rel_pos.y - 1; x < 8 && y >= 0; ++x, --y) {
		u8 to = rel_pos_to_idx(vec2i_new(x, y), side);
		if (is_free_idx(board, to)) {
			try_add_move(board, &moves, from, from_idx, to, side);
			continue;
		}
		if (is_enemy_idx(board, to, side)) {
			try_add_move(board, &moves, from, from_idx, to, side);
		}
		break;
	}
	for (int x = rel_pos.x - 1, y = rel_pos.y + 1; x >= 0 && y < 8; --x, ++y) {
		u8 to = rel_pos_to_idx(vec2i_new(x, y), side);
		if (is_free_idx(board, to)) {
			try_add_move(board, &moves, from, from_idx, to, side);
			continue;
		}
		if (is_enemy_idx(board, to, side)) {
			try_add_move(board, &moves, from, from_idx, to, side);
		}
		break;
	}
	for (int x = rel_pos.x + 1, y = rel_pos.y + 1; x < 8 && y < 8; ++x, ++y) {
		u8 to = rel_pos_to_idx(vec2i_new(x, y), side);
		if (is_free_idx(board, to)) {
			try_add_move(board, &moves, from, from_idx, to, side);
			continue;
		}
		if (is_enemy_idx(board, to, side)) {
			try_add_move(board, &moves, from, from_idx, to, side);
		}
		break;
	}
	return moves;
}

LegalBoardMoves rook_moves(ChessBoard * board, u8 from, u8 from_idx) {
	LegalBoardMoves moves = 0;
	ChessSide side = board->slots[from].side;
	Vec2i rel_pos = idx_to_rel_pos(from, side);
	for (int x = rel_pos.x - 1; x >= 0; --x) {
		u8 to = rel_pos_to_idx(vec2i_new(x, rel_pos.y), side);
		if (is_free_idx(board, to)) {
			try_add_move(board, &moves, from, from_idx, to, side);
			continue;
		}
		if (is_enemy_idx(board, to, side)) {
			try_add_move(board, &moves, from, from_idx, to, side);
		}
		break;
	}
	for (int x = rel_pos.x + 1; x < 8; ++x) {
		u8 to = rel_pos_to_idx(vec2i_new(x, rel_pos.y), side);
		if (is_free_idx(board, to)) {
			try_add_move(board, &moves, from, from_idx, to, side);
			continue;
		}
		if (is_enemy_idx(board, to, side)) {
			try_add_move(board, &moves, from, from_idx, to, side);
		}
		break;
	}
	for (int y = rel_pos.y - 1; y >= 0; --y) {
		u8 to = rel_pos_to_idx(vec2i_new(rel_pos.x, y), side);
		if (is_free_idx(board, to)) {
			try_add_move(board, &moves, from, from_idx, to, side);
			continue;
		}
		if (is_enemy_idx(board, to, side)) {
			try_add_move(board, &moves, from, from_idx, to, side);
		}
		break;
	}
	for (int y = rel_pos.y + 1; y < 8; ++y) {
		u8 to = rel_pos_to_idx(vec2i_new(rel_pos.x, y), side);
		if (is_free_idx(board, to)) {
			try_add_move(board, &moves, from, from_idx, to, side);
			continue;
		}
		if (is_enemy_idx(board, to, side)) {
			try_add_move(board, &moves, from, from_idx, to, side);
		}
		break;
	}
	return moves;
}

LegalBoardMoves queen_moves(ChessBoard * board, u8 from, u8 from_idx) {
	return bishop_moves(board, from, from_idx) | rook_moves(board, from, from_idx);
}

LegalBoardMoves king_castle_moves(ChessBoard * board, u8 from, u8 from_idx, ChessSide side) {
	LegalBoardMoves moves = 0;
	if (side == WHITE_SIDE) {
		if (board->white_king_side_castle_ok
			&& is_free_idx(board, INITIAL_WHITE_KING_SIDE_ROOK_IDX + 1)
			&& is_free_idx(board, INITIAL_WHITE_KING_SIDE_ROOK_IDX + 2)
			&& !board_has_checks(board, side)) {
			try_add_move(board, &moves, from, from_idx, WHITE_KING_SIDE_CASTLE_IDX, side);
		}
		if (board->white_queen_side_castle_ok
			&& is_free_idx(board, INITIAL_WHITE_QUEEN_SIDE_ROOK_IDX - 1)
			&& is_free_idx(board, INITIAL_WHITE_QUEEN_SIDE_ROOK_IDX - 2)
			&& is_free_idx(board, INITIAL_WHITE_QUEEN_SIDE_ROOK_IDX - 3)
			&& !board_has_checks(board, side)) {
			try_add_move(board, &moves, from, from_idx, WHITE_QUEEN_SIDE_CASTLE_IDX, side);
		}
	} else {
		if (board->black_king_side_castle_ok
			&& is_free_idx(board, INITIAL_BLACK_KING_SIDE_ROOK_IDX + 1)
			&& is_free_idx(board, INITIAL_BLACK_KING_SIDE_ROOK_IDX + 2)
			&& !board_has_checks(board, side)) {
			try_add_move(board, &moves, from, from_idx, BLACK_KING_SIDE_CASTLE_IDX, side);
		}
		if (board->black_queen_side_castle_ok
			&& is_free_idx(board, INITIAL_BLACK_QUEEN_SIDE_ROOK_IDX - 1)
			&& is_free_idx(board, INITIAL_BLACK_QUEEN_SIDE_ROOK_IDX - 2)
			&& is_free_idx(board, INITIAL_BLACK_QUEEN_SIDE_ROOK_IDX - 3)
			&& !board_has_checks(board, side)) {
			try_add_move(board, &moves, from, from_idx, BLACK_QUEEN_SIDE_CASTLE_IDX, side);
		}
	}
	return moves;
}

LegalBoardMoves king_moves(ChessBoard * board, u8 from, u8 from_idx) {
	LegalBoardMoves moves = 0;
	ChessSide side = board->slots[from].side;
	Vec2i rel_pos = idx_to_rel_pos(from, side);
	for (int i = 0; i < 8; ++i) {
		Vec2i new_pos = vec2i_add(rel_pos, king_offsets[i]);
		if (!rel_pos_in_bounds(new_pos))
			continue;
		u8 to = rel_pos_to_idx(new_pos, side);
		if (is_free_or_enemy_idx(board, to, side)) {
			try_add_move(board, &moves, from, from_idx, to, side);
		}
	}
	moves |= king_castle_moves(board, from, from_idx, side);
	return moves;
}

LegalBoardMoves board_get_legal_moves_for_piece(ChessBoard * board, u8 piece_idx, u8 piece_idx_idx) {
	SDL_assert(piece_idx != INVALID_PIECE_IDX);
	BoardSlot * slot = &board->slots[piece_idx];
	SDL_assert(slot->has_piece == true);
	switch (slot->piece) {
		case CHESS_PAWN:
			return pawn_moves(board, piece_idx, piece_idx_idx);
		case CHESS_KNIGHT:
			return knight_moves(board, piece_idx, piece_idx_idx);
		case CHESS_BISHOP:
			return bishop_moves(board, piece_idx, piece_idx_idx);
		case CHESS_ROOK:
			return rook_moves(board, piece_idx, piece_idx_idx);
		case CHESS_QUEEN:
			return queen_moves(board, piece_idx, piece_idx_idx);
		case CHESS_KING:
			return king_moves(board, piece_idx, piece_idx_idx);
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
