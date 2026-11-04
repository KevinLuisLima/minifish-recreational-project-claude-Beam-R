#include "eval.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static uint32_t rd_u32(const unsigned char *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1]<<8) | ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24);
}
static float rd_f32(const unsigned char *p) {
    uint32_t u = rd_u32(p);
    float f;
    memcpy(&f,&u,sizeof(f));
    return f;
}

int weights_load(const char *path, Weights *out) {
    if (!path||!out) return -1;
    FILE *f = fopen(path,"rb");
    if (!f) return -1;
    fseek(f,0,SEEK_END);
    long sz = ftell(f);
    fseek(f,0,SEEK_SET);
    if (sz < 12) { fclose(f); return -1; }
    unsigned char hdr[12];
    if (fread(hdr,1,12,f)!=12){fclose(f);return -1;}
    if (memcmp(hdr,"CHSS",4)!=0){fclose(f);return -1;}
    uint32_t ver = rd_u32(hdr+4);
    if (ver != 1) { fclose(f); return -1; }
    uint32_t n = rd_u32(hdr+8);
    if (n != WEIGHT_COUNT) { fclose(f); return -1; }
    size_t expected = 12 + (size_t)n*4;
    if ((long)expected != sz) { fclose(f); return -1; }
    size_t rd = fread(out->w,4,n,f);
    if (rd != n) { fclose(f); return -1; }
    /* convert bytes to little-endian floats (assumes platform little? ensure) */
    /* We read raw bytes into float array; if platform not little-endian issues; assume little. */
    /* But floats may be in native order; rd_f32 safer: re-read */
    fseek(f,12,SEEK_SET);
    for (int i=0;i<(int)n;i++){
        unsigned char buf[4];
        if (fread(buf,1,4,f)!=4){fclose(f);return -1;}
        out->w[i]=rd_f32(buf);
    }
    fclose(f);
    if (!weights_validate(out)) return -1;
    return 0;
}

int weights_validate(const Weights *w) {
    if (!w) return 0;
    for (int i=0;i<WEIGHT_COUNT;i++){
        if (!isfinite(w->w[i])) return 0;
    }
    return 1;
}
void weights_default(Weights *out) {
    if(!out) return;
    /* reasonable defaults */
    for (int i=0;i<WEIGHT_COUNT;i++) out->w[i]=0.0f;
    /* PST material defaults: set all PST zero; material weights at 384.. */
    float mat[5] = {100,320,330,500,900};
    for (int i=0;i<5;i++) out->w[384+i]=mat[i];
    /* mobility etc */
    out->w[389]=10.0f;
    out->w[390]=-20.0f; /* doubled */
    out->w[391]=-15.0f; /* isolated */
    out->w[392]=-5.0f; /* king safety per attacker? */
    /* Simple PST: fill based on piece-centric? Keep zero; maybe add small central */
}