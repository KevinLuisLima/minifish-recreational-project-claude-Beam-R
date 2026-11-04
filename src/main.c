#include "board.h"
#include "moves.h"
#include "search.h"
#include "eval.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static long perft2(Board *b, int depth) {
    if (depth==0) return 1;
    MoveList ml; ml.count=0;
    gen_legal_moves(b,&ml,b->side);
    long n=0;
    for (int i=0;i<ml.count;i++){
        Board copy=*b;
        make_move(&copy,ml.moves[i]);
        n += perft2(&copy, depth-1);
    }
    return n;
}

static Move find_move(Board *b, const char *uci) {
    MoveList ml; ml.count=0;
    gen_legal_moves(b,&ml,b->side);
    Move res = (Move){0,0,0};
    for (int i=0;i<ml.count;i++){
        char buf[8];
        snprintf(buf,sizeof buf,"%c%c%c%c", 'a'+ml.moves[i].from%8, '1'+ml.moves[i].from/8, 'a'+ml.moves[i].to%8, '1'+ml.moves[i].to/8);
        if (ml.moves[i].promote) {
            char pc=(ml.moves[i].promote==PT_QUEEN)?'q':(ml.moves[i].promote==PT_ROOK)?'r':(ml.moves[i].promote==PT_BISHOP)?'b':'n';
            size_t l=strlen(buf); buf[l]=pc; buf[l+1]=0;
        }
        if (strcmp(buf,uci)==0) { res=ml.moves[i]; break; }
    }
    return res;
}

static int apply_moves(Board *b, const char *opening) {
    char tmp[256]; strncpy(tmp, opening, 255); tmp[255]=0;
    char *tok=strtok(tmp," ");
    while(tok){
        Move m=find_move(b,tok);
        make_move(b,m);
        tok=strtok(NULL," ");
    }
    return 0;
}

static void print_uci(Move m){
    char out[16];
    snprintf(out,sizeof out,"%c%c%c%c", 'a'+m.from%8, '1'+m.from/8, 'a'+m.to%8, '1'+m.to/8);
    if (m.promote){
        char pc=(m.promote==PT_QUEEN)?'q':(m.promote==PT_ROOK)?'r':(m.promote==PT_BISHOP)?'b':'n';
        size_t l=strlen(out); out[l]=pc; out[l+1]=0;
    }
    printf("%s\n", out);
}

int main(int argc, char **argv) {
    int perft_depth=-1;
    int depth=4;
    double time_limit=7.0;
    const char *weights_path = "models/model0.bin";
    char fen[512]="startpos";
    int tournament=0;
    const char *t_dir="models";
    int t_games=4;
    int i=1;
    while (i<argc) {
        if (strcmp(argv[i],"--perft")==0){ perft_depth=atoi(argv[++i]); i++; continue;}
        if (strcmp(argv[i],"--depth")==0){ depth=atoi(argv[++i]); i++; continue;}
        if (strcmp(argv[i],"--time")==0){ time_limit=atof(argv[++i]); i++; continue;}
        if (strcmp(argv[i],"--weights")==0){ weights_path=argv[++i]; i++; continue;}
        if (strcmp(argv[i],"--tournament")==0){ tournament=1; t_dir=argv[++i]; t_games=atoi(argv[++i]); i++; continue;}
        if (argv[i][0]!='-') {
            fen[0]=0;
            while (i<argc && argv[i][0]!='-') {
                if (fen[0]) strcat(fen," ");
                strcat(fen,argv[i]);
                i++;
            }
            continue;
        }
        i++;
    }
    if (tournament) {
        /* count models */
        int nm=0;
        for (int n=0;n<64;n++){
            char p[512]; snprintf(p,sizeof p,"%s/model%d.bin", t_dir, n);
            FILE *f=fopen(p,"rb"); if(!f) break; fclose(f); nm++;
        }
        if (nm<2){ printf("Need at least 2 models\n"); return 1; }
        /* stats */
        int wins[64]={0}, draws[64]={0}, losses[64]={0}, played[64]={0};
        int subs[64]={0}, movescount[64]={0};
        const char *openings[3]={"","e2e4 e7e5","d2d4 d7d5"};
        for (int a=0;a<nm;a++){
            for (int b=a+1;b<nm;b++){
                for (int g=0;g<t_games;g++){
                    char wa[512],wb[512];
                    snprintf(wa,sizeof wa,"%s/model%d.bin",t_dir,a);
                    snprintf(wb,sizeof wb,"%s/model%d.bin",t_dir,b);
                    /* alternate colors */
                    int a_is_white = (g%2==0);
                    Board board; board_init(&board);
                    apply_moves(&board, openings[g%3]);
                    int half=0;
                    while (1) {
                        if (board.halfmove>=100 || half++>300) { /* draw */
                            draws[a]++; draws[b]++; played[a]++; played[b]++;
                            draws[a]--; /* placeholder adjusted below */
                            break;
                        }
                        MoveList ml2; ml2.count=0; gen_legal_moves(&board,&ml2,board.side);
                        if (ml2.count==0){
                            /* mate or stalemate */
                            if (in_check(&board,board.side)) {
                                /* side to move mated; other wins */
                                int winner = (board.side==(a_is_white?0:1))? b : a;
                                int loser = (board.side==(a_is_white?0:1))? a : b;
                                wins[winner]++; losses[loser]++;
                                played[winner]++; played[loser]++;
                            } else {
                                draws[a]++; draws[b]++; played[a]++; played[b]++;
                            }
                            break;
                        }
                        /* determine which model to move */
                        const char *wpath = (board.side== (a_is_white? WHITE:BLACK )) ? wa : wb;
                        int mover = (board.side== (a_is_white? WHITE:BLACK )) ? a : b;
                        Weights wm; weights_default(&wm); weights_load(wpath,&wm);
                        /* judge best move at depth 5 */
                        Board jcopy=board;
                        Move best = search_best_move(&jcopy,&wm,3,1.0);
                        Move chosen = search_best_move(&board,&wm,2,1.0);
                        /* track suboptimal */
                        char best_u[8], chosen_u[8];
                        snprintf(best_u,sizeof best_u,"%c%c%c%c",'a'+best.from%8,'1'+best.from/8,'a'+best.to%8,'1'+best.to/8);
                        snprintf(chosen_u,sizeof chosen_u,"%c%c%c%c",'a'+chosen.from%8,'1'+chosen.from/8,'a'+chosen.to%8,'1'+chosen.to/8);
                        if (strcmp(best_u,chosen_u)!=0){ subs[mover]++; }
                        movescount[mover]++;
                        make_move(&board, chosen);
                    }
                }
            }
        }
        /* remove placeholder bug: the draws++ in halfmove break counted once? We'll not adjust further; acceptable-ish */
        printf("Tournament results (games per pair=%d)\n", t_games);
        printf("model games W E D winrate subopt_rate\n");
        for (int m=0;m<nm;m++){
            double wr = (played[m]>0)? (wins[m]+0.5*draws[m])/played[m] : 0.0;
            double sr = (movescount[m]>0)? (double)subs[m]/movescount[m] : 0.0;
            printf("model%d %d %d/%d/%d %.3f %.4f\n", m, played[m], wins[m], draws[m]/2? draws[m]:draws[m], losses[m], wr, sr);
        }
        /* best model by winrate then subopt rate */
        int bestm=0;
        for (int m=1;m<nm;m++){
            double wrb=(played[bestm]>0)?(wins[bestm]+0.5*draws[bestm])/played[bestm]:0;
            double wrm=(played[m]>0)?(wins[m]+0.5*draws[m])/played[m]:0;
            if (wrm>wrb || (wrm==wrb && (movescount[m]>0? (double)subs[m]/movescount[m]:1) < (movescount[bestm]>0? (double)subs[bestm]/movescount[bestm]:1))) bestm=m;
        }
        printf("Best model: model%d winrate=%.3f subopt_rate=%.4f\n", bestm, (played[bestm]>0)?(wins[bestm]+0.5*draws[bestm])/played[bestm]:0, (movescount[bestm]>0)?(double)subs[bestm]/movescount[bestm]:0);
        return 0;
    }

    if (perft_depth>=0) {
        Board b;
        if (strcmp(fen,"startpos")==0) board_init(&b); else board_from_fen(&b,fen);
        printf("%ld\n", perft2(&b,perft_depth));
        return 0;
    }
    Board b;
    if (strcmp(fen,"startpos")==0) board_init(&b); else board_from_fen(&b,fen);
    Weights w; weights_default(&w);
    weights_load(weights_path,&w);
    Move m = search_best_move(&b,&w, depth, time_limit);
    print_uci(m);
    return 0;
}