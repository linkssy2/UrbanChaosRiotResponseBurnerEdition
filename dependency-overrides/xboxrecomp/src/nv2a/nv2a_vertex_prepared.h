#ifndef NV2A_VERTEX_PREPARED_H
#define NV2A_VERTEX_PREPARED_H
#include "nv2a_vertex_eval.h"
/* Owned decoded metadata only. Inputs/constants/results are always live.
 * Update this checked transcription when the original evaluator changes. */
typedef struct { uint8_t mac,ilu,vertex,r,constant,relative,amux,areg,aswizzle,aneg,bmux,breg,bswizzle,bneg,cmux,creg,cswizzle,cneg,mmask,imask,omask,out,output_write,output_ilu,end,need_b,need_c,commit_mac,ilu_dest; } NvVpInstruction;
typedef struct { NvVpInstruction code[136]; unsigned span; } NvVpPrepared;
static void nv_vp_prepare(NvVpPrepared*p,const uint32_t raw[136][4],unsigned start){
 p->span=0;
 for(unsigned pc=start;pc<136;++pc){const uint32_t*w=raw[pc];NvVpInstruction*d=&p->code[pc];
 d->mac=(uint8_t)((w[1]>>21)&15);
 d->ilu=(uint8_t)((w[1]>>25)&7);
 d->vertex=(uint8_t)((w[1]>>9)&15);
 d->r=(uint8_t)((w[3]>>20)&15);
 d->constant=(uint8_t)((w[1]>>13)&255);
 d->relative=(uint8_t)(w[3]&2);
 d->amux=(uint8_t)((w[2]>>26)&3);
 d->areg=(uint8_t)(w[2]>>28);
 d->aswizzle=(uint8_t)(w[1]&255);
 d->aneg=(uint8_t)((w[1]>>8)&1);
 d->bmux=(uint8_t)((w[2]>>11)&3);
 d->breg=(uint8_t)((w[2]>>13)&15);
 d->bswizzle=(uint8_t)((w[2]>>17)&255);
 d->bneg=(uint8_t)((w[2]>>25)&1);
 d->cmux=(uint8_t)((w[3]>>28)&3);
 d->creg=(uint8_t)(((w[2]&3)<<2)|(w[3]>>30));
 d->cswizzle=(uint8_t)((w[2]>>2)&255);
 d->cneg=(uint8_t)((w[2]>>10)&1);
 d->mmask=(uint8_t)((w[3]>>24)&15);
 d->imask=(uint8_t)((w[3]>>16)&15);
 d->omask=(uint8_t)((w[3]>>12)&15);
 d->out=(uint8_t)((w[3]>>3)&15);
 d->output_write=(uint8_t)(!!(w[3]&0x800));
 d->output_ilu=(uint8_t)(w[3]&4);
 d->end=(uint8_t)(w[3]&1);
 d->need_b=d->mac>=2&&d->mac!=3&&d->mac!=13;
 d->need_c=d->ilu||d->mac==3||d->mac==4;
 d->commit_mac=d->mac&&d->mac!=13&&!(d->ilu&&d->r==1);
 d->ilu_dest=d->mac?1:d->r;
 ++p->span;if(d->end)break;
 }
}
#if defined(_MSC_VER)
static __forceinline int nv_vp_prepared_eval(const NvVpPrepared *plan,const uint32_t program[136][4], unsigned start,
#else
static inline int nv_vp_prepared_eval(const NvVpPrepared *plan,const uint32_t program[136][4], unsigned start,
#endif
    const float constants[192][4], const float input[16][4],
    float output[16][4], unsigned written[16])
{
    float temp[16][4]={{0}};
    const int bundled=nv2a_alu_bundle_enabled();
#define VP_WRITE(D,S,M) do { unsigned vp_mask=(M); if(bundled&&vp_mask==15)memcpy((D),(S),16);else nv2a_vp_write((D),(S),vp_mask); } while(0)
    int address=0;
    memset(output,0,16*4*sizeof(float)); memset(written,0,16*sizeof(unsigned));
    for (unsigned k=0; k<16; ++k) output[k][3]=1.0f;
    for (unsigned pc=start; pc<136; ++pc) {
        const NvVpInstruction *d=&plan->code[pc];
        unsigned mac=d->mac, ilu=d->ilu;
        unsigned vertex=d->vertex, r=d->r;
        int ci=(int)(d->constant) + ((d->relative) ? address : 0);
        float a[4]={0}, b[4]={0}, c[4]={0}, m[4]={0}, s[4]={0};
        if (mac>13 || ilu>4) return 0;
        if (mac && !nv2a_vp_source(a,d->amux,d->areg,d->aswizzle,d->aneg,vertex,ci,input,constants,temp)) return 0;
        if (d->need_b && !nv2a_vp_source(b,d->bmux,d->breg,d->bswizzle,d->bneg,vertex,ci,input,constants,temp)) return 0;
        if (d->need_c && !nv2a_vp_source(c,d->cmux,d->creg,d->cswizzle,d->cneg,vertex,ci,input,constants,temp)) return 0;
        int original_alu=!bundled;
        if(bundled){
            nv2a_vp_alu_bundle(mac,ilu,a,b,c,m,s);
            /* Host code layout can select a different NaN operand/sign.
             * Re-run just this uncommitted instruction using original math. */
#if defined(_M_X64) || defined(__SSE2__)
            __m128 vm=_mm_loadu_ps(m),vs=_mm_loadu_ps(s);
            original_alu=_mm_movemask_ps(_mm_or_ps(_mm_cmpunord_ps(vm,vm),_mm_cmpunord_ps(vs,vs)))!=0;
#else
            for(unsigned k=0;k<4;++k)if(isnan(m[k])||isnan(s[k]))original_alu=1;
#endif
        }
        if(original_alu&&bundled)return nv2a_vertex_eval(program,start,constants,input,output,written);
        if(original_alu)for(unsigned k=0;k<4;++k) {
            switch(mac) {
            case 1:m[k]=a[k];break;
            case 2:m[k]=a[k]*b[k];break;
            case 3:m[k]=a[k]+c[k];break;
            case 4:m[k]=a[k]*b[k]+c[k];break;
            case 5:m[k]=a[0]*b[0]+a[1]*b[1]+a[2]*b[2];break;
            case 6:m[k]=a[0]*b[0]+a[1]*b[1]+a[2]*b[2]+b[3];break;
            case 7:m[k]=a[0]*b[0]+a[1]*b[1]+a[2]*b[2]+a[3]*b[3];break;
            case 8:m[k]=k==0?1.0f:k==1?a[1]*b[1]:k==2?a[2]:b[3];break;
            case 9:m[k]=fminf(a[k],b[k]);break;
            case 10:m[k]=fmaxf(a[k],b[k]);break;
            case 11:m[k]=a[k]<b[k]?1.0f:0.0f;break;
            case 12:m[k]=a[k]>=b[k]?1.0f:0.0f;break;
            }
            if(ilu==1)s[k]=c[k];
            if(ilu==2 || ilu==3)s[k]=1.0f/c[0];
            if(ilu==3) {
                float magnitude=fminf(fmaxf(fabsf(s[k]),0x1p-64f),0x1p64f);
                s[k]=copysignf(magnitude,s[k]);
            }
            if(ilu==4)s[k]=1.0f/sqrtf(fabsf(c[0]));
        }
        if(mac==13) {
            if(!isfinite(a[0]) || a[0]<-192 || a[0]>192) return 0;
            address=(int)floorf(a[0]);
        }
        /* Both units read old registers; commit only after evaluating both. */
        if(d->commit_mac) {
            unsigned mask=d->mmask;
            VP_WRITE(temp[r],m,mask);
            if(r==12){VP_WRITE(output[0],m,mask);written[0]|=mask;}
        }
        if(ilu) {
            unsigned dest=d->ilu_dest, mask=d->imask;
            VP_WRITE(temp[dest],s,mask);
            if(dest==12){VP_WRITE(output[0],s,mask);written[0]|=mask;}
        }
        unsigned mask=d->omask, out=d->out;
        if(mask) {
            if(!(d->output_write)) return 0; /* Constant writes require another contract. */
            VP_WRITE(output[out],(d->output_ilu)?s:m,mask);
            written[out]|=mask;
            if(out==0) VP_WRITE(temp[12],output[0],mask);
        }
        if(d->end) return 1;
    }
    return 0;
#undef VP_WRITE
}


/* Single renderer owner. All effective program/start writes and device resets
 * invalidate. Short/low-reuse routes retain the current evaluator. */
static int nv_vp_decode_option=-1;
static struct {NvVpPrepared plan;const void*owner;unsigned start,calls,mode;} nv_vp_prepared_cache;
static int nv_vp_decode_enabled(void){
 if(nv_vp_decode_option<0){const char*s=getenv("UCRR_CPU_VP_DECODE");nv_vp_decode_option=s&&!strcmp(s,"1");}
 return nv_vp_decode_option;
}
static void nv_vp_decode_invalidate(void){nv_vp_prepared_cache.owner=NULL;nv_vp_prepared_cache.calls=nv_vp_prepared_cache.mode=0;}
static void nv_vp_decode_change(uint32_t before,uint32_t after){if(before!=after)nv_vp_decode_invalidate();}
static int nv_vp_decode_eval(unsigned route,const uint32_t program[136][4],unsigned start,
 const float constants[192][4],const float input[16][4],float output[16][4],unsigned written[16]){
 if(route!=3||!nv_vp_decode_enabled()||!nv2a_alu_bundle_enabled())return nv2a_vertex_eval(program,start,constants,input,output,written);
 if(nv_vp_prepared_cache.owner!=program||nv_vp_prepared_cache.start!=start){
  nv_vp_decode_invalidate();nv_vp_prepared_cache.owner=program;nv_vp_prepared_cache.start=start;
 }
 if(!nv_vp_prepared_cache.mode&&++nv_vp_prepared_cache.calls==8){
  nv_vp_prepare(&nv_vp_prepared_cache.plan,program,start);
  nv_vp_prepared_cache.mode=nv_vp_prepared_cache.plan.span==17?1:2;
 }
 if(nv_vp_prepared_cache.mode==1)return nv_vp_prepared_eval(&nv_vp_prepared_cache.plan,program,start,constants,input,output,written);
 return nv2a_vertex_eval(program,start,constants,input,output,written);
}
#endif
