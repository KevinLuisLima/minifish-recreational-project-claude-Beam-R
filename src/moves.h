#pragma once
#include "board.h"

void gen_pseudo_moves(const Board *b, MoveList *ml, Color side);
void gen_legal_moves(Board *b, MoveList *ml, Color side);
int has_legal_moves(Board *b, Color side);
int is_checkmate(const Board *b, Color side);
int is_stalemate(const Board *b, Color side);