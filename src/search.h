#pragma once
#include "board.h"
#include "moves.h"
#include "eval.h"

extern long search_nodes;

Move search_best_move(Board *b, const Weights *w, int max_depth, double time_limit_sec);
void search_set_time(double t);