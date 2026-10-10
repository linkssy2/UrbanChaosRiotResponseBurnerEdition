#ifndef NV2A_EFFECT_PREPARATION_H
#define NV2A_EFFECT_PREPARATION_H
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <xmmintrin.h>
/* Pure screen-space rejection. Keep uncertain/near-plane/edge cases. The
 * caller admits only transformed ordered quads with no query/stencil work. */
static int nv_effect_quad_outside(const float xyzw[4][4],unsigned width,unsigned height){
    if(!width||!height||width>16384||height>16384)return 0;
    unsigned common=15;
    for(unsigned i=0;i<4;++i){
        const float*p=xyzw[i];
        for(unsigned j=0;j<4;++j)if(!isfinite(p[j]))return 0;
        if(p[3]<=0 || p[2]<0 || p[2]>1)return 0;
        unsigned side=(p[0]<-2?1:0)|(p[0]>(float)width+2?2:0)|
                      (p[1]<-2?4:0)|(p[1]>(float)height+2?8:0);
        common&=side;
    }
    return common!=0;
}
/* Exact memoization, no pointer identities. One state epoch; bounded direct
 * map with full equality after hashing. Program/constants compared on EVERY
 * lookup, including mid-draw changes. Failed evaluations never cached. */
typedef struct {
    uint64_t epoch;
    float input[16][4],output[16][4];
    unsigned written[16],exceptions;
} NvEffectMemoEntry;
typedef struct {
    uint64_t epoch;
    unsigned start,control;
    uint32_t program[136][4];
    float constants[192][4];
    NvEffectMemoEntry entries[1024];
} NvEffectMemo;
static NvEffectMemoEntry*nv_effect_memo_slot(NvEffectMemo*s,const uint32_t p[136][4],
 unsigned start,const float c[192][4],const float in[16][4]){
    unsigned control=_mm_getcsr();
    /* Unmasked floating exceptions require executing the original operation. */
    if((control&0x1F80)!=0x1F80)return NULL;
    if(!s->epoch||start!=s->start||control!=s->control||
       memcmp(s->program,p,sizeof(s->program))||memcmp(s->constants,c,sizeof(s->constants))){
        if(++s->epoch==0){memset(s->entries,0,sizeof(s->entries));s->epoch=1;}
        s->start=start;s->control=control;
        memcpy(s->program,p,sizeof(s->program));memcpy(s->constants,c,sizeof(s->constants));
    }
    uint32_t h=2166136261u,word;
    for(unsigned i=0;i<64;++i){memcpy(&word,(const char*)in+i*4,4);h=(h^word)*16777619u;}
    return &s->entries[(h^(h>>16))&1023u];
}
static int nv_effect_memo_get(NvEffectMemo*s,NvEffectMemoEntry*e,const float in[16][4],float out[16][4],unsigned written[16]){
    if(!e||e->epoch!=s->epoch||memcmp(e->input,in,sizeof(e->input)))return 0;
    memcpy(out,e->output,sizeof(e->output));memcpy(written,e->written,sizeof(e->written));
    _mm_setcsr(_mm_getcsr()|e->exceptions);return 1;
}
static void nv_effect_memo_put(NvEffectMemo*s,NvEffectMemoEntry*e,const float in[16][4],const float out[16][4],const unsigned written[16],unsigned before){
    if(!e)return;
    e->epoch=s->epoch;memcpy(e->input,in,sizeof(e->input));memcpy(e->output,out,sizeof(e->output));
    memcpy(e->written,written,sizeof(e->written));e->exceptions=_mm_getcsr()&63u;(void)before;
}
#endif
