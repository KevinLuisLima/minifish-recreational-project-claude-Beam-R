#include "search.h"
#include <time.h>
#include <stdlib.h>

long search_nodes = 0;
static double start_time = 0;
static double time_limit = 5.0;
static int stop_search = 0;

static double now_sec(void){ return (double)clock()/CLOCKS_PER_SEC; }

static int quiesce(Board *b, const Weights *w, int alpha, int beta) {
    search_nodes++;
    if (now_sec()-start_time > time_limit) { stop_search=1; return 0; }
    int stand = eval_position(b,w);
    if (stand >= beta) return beta;
    if (alpha < stand) alpha = stand;
    return stand;
}

/* Full alpha-beta */
static int ab(Board *b, const Weights *w, int depth, int alpha, int beta) {
    if (stop_search) return 0;
    if (now_sec()-start_time > time_limit) { stop_search=1; return 0; }
    search_nodes++;
    /* terminal */
    if (b->halfmove >= 100) return 0;
    Color side = b->side;
    MoveList ml; ml.count=0;
    gen_legal_moves(b,&ml,side);
    if (ml.count==0) {
        if (in_check(b, side)) return -100000 + (MAX_DEPTH-depth); /* mate */
        return 0; /* stalemate */
    }
    if (depth <= 0) {
        return quiesce(b,w,alpha,beta); /* leaf eval */
    }
    /* move ordering not implemented */
    int best = -1000000;
    for (int i=0;i<ml.count;i++){
        Board copy = *b;
        make_move(&copy, ml.moves[i]);
        int val = -ab(&copy,w,depth-1,-beta,-alpha);
        if (stop_search) return 0;
        if (val > best) best = val;
        if (val > alpha) alpha = val;
        if (alpha >= beta) break;
    }
    return best;
}

Move search_best_move(Board *b, const Weights *w, int max_depth, double time_limit_sec) {
    start_time = now_sec();
    time_limit = (time_limit_sec>0)? time_limit_sec : 5.0;
    stop_search = 0;
    search_nodes = 0;
    Move best = (Move){0,0,0};
    /* single legal move shortcut */
    MoveList ml; ml.count=0;
    gen_legal_moves(b,&ml,b->side);
    if (ml.count==1) return ml.moves[0];
    int depth;
    for (depth=1; depth<=max_depth; depth++){
        stop_search=0;
        int alpha=-1000000,beta=1000000;
        Move curBest=ml.moves[0];
        int bestVal=-1000000;
        for (int i=0;i<ml.count;i++){
            Board copy=*b;
            make_move(&copy, ml.moves[i]);
            int val = -ab(&copy,w,depth-1,-beta,-alpha);
            if (stop_search) break;
            if (val > bestVal) { bestVal=val; curBest=ml.moves[i]; }
            if (val > alpha) alpha=val;
        }
        if (stop_search) break;
        best = curBest;
        /* time check */
        if (now_sec()-start_time > time_limit*0.7) break;
    }
    return best;
}

void search_set_time(double t){ time_limit=t; }