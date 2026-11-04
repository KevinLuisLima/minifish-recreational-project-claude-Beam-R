#pragma once
#include "types.h"

typedef struct {
    Bitboard pieces[2][6]; /* color, piece */
    Bitboard occ[2];
    Bitboard all;
    Color side;
    int castle; /* bits: 1 wK,2 wQ,4 bK,8 bQ */
    int ep; /* square or -1 */
    int halfmove;
    int fullmove;
    /* history */
    struct {
        Move m;
        int moved_piece;
        int captured_piece;
        Color captured_color;
        int captured_sq;
        int prev_castle;
        int prev_ep;
        int prev_halfmove;
        int is_castling;
        int is_ep;
        int is_promo;
    } hist[HISTORY_SIZE];
    int hist_top;
} Board;

void board_init(Board *b);
int board_from_fen(Board *b, const char *fen);
int board_to_fen(const Board *b, char *buf, size_t sz);
Bitboard attacks_for_piece(const Board *b, Color side); /* not used */
int is_square_attacked(const Board *b, int sq, Color by);
int in_check(const Board *b, Color side);
int is_legal_move(Board *b, Move m);

void make_move(Board *b, Move m);
void unmake_move(Board *b);

int is_terminal(const Board *b); /* mate/stalemate/50 */
int count_material(const Board *b, Color side);