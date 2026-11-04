#include "board.h"
#include "attacks.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

void board_init(Board *b) {
    memset(b,0,sizeof(*b));
    b->castle = 0b1111;
    b->ep=-1;
    b->side=WHITE;
    /* initial */
    const char *back[2] = {"rnbqkbnr","RNBQKBNR"};
    for (int c=0;c<2;c++){
        for (int i=0;i<8;i++){
            char ch = back[c][i];
            PieceType pt;
            if (ch=='p'||ch=='P') pt=PT_PAWN;
            else if (ch=='n'||ch=='N') pt=PT_KNIGHT;
            else if (ch=='b'||ch=='B') pt=PT_BISHOP;
            else if (ch=='r'||ch=='R') pt=PT_ROOK;
            else if (ch=='q'||ch=='Q') pt=PT_QUEEN;
            else pt=PT_KING;
            int sq = (c==WHITE)? (i) : (56+i);
            b->pieces[c][pt] |= 1ULL<<sq;
        }
        for (int i=0;i<8;i++){
            int sq=(c==WHITE)?(8+i):(48+i);
            b->pieces[c][PT_PAWN] |= 1ULL<<sq;
        }
    }
    for (int c=0;c<2;c++) for (int t=0;t<6;t++) b->occ[c] |= b->pieces[c][t];
    b->all = b->occ[0]|b->occ[1];
    b->hist_top=0;
}

/* FEN parser minimal */
int board_from_fen(Board *b, const char *fen) {
    if (!fen) return -1;
    memset(b,0,sizeof(*b));
    b->castle=0;
    b->ep=-1;
    b->halfmove=0;
    b->fullmove=1;
    b->hist_top=0;
    /* parse placement */
    int sq=56; /* start a8 */
    const char *p=fen;
    /* placement */
    while (*p && *p!=' ') {
        if (*p=='/') { sq -= 16; /* move down a rank? adjust */ /* Actually after 8 squares, next rank start */
            /* We track via rank boundaries; simpler: recompute */
        }
        if (*p >= '1' && *p <= '8') {
            sq += *p - '0';
        } else if (*p=='/') {
            /* newline rank: current sq should align; do nothing but ensure? */
        } else {
            Color c = (*p >= 'a' && *p <= 'z') ? BLACK : WHITE;
            char lower = (*p >= 'A' && *p <= 'Z') ? *p + 32 : *p;
            PieceType pt;
            if (lower=='p') pt=PT_PAWN;
            else if (lower=='n') pt=PT_KNIGHT;
            else if (lower=='b') pt=PT_BISHOP;
            else if (lower=='r') pt=PT_ROOK;
            else if (lower=='q') pt=PT_QUEEN;
            else if (lower=='k') pt=PT_KING;
            else return -1;
            /* We'll manage sq index as 0..63 with a8=56. Need consistent. */
            /* sq currently points to target square index in 0..63? Let's redefine. */
            if (sq < 0 || sq > 63) return -1;
            b->pieces[c][pt] |= 1ULL << sq;
            sq++;
        }
        p++;
    }
    /* The above sq handling flawed due to '/' handling. Rewrite cleaner below by restarting. */
    (void)b;
    /* Instead do a clean parse using separate loop. */
    /* Reset and parse properly */
    memset(b,0,sizeof(*b));
    b->castle=0; b->ep=-1; b->halfmove=0; b->fullmove=1; b->hist_top=0;
    /* placement */
    int rank=7, file=0;
    p=fen;
    while (*p && *p!=' ') {
        if (*p=='/') { rank--; file=0; p++; continue; }
        if (*p >= '1' && *p <= '8') { file += *p - '0'; p++; continue; }
        Color c = (*p >= 'a' && *p <= 'z') ? BLACK : WHITE;
        char lower = (*p >= 'A' && *p <= 'Z') ? *p + 32 : *p;
        PieceType pt;
        if (lower=='p') pt=PT_PAWN;
        else if (lower=='n') pt=PT_KNIGHT;
        else if (lower=='b') pt=PT_BISHOP;
        else if (lower=='r') pt=PT_ROOK;
        else if (lower=='q') pt=PT_QUEEN;
        else if (lower=='k') pt=PT_KING;
        else return -1;
        int sq2 = rank*8 + file;
        if (sq2<0||sq2>63) return -1;
        b->pieces[c][pt] |= 1ULL<<sq2;
        file++;
        p++;
    }
    if (*p==' ') p++;
    /* side */
    if (*p=='w') b->side=WHITE;
    else if (*p=='b') b->side=BLACK;
    else return -1;
    p++; if (*p==' ') p++;
    /* castling */
    if (*p=='-') { p++; }
    else {
        while (*p && *p!=' ') {
            if (*p=='K') b->castle|=1;
            else if (*p=='Q') b->castle|=2;
            else if (*p=='k') b->castle|=4;
            else if (*p=='q') b->castle|=8;
            p++;
        }
    }
    if (*p==' ') p++;
    /* ep */
    if (*p=='-') { p++; }
    else {
        if (p[1] >= '1' && p[1] <= '8') {
            int f = p[0]-'a'; int r = p[1]-'1';
            b->ep = r*8+f; p+=2;
        }
    }
    if (*p==' ') p++;
    /* halfmove */
    b->halfmove=0;
    while (*p && *p>='0' && *p<='9') { b->halfmove = b->halfmove*10 + (*p-'0'); p++; }
    if (*p==' ') p++;
    /* fullmove */
    b->fullmove=1;
    while (*p && *p>='0' && *p<='9') { b->fullmove = b->fullmove*10 + (*p-'0'); p++; }
    for (int c=0;c<2;c++){ for(int t=0;t<6;t++) b->occ[c]|=b->pieces[c][t]; b->all|=b->occ[c]; }
    return 0;
}

