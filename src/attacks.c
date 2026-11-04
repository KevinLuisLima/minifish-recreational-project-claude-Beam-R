#include "attacks.h"
#include <stdlib.h>

static uint64_t knight_table[64];
static uint64_t king_table[64];
static int inited = 0;

static const uint64_t FILE_A = 0x0101010101010101ULL;
static const uint64_t FILE_H = 0x8080808080808080ULL;

uint64_t pawn_attacks(Color c, int sq) {
    if (sq < 0 || sq > 63) return 0;
    uint64_t bit = 1ULL << sq;
    if (c == WHITE) {
        return ((bit & ~FILE_A) << 7) | ((bit & ~FILE_H) << 9);
    } else {
        return ((bit & ~FILE_H) >> 7) | ((bit & ~FILE_A) >> 9);
    }
}

uint64_t knight_attacks(int sq) {
    if (!inited) init_attacks();
    return (sq>=0&&sq<64)?knight_table[sq]:0;
}
uint64_t king_attacks(int sq) {
    if (!inited) init_attacks();
    return (sq>=0&&sq<64)?king_table[sq]:0;
}

static int ray(int sq, int dx, int dy, uint64_t occ, uint64_t *res) {
    /* returns further? we build incrementally */
    (void)occ; (void)res; (void)sq; (void)dx; (void)dy;
    return 0;
}

static uint64_t slide(int sq, int dx, int dy, uint64_t blockers, int own_side_occ) {
    /* own_side_occ unused; blockers includes all pieces */
    (void)own_side_occ;
    uint64_t res = 0;
    int r = sq / 8, f = sq % 8;
    for (;;) {
        r += dy; f += dx;
        if (r < 0 || r > 7 || f < 0 || f > 7) break;
        int s = r*8 + f;
        res |= 1ULL << s;
        if (blockers & (1ULL << s)) break;
    }
    return res;
}

uint64_t bishop_attacks(const Bitboard *occ, int sq) {
    if (sq<0||sq>63) return 0;
    uint64_t blockers = occ ? *occ : 0;
    uint64_t r = 0;
    r |= slide(sq,1,1,blockers,0);
    r |= slide(sq,1,-1,blockers,0);
    r |= slide(sq,-1,1,blockers,0);
    r |= slide(sq,-1,-1,blockers,0);
    return r;
}
uint64_t rook_attacks(const Bitboard *occ, int sq) {
    if (sq<0||sq>63) return 0;
    uint64_t blockers = occ ? *occ : 0;
    uint64_t r = 0;
    r |= slide(sq,1,0,blockers,0);
    r |= slide(sq,-1,0,blockers,0);
    r |= slide(sq,0,1,blockers,0);
    r |= slide(sq,0,-1,blockers,0);
    return r;
}
uint64_t queen_attacks(const Bitboard *occ, int sq) {
    return bishop_attacks(occ,sq) | rook_attacks(occ,sq);
}

void init_attacks(void) {
    if (inited) return;
    /* knight */
    int kx[8]={1,2,2,1,-1,-2,-2,-1};
    int ky[8]={2,1,-1,-2,-2,-1,1,2};
    for (int s=0;s<64;s++){
        uint64_t m=0;
        int r=s/8,f=s%8;
        for (int i=0;i<8;i++){
            int rr=r+ky[i], ff=f+kx[i];
            if (rr>=0&&rr<8&&ff>=0&&ff<8) m|=1ULL<<(rr*8+ff);
        }
        knight_table[s]=m;
    }
    /* king */
    for (int s=0;s<64;s++){
        uint64_t m=0; int r=s/8,f=s%8;
        for (int dr=-1;dr<=1;dr++) for(int df=-1;df<=1;df++){
            if (dr==0&&df==0) continue;
            int rr=r+dr,ff=f+df;
            if (rr>=0&&rr<8&&ff>=0&&ff<8) m|=1ULL<<(rr*8+ff);
        }
        king_table[s]=m;
    }
    inited=1;
}