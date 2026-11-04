#include "eval.h"
#include "moves.h"
#include <math.h>

static int pst_index(PieceType pt, int sq) { return (int)pt * 64 + sq; }

static int count_doubled(const Board *b, Color c) {
    /* simplified: more than one pawn per file */
    int files[8]={0};
    Bitboard bb=b->pieces[c][PT_PAWN];
    while (bb){ int s=__builtin_ctzll(bb); bb&=bb-1; files[s%8]++; }
    int d=0;
    for (int f=0;f<8;f++) if (files[f]>1) d += files[f]-1;
    return d;
}
static int count_isolated(const Board *b, Color c) {
    int files[8]={0};
    Bitboard bb=b->pieces[c][PT_PAWN];
    while (bb){ int s=__builtin_ctzll(bb); bb&=bb-1; files[s%8]++; }
    int iso=0;
    for (int f=0;f<8;f++){
        if (files[f]>0 && (f==0||files[f-1]==0) && (f==7||files[f+1]==0)) iso+=files[f];
    }
    return iso;
}

int eval_position(const Board *b, const Weights *w) {
    int score=0; /* white perspective */
    /* material + PST */
    for (int c=0;c<2;c++){
        for (int t=0;t<6;t++){
            Bitboard bb = b->pieces[c][t];
            while (bb){
                int s=__builtin_ctzll(bb); bb&=bb-1;
                int sign = (c==WHITE)? 1 : -1;
                if (t!=PT_KING) {
                    /* material weight index 384..388 for pawn..queen */
                    if (t<5) score += sign * (int)w->w[384+t];
                }
                int idx = pst_index((PieceType)t, (c==WHITE)? s : (s^56));
                score += sign * (int)w->w[idx];
            }
        }
    }
    /* mobility */
    MoveList ml; ml.count=0;
    Board bcopy = *b;
    bcopy.side=WHITE;
    gen_legal_moves(&bcopy,&ml,WHITE);
    int wm = ml.count;
    ml.count=0;
    bcopy.side=BLACK;
    gen_legal_moves(&bcopy,&ml,BLACK);
    int bm = ml.count;
    score += (int)((wm-bm)*w->w[389]);
    /* doubled pawns */
    score += (int)(-w->w[390]*(count_doubled(b,WHITE)-count_doubled(b,BLACK)));
    /* isolated */
    score += (int)(-w->w[391]*(count_isolated(b,WHITE)-count_isolated(b,BLACK)));
    /* king safety: count enemy pieces attacking squares around king (simple) */
    /* skip advanced */
    /* relative */
    if (b->side == BLACK) score = -score;
    return score;
}