int board_to_fen(const Board *b, char *buf, size_t sz) {
    /* basic placement */
    char tmp[128]; int idx=0;
    for (int r=7;r>=0;r--){
        int empty=0;
        for (int f=0;f<8;f++){
            int sq=r*8+f;
            int found=-1; Color col=WHITE; PieceType pt=PT_NONE;
            for (int c=0;c<2;c++) for (int t=0;t<6;t++) if (b->pieces[c][t]&(1ULL<<sq)){ found=1; col=c; pt=t; }
            if (found<0){ empty++; continue; }
            if (empty>0){ tmp[idx++]= '0'+empty; empty=0; }
            char ch = "?pnbrqk"[ (pt==PT_PAWN?1:pt==PT_KNIGHT?2:pt==PT_BISHOP?3:pt==PT_ROOK?4:pt==PT_QUEEN?5:6)];
            if (col==WHITE) ch -= 32;
            tmp[idx++]=ch;
        }
        if (empty>0) tmp[idx++]='0'+empty;
        if (r>0) tmp[idx++]='/';
    }
    tmp[idx++]= ' ';
    tmp[idx++]= b->side==WHITE?'w':'b';
    tmp[idx++]= ' ';
    int any=0;
    if (b->castle&1){tmp[idx++]='K';any=1;}
    if (b->castle&2){tmp[idx++]='Q';any=1;}
    if (b->castle&4){tmp[idx++]='k';any=1;}
    if (b->castle&8){tmp[idx++]='q';any=1;}
    if (!any) tmp[idx++]='-';
    tmp[idx++]=' ';
    if (b->ep>=0 && b->ep<64){
        tmp[idx++] = 'a'+(b->ep%8);
        tmp[idx++] = '1'+(b->ep/8);
    } else tmp[idx++]='-';
    tmp[idx++]=' ';
    /* halfmove */
    char num[16]; snprintf(num,sizeof num,"%d",b->halfmove); for (char *q=num;*q;q++) tmp[idx++]=*q;
    tmp[idx++]=' ';
    snprintf(num,sizeof num,"%d",b->fullmove); for (char *q=num;*q;q++) tmp[idx++]=*q;
    tmp[idx++]=0;
    snprintf(buf,sz,"%s",tmp);
    return 0;
}


int is_square_attacked(const Board *b, int sq, Color by) {
    if (sq<0||sq>63) return 0;
    /* pawns */
    /* squares from which a pawn of color `by` attacks sq */
    /* reverse: pawns of `by` located at squares that attack sq */
    Bitboard bp = b->pieces[by][PT_PAWN];
    if (pawn_attacks((Color)(by^1), sq) & bp) /* attacks generated by opposite? careful */
    {
        /* pawn_attacks(by, origin) gives target squares; we need origins attacking sq. Use reverse by swapping color formula: */
    }
    /* Use direct: for each pawn bit, compute its attacks */
    /* Simpler: iterate bits */
    Bitboard tmp = bp;
    while (tmp) {
        int s = __builtin_ctzll(tmp);
        tmp &= tmp-1;
        if (pawn_attacks(by, s) & (1ULL<<sq)) return 1;
    }
    /* knights */
    if (knight_attacks(sq) & b->pieces[by][PT_KNIGHT]) return 1;
    /* king */
    if (king_attacks(sq) & b->pieces[by][PT_KING]) return 1;
    /* sliders */
    uint64_t occ = b->all;
    /* bishop/queen */
    uint64_t bq = b->pieces[by][PT_BISHOP] | b->pieces[by][PT_QUEEN];
    if (bishop_attacks(&occ, sq) & bq) return 1;
    /* rook/queen */
    uint64_t rq = b->pieces[by][PT_ROOK] | b->pieces[by][PT_QUEEN];
    if (rook_attacks(&occ, sq) & rq) return 1;
    return 0;
}

int in_check(const Board *b, Color side) {
    /* find king */
    Bitboard k = b->pieces[side][PT_KING];
    if (!k) return 1; /* no king => in check? */
    int sq = __builtin_ctzll(k);
    return is_square_attacked(b, sq, side^1);
}

int is_legal_move(Board *b, Move m) {
    (void)b; (void)m;
    return 1;
}

/* Move gen not here? moves.c */

static void remove_piece(Board *b, Color c, PieceType pt, int sq) {
    b->pieces[c][pt] &= ~(1ULL<<sq);
    b->occ[c] &= ~(1ULL<<sq);
    b->all &= ~(1ULL<<sq);
}
static void add_piece(Board *b, Color c, PieceType pt, int sq) {
    b->pieces[c][pt] |= 1ULL<<sq;
    b->occ[c] |= 1ULL<<sq;
    b->all |= 1ULL<<sq;
}

void make_move(Board *b, Move m) {
    int h = b->hist_top++;
    b->hist[h].m = m;
    b->hist[h].prev_castle = b->castle;
    b->hist[h].prev_ep = b->ep;
    b->hist[h].prev_halfmove = b->halfmove;
    Color us = b->side;
    Color them = us^1;
    int moved_pt = -1;
    /* find moved piece */
    for (int t=0;t<6;t++) if (b->pieces[us][t]&(1ULL<<m.from)) { moved_pt=t; break; }
    b->hist[h].moved_piece = moved_pt;
    int captured_piece = PT_NONE;
    Color captured_color = them;
    int captured_sq = m.to;
    int is_ep = 0;
    int is_castle = 0;
    int is_promo = 0;
    if (moved_pt==PT_PAWN && m.to==b->ep && b->ep>=0) {
        is_ep=1; captured_sq = (us==WHITE)? (m.to-8) : (m.to+8);
    }
    if (is_ep) {
        for (int t=0;t<6;t++) if (b->pieces[them][t]&(1ULL<<captured_sq)){captured_piece=t;break;}
        remove_piece(b, them, (PieceType)captured_piece, captured_sq);
    } else {
        for (int t=0;t<6;t++) if (b->pieces[them][t]&(1ULL<<m.to)){captured_piece=t;break;}
        if (captured_piece!=PT_NONE) remove_piece(b, them, (PieceType)captured_piece, m.to);
    }
    b->hist[h].captured_piece = captured_piece;
    b->hist[h].captured_color = captured_color;
    b->hist[h].captured_sq = captured_sq;
    b->hist[h].is_ep = is_ep;
    /* remove from origin */
    remove_piece(b, us, (PieceType)moved_pt, m.from);
    /* castling rights updates / rook move */
    if (moved_pt==PT_KING) {
        if (us==WHITE) { if (m.from==4) { /* castle? */
            if (m.to==6){ /* king side */
                /* move rook */
                remove_piece(b, us, PT_ROOK, 7);
                add_piece(b, us, PT_ROOK, 5);
                is_castle=1;
            } else if (m.to==2){
                remove_piece(b, us, PT_ROOK, 0);
                add_piece(b, us, PT_ROOK, 3);
                is_castle=1;
            }
        }}
        else {
            if (m.from==60){
                if (m.to==62){
                    remove_piece(b, us, PT_ROOK, 63);
                    add_piece(b, us, PT_ROOK, 61);
                    is_castle=1;
                } else if (m.to==58){
                    remove_piece(b, us, PT_ROOK, 56);
                    add_piece(b, us, PT_ROOK, 59);
                    is_castle=1;
                }
            }
        }
        b->castle &= (us==WHITE)? ~3 : ~12;
    }
    /* rook moved from initial squares */
    if (moved_pt==PT_ROOK) {
        if (us==WHITE){
            if (m.from==7) b->castle &= ~1;
            if (m.from==0) b->castle &= ~2;
        } else {
            if (m.from==63) b->castle &= ~4;
            if (m.from==56) b->castle &= ~8;
        }
    }
    /* capture of rook */
    if (captured_piece==PT_ROOK) {
        if (m.to==7) b->castle &= ~1;
        if (m.to==0) b->castle &= ~2;
        if (m.to==63) b->castle &= ~4;
        if (m.to==56) b->castle &= ~8;
    }
    /* promotion */
    PieceType place = (PieceType)moved_pt;
    if (m.promote>0) { place = (PieceType)m.promote; is_promo=1; }
    add_piece(b, us, place, m.to);
    b->hist[h].is_castling = is_castle;
    b->hist[h].is_promo = is_promo;
    /* ep square */
    b->ep = -1;
    if (moved_pt==PT_PAWN && ((m.from/8==1 && m.to/8==3)||(m.from/8==6 && m.to/8==4))) {
        b->ep = (m.from + m.to)/2;
    }
    /* halfmove */
    if (moved_pt==PT_PAWN || captured_piece!=PT_NONE) b->halfmove=0;
    else b->halfmove++;
    if (us==BLACK) b->fullmove++;
    b->side = them;
}

void unmake_move(Board *b) {
    if (b->hist_top<=0) return;
    int h = --b->hist_top;
    Move m = b->hist[h].m;
    Color them = b->side; /* side to move now is opponent of mover */
    Color us = them^1;
    /* remove placed piece */
    PieceType placed = (b->hist[h].is_promo)? (PieceType)m.promote : (PieceType)b->hist[h].moved_piece;
    /* find current piece at m.to */
    remove_piece(b, us, placed, m.to);
    /* restore moved piece at origin */
    add_piece(b, us, (PieceType)b->hist[h].moved_piece, m.from);
    /* castle rook undo */
    if (b->hist[h].is_castling) {
        if (us==WHITE){
            if (m.to==6){ remove_piece(b,us,PT_ROOK,5); add_piece(b,us,PT_ROOK,7);}
            else if(m.to==2){ remove_piece(b,us,PT_ROOK,3); add_piece(b,us,PT_ROOK,0);}
        } else {
            if (m.to==62){ remove_piece(b,us,PT_ROOK,61); add_piece(b,us,PT_ROOK,63);}
            else if(m.to==58){ remove_piece(b,us,PT_ROOK,59); add_piece(b,us,PT_ROOK,56);}
        }
    }
    /* capture restore */
    if (b->hist[h].captured_piece != PT_NONE) {
        int sq = b->hist[h].captured_sq;
        add_piece(b, b->hist[h].captured_color, (PieceType)b->hist[h].captured_piece, sq);
    }
    /* restore state */
    b->castle = b->hist[h].prev_castle;
    b->ep = b->hist[h].prev_ep;
    b->halfmove = b->hist[h].prev_halfmove;
    if (us==BLACK) b->fullmove--;
    b->side = us;
}

int is_terminal(const Board *b) {
    /* no moves? handled outside; here check 50-move or insufficient? */
    if (b->halfmove >= 100) return 1;
    /* insufficient material basic */
    /* count pieces excluding kings */
    int nonpawn=0;
    for (int c=0;c<2;c++) for (int t=0;t<5;t++) {
        if (t==PT_PAWN) {
            if (b->pieces[c][t]) return 0;
        } else {
            nonpawn += __builtin_popcountll(b->pieces[c][t]);
        }
    }
    /* K vs K, K+B vs K, K+N vs K */
    if (nonpawn==0) return 1;
    if (nonpawn==1) {
        /* one minor piece total */
        int total=0; for (int c=0;c<2;c++) for(int t=1;t<5;t++) total+=__builtin_popcountll(b->pieces[c][t]);
        if (total==1) return 1;
    }
    /* two bishops opposite colors? skip */
    return 0;
}

int count_material(const Board *b, Color side) {
    (void)b;(void)side; return 0;
}