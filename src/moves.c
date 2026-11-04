#include "moves.h"
#include "attacks.h"
#include <string.h>

static void add_move(MoveList *ml, int from, int to, int promo) {
    if (ml->count < MAX_MOVES) {
        ml->moves[ml->count].from=from;
        ml->moves[ml->count].to=to;
        ml->moves[ml->count].promote=promo;
        ml->count++;
    }
}

void gen_pseudo_moves(const Board *b, MoveList *ml, Color side) {
    ml->count=0;
    Color us=side, them=side^1;
    uint64_t occAll = b->all;
    uint64_t occUs = b->occ[us];
    uint64_t occThem = b->occ[them];
    /* pawns */
    Bitboard pawns = b->pieces[us][PT_PAWN];
    Bitboard bb = pawns;
    while (bb) {
        int s = __builtin_ctzll(bb); bb &= bb-1;
        int dir = (us==WHITE)? 8 : -8;
        int startRank = (us==WHITE)? 1 : 6;
        int promoRank = (us==WHITE)? 7 : 0;
        /* single push */
        int s1 = s + dir;
        if (s1>=0 && s1<64 && !(occAll & (1ULL<<s1))) {
            if (s/8 + (us==WHITE?1:-1) == promoRank) {
                add_move(ml,s,s1,PT_QUEEN); add_move(ml,s,s1,PT_ROOK); add_move(ml,s,s1,PT_BISHOP); add_move(ml,s,s1,PT_KNIGHT);
            } else {
                add_move(ml,s,s1,0);
                /* double push */
                if (s/8 == startRank) {
                    int s2 = s + 2*dir;
                    if (!(occAll & (1ULL<<s2))) add_move(ml,s,s2,0);
                }
            }
        }
        /* captures */
        uint64_t attacks = pawn_attacks(us, s);
        uint64_t targets = attacks & occThem;
        Bitboard t = targets;
        while (t) {
            int ts = __builtin_ctzll(t); t &= t-1;
            if (ts/8 == promoRank) {
                add_move(ml,s,ts,PT_QUEEN); add_move(ml,s,ts,PT_ROOK); add_move(ml,s,ts,PT_BISHOP); add_move(ml,s,ts,PT_KNIGHT);
            } else {
                add_move(ml,s,ts,0);
            }
        }
        /* en passant */
        if (b->ep >= 0 && (attacks & (1ULL<<b->ep))) {
            add_move(ml,s,b->ep,0);
        }
    }
    /* knights */
    bb = b->pieces[us][PT_KNIGHT];
    while (bb){ int s=__builtin_ctzll(bb); bb&=bb-1;
        uint64_t atk = knight_attacks(s) & ~occUs;
        Bitboard t=atk;
        while(t){ int ts=__builtin_ctzll(t); t&=t-1; add_move(ml,s,ts,0);}
    }
    /* bishops */
    bb = b->pieces[us][PT_BISHOP];
    while (bb){ int s=__builtin_ctzll(bb); bb&=bb-1;
        uint64_t atk = bishop_attacks(&occAll,s) & ~occUs;
        Bitboard t=atk;
        while(t){ int ts=__builtin_ctzll(t); t&=t-1; add_move(ml,s,ts,0);}
    }
    /* rooks */
    bb = b->pieces[us][PT_ROOK];
    while (bb){ int s=__builtin_ctzll(bb); bb&=bb-1;
        uint64_t atk = rook_attacks(&occAll,s) & ~occUs;
        Bitboard t=atk;
        while(t){ int ts=__builtin_ctzll(t); t&=t-1; add_move(ml,s,ts,0);}
    }
    /* queens */
    bb = b->pieces[us][PT_QUEEN];
    while (bb){ int s=__builtin_ctzll(bb); bb&=bb-1;
        uint64_t atk = queen_attacks(&occAll,s) & ~occUs;
        Bitboard t=atk;
        while(t){ int ts=__builtin_ctzll(t); t&=t-1; add_move(ml,s,ts,0);}
    }
    /* king */
    bb = b->pieces[us][PT_KING];
    if (bb){
        int s=__builtin_ctzll(bb);
        uint64_t atk = king_attacks(s) & ~occUs;
        Bitboard t=atk;
        while(t){ int ts=__builtin_ctzll(t); t&=t-1; add_move(ml,s,ts,0);}
        /* castling */
        int kingSq = (us==WHITE)?4:60;
        if (s==kingSq && !in_check((Board*)b, us)) {
            /* king side */
            if (us==WHITE){
                if ((b->castle&1) && !(occAll&(1ULL<<5)) && !(occAll&(1ULL<<6)) &&
                    !is_square_attacked(b,5,them) && !is_square_attacked(b,6,them)) add_move(ml,4,6,0);
                if ((b->castle&2) && !(occAll&(1ULL<<1)) && !(occAll&(1ULL<<2)) && !(occAll&(1ULL<<3)) &&
                    !is_square_attacked(b,2,them) && !is_square_attacked(b,3,them)) add_move(ml,4,2,0);
            } else {
                if ((b->castle&4) && !(occAll&(1ULL<<61)) && !(occAll&(1ULL<<62)) &&
                    !is_square_attacked(b,61,them) && !is_square_attacked(b,62,them)) add_move(ml,60,62,0);
                if ((b->castle&8) && !(occAll&(1ULL<<58)) && !(occAll&(1ULL<<57)) && !(occAll&(1ULL<<59)) &&
                    !is_square_attacked(b,58,them) && !is_square_attacked(b,59,them)) add_move(ml,60,58,0);
            }
        }
    }
}

void gen_legal_moves(Board *b, MoveList *ml, Color side) {
    MoveList pseudo; pseudo.count=0;
    gen_pseudo_moves(b,&pseudo,side);
    ml->count=0;
    for (int i=0;i<pseudo.count;i++){
        Move m=pseudo.moves[i];
        /* make on copy */
        Board copy = *b;
        /* ensure side */
        copy.side = side;
        /* but b already side? assume side matches b->side? */
        make_move(&copy,m);
        /* after move, side flipped; check if mover's king in check before? Actually need check current side not in check after? We checked mover not in check before via? filter: mover's king not attacked after move */
        /* Determine if the side that moved (side) is in check in resulting position => illegal. Since side to move now is them, check if `side` king attacked = is_square_attacked(&copy, kingSq, them) */
        /* Find king of side */
        Bitboard k = copy.pieces[side][PT_KING]; /* side has moved; king updated to copy pieces? make_move toggles side but pieces arrays still belong to side? yes */
        int ks = __builtin_ctzll(k);
        if (!is_square_attacked(&copy, ks, side^1)) {
            ml->moves[ml->count++]=m;
        }
    }
}

int has_legal_moves(Board *b, Color side) {
    MoveList ml; ml.count=0;
    /* set side temporarily */
    Board copy=*b; copy.side=side;
    gen_legal_moves(&copy,&ml,side);
    return ml.count>0;
}
int is_checkmate(const Board *b, Color side) {
    if (!in_check(b, side)) return 0;
    return !has_legal_moves((Board*)b, side);
}
int is_stalemate(const Board *b, Color side) {
    if (in_check(b, side)) return 0;
    return !has_legal_moves((Board*)b, side);
}