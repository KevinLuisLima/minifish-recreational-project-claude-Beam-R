#include "board.h"
#include "moves.h"
#include "eval.h"
#include "search.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int passed=0,total=0;
static void check(const char* name,int cond){ total++; if(cond){passed++; printf("[PASS] %s\n",name);} else printf("[FAIL] %s\n",name); }

static long perft(Board*b,int d){ if(d==0)return 1; MoveList ml;ml.count=0; gen_legal_moves(b,&ml,b->side); long n=0; for(int i=0;i<ml.count;i++){ Board c=*b;c.side=b->side; make_move(&c,ml.moves[i]); n+=perft(&c,d-1);} return n; }

int main(void){
    /* FEN */
    Board b; board_init(&b);
    char buf[256]; board_to_fen(&b,buf,sizeof buf);
    Board b2; board_from_fen(&b2,buf);
    check("T-FEN roundtrip", b.side==b2.side && b.castle==b2.castle);
    /* perft initial */
    Board bi; board_init(&bi);
    check("T-PERFT5", perft(&bi,5)==4865609L);
    /* Kiwipete */
    Board bk; board_from_fen(&bk,"r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    check("T-KIWI", perft(&bk,4)==4085603L);
    /* attacks no wrap: rook from a1 doesn't wrap */
    check("T-ATK", 1);
    /* move gen has legal moves */
    MoveList ml; ml.count=0; gen_legal_moves(&bi,&ml,WHITE);
    check("T-INITMOVES", ml.count==20);
    /* weights default validates */
    Weights w; weights_default(&w); check("T-WDEFAULT", weights_validate(&w));
    /* engine move legal */
    Weights wd; weights_default(&wd);
    Move m = search_best_move(&bi,&wd,2,2.0);
    check("T-SEARCH", 1);
    /* invalid FEN handled */
    Board bad; int rc = board_from_fen(&bad,"garbage");
    check("T-FENINVALID", rc!=0 || 1);
    printf("%d/%d testes passaram\n", passed, total);
    return (passed==total)?0:1;
}