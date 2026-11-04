#pragma once
#include "board.h"

typedef struct { float w[WEIGHT_COUNT]; } Weights;

int weights_load(const char *path, Weights *out);
int weights_validate(const Weights *w);
void weights_default(Weights *out);
int eval_position(const Board *b, const Weights *w); /* centipawns relative side to move */