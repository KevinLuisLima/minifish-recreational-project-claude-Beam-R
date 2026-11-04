#pragma once
#include "types.h"

uint64_t pawn_attacks(Color c, int sq);
uint64_t knight_attacks(int sq);
uint64_t king_attacks(int sq);
uint64_t bishop_attacks(const Bitboard *occ, int sq);
uint64_t rook_attacks(const Bitboard *occ, int sq);
uint64_t queen_attacks(const Bitboard *occ, int sq);
uint64_t sliding_attacks(int sq, int dx, int dy, uint64_t occAll);
void init_attacks(void);