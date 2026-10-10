#ifndef NV2A_VERTEX_EVAL_H
#define NV2A_VERTEX_EVAL_H
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#if defined(_M_X64) || defined(__SSE2__)
#include <xmmintrin.h>
#endif
static int nv2a_alu_bundle_option=-1;
static int nv2a_alu_bundle_enabled(void) {
    if(nv2a_alu_bundle_option<0){const char*s=getenv("UCRR_CPU_ALU_BUNDLE");nv2a_alu_bundle_option=s&&!strcmp(s,"1");}
    return nv2a_alu_bundle_option;
}

/* Preserve expression order, old-register reads, masks and rejection in the
 * evaluator. Only redundant lane dispatch/scalar evaluation is removed. */
static void nv2a_vp_alu_bundle(unsigned mac,unsigned ilu,const float a[4],
    const float b[4],const float c[4],float m[4],float s[4]) {
    float value;
    switch(mac) {
    case 1:memcpy(m,a,16);break;
    case 2:for(unsigned k=0;k<4;++k)m[k]=a[k]*b[k];break;
    case 3:for(unsigned k=0;k<4;++k)m[k]=a[k]+c[k];break;
    case 4:for(unsigned k=0;k<4;++k)m[k]=a[k]*b[k]+c[k];break;
    case 5:value=a[0]*b[0]+a[1]*b[1]+a[2]*b[2];m[0]=m[1]=m[2]=m[3]=value;break;
    case 6:value=a[0]*b[0]+a[1]*b[1]+a[2]*b[2]+b[3];m[0]=m[1]=m[2]=m[3]=value;break;
    case 7:value=a[0]*b[0]+a[1]*b[1]+a[2]*b[2]+a[3]*b[3];m[0]=m[1]=m[2]=m[3]=value;break;
    case 8:m[0]=1.0f;m[1]=a[1]*b[1];m[2]=a[2];m[3]=b[3];break;
    case 9:for(unsigned k=0;k<4;++k)m[k]=fminf(a[k],b[k]);break;
    case 10:for(unsigned k=0;k<4;++k)m[k]=fmaxf(a[k],b[k]);break;
    case 11:for(unsigned k=0;k<4;++k)m[k]=a[k]<b[k]?1.0f:0.0f;break;
    case 12:for(unsigned k=0;k<4;++k)m[k]=a[k]>=b[k]?1.0f:0.0f;break;
    }
    switch(ilu) {
    case 1:memcpy(s,c,16);break;
    case 2:value=1.0f/c[0];s[0]=s[1]=s[2]=s[3]=value;break;
    case 3:value=1.0f/c[0];{
        float magnitude=fminf(fmaxf(fabsf(value),0x1p-64f),0x1p64f);
        value=copysignf(magnitude,value);
    }s[0]=s[1]=s[2]=s[3]=value;break;
    case 4:value=1.0f/sqrtf(fabsf(c[0]));s[0]=s[1]=s[2]=s[3]=value;break;
    }
}

/* NV2A viewport methods alias the transform constant file (c[-38/-37]
 * in XDK notation, raw slots 58/59). Keep command order with explicit uploads. */
static int nv2a_vertex_viewport_constant(float constants[192][4],uint32_t method,uint32_t bits)
{
    unsigned row,component;
    if(method>=0xA20 && method<=0xA2C && !(method&3)){row=59;component=(method-0xA20)/4;}
    else if(method>=0xAF0 && method<=0xAFC && !(method&3)){row=58;component=(method-0xAF0)/4;}
    else return 0;
    memcpy(&constants[row][component],&bits,4);return 1;
}

/* Bounded CPU evaluator for the immediate 2D path. Hardware instruction
 * fields, not D3D shader tokens. Unsupported instructions fail explicitly.
 * This does not implement indexed geometry, clipping, or all ILU precision. */
static int nv2a_vp_source(float dst[4], unsigned mux, unsigned reg,
    unsigned swizzle, unsigned negate, unsigned vertex, int constant,
    const float input[16][4], const float constants[192][4], float temp[16][4])
{
    const float *s;
    if (mux == 1) s = temp[reg];
    else if (mux == 2) s = input[vertex];
    else if (mux == 3 && constant >= 0 && constant < 192) s = constants[constant];
    else return 0;
    for (unsigned k=0; k<4; ++k)
        dst[k] = s[(swizzle >> (6-2*k)) & 3] * (negate ? -1.0f : 1.0f);
    return 1;
}

static void nv2a_vp_write(float dst[4], const float src[4], unsigned mask)
{
    for (unsigned k=0; k<4; ++k) if (mask & (8u >> k)) dst[k]=src[k];
}

static int nv2a_vertex_eval(const uint32_t program[136][4], unsigned start,
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
        const uint32_t *w=program[pc];
        unsigned mac=(w[1]>>21)&15, ilu=(w[1]>>25)&7;
        unsigned vertex=(w[1]>>9)&15, r=(w[3]>>20)&15;
        int ci=(int)((w[1]>>13)&255) + ((w[3]&2) ? address : 0);
        float a[4]={0}, b[4]={0}, c[4]={0}, m[4]={0}, s[4]={0};
        if (mac>13 || ilu>4) return 0;
        if (mac && !nv2a_vp_source(a,(w[2]>>26)&3,w[2]>>28,w[1]&255,(w[1]>>8)&1,vertex,ci,input,constants,temp)) return 0;
        if (mac>=2 && mac!=3 && mac!=13 && !nv2a_vp_source(b,(w[2]>>11)&3,(w[2]>>13)&15,(w[2]>>17)&255,(w[2]>>25)&1,vertex,ci,input,constants,temp)) return 0;
        if ((ilu || mac==3 || mac==4) && !nv2a_vp_source(c,(w[3]>>28)&3,((w[2]&3)<<2)|(w[3]>>30),(w[2]>>2)&255,(w[2]>>10)&1,vertex,ci,input,constants,temp)) return 0;
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
        if(mac && mac!=13 && !(ilu && r==1)) {
            unsigned mask=(w[3]>>24)&15;
            VP_WRITE(temp[r],m,mask);
            if(r==12){VP_WRITE(output[0],m,mask);written[0]|=mask;}
        }
        if(ilu) {
            unsigned dest=mac?1:r, mask=(w[3]>>16)&15;
            VP_WRITE(temp[dest],s,mask);
            if(dest==12){VP_WRITE(output[0],s,mask);written[0]|=mask;}
        }
        unsigned mask=(w[3]>>12)&15, out=(w[3]>>3)&15;
        if(mask) {
            if(!(w[3]&0x800)) return 0; /* Constant writes require another contract. */
            VP_WRITE(output[out],(w[3]&4)?s:m,mask);
            written[out]|=mask;
            if(out==0) VP_WRITE(temp[12],output[0],mask);
        }
        if(w[3]&1) return 1;
    }
    return 0;
#undef VP_WRITE
}

static uint32_t nv2a_pack_argb(const float rgba[4])
{
    uint32_t channel[4];
    for(unsigned k=0;k<4;++k) {
        float f=rgba[k];
        if(!(f>=0))f=0;
        if(f>1)f=1;
        channel[k]=(uint32_t)(f*255.0f+0.5f);
    }
    return (channel[3]<<24)|(channel[0]<<16)|(channel[1]<<8)|channel[2];
}
#endif
