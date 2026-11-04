#pragma once
#include <stdint.h>
#include <stddef.h>

#define NUM_SQUARES 64
#define WEIGHT_COUNT 393
#define MAX_MOVES 256
#define MAX_DEPTH 64
#define HISTORY_SIZE 1024

typedef enum { WHITE=0, BLACK=1 } Color;
typedef enum { PT_PAWN=0, PT_KNIGHT, PT_BISHOP, PT_ROOK, PT_QUEEN, PT_KING, PT_NONE=-1 } PieceType;

typedef struct { int from, to, promote; } Move;
typedef struct { Move moves[MAX_MOVES]; int count; } MoveList;

/* bitboards */
typedef uint64_t Bitboard;