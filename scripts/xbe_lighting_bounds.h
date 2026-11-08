#pragma once
#include "cpu/x86_context.h"
#include <cstdint>
#include <cstring>
#include <immintrin.h>

// Current xberecomp B2A40 leaf. No scene selection,
// light lifetime, camera state or shader is changed. Unsupported inputs keep
// the generated reference, including its exceptional-value operand ordering.
namespace trial_lighting_bounds {
inline uint32_t Word(const uint8_t* b,uint32_t p){uint32_t v;std::memcpy(&v,b+p,4);return v;}
inline bool Span(uint32_t p,uint32_t n){return p>=0x10000u&&uint64_t(p)+n<=0x100000000ull;}
inline bool Overlap(uint32_t a,uint32_t an,uint32_t b,uint32_t bn){return uint64_t(a)<uint64_t(b)+bn&&uint64_t(b)<uint64_t(a)+an;}
inline __m128 Scalar(uint32_t bits){return _mm_castsi128_ps(_mm_cvtsi32_si128(int32_t(bits)));}
inline bool Try(X86Context& c,uint8_t* b){
    // Trap-time partial register/stack publication belongs to the reference.
    if((_mm_getcsr()&0x1f80u)!=0x1f80u)return false;
    const uint32_t a=c.eax,d=c.edx,t=c.ecx,sp=c.esp;
    if(!b||sp<0x1001cu||!Span(sp-28,28)||!Span(a,12)||!Span(t,12)||!Span(d,24))return false;
    const uint32_t scratch=sp-28;
    if(Overlap(scratch,28,a,12)||Overlap(scratch,28,t,12)||
       Overlap(scratch,28,d,24)||Overlap(scratch,28,0x44d660,4))return false;
    uint32_t av[3],tv[3],dv[6];
    std::memcpy(av,b+a,12);std::memcpy(tv,b+t,12);std::memcpy(dv,b+d,24);
    // Finite inputs <= 2^30 keep all products, translated coordinates and
    // squared sums finite; signed zero and subnormals retain caller MXCSR.
    for(unsigned i=0;i<3;++i)if((av[i]&0x7fffffffu)>0x4e800000u||(tv[i]&0x7fffffffu)>0x4e800000u)return false;
    for(unsigned i=0;i<6;++i)if((dv[i]&0x7fffffffu)>0x4e800000u)return false;
    const uint32_t limit=Word(b,0x44d660);
    if((limit&0x7f800000u)==0x7f800000u)return false;

    __m128 x[8];
    // Preserve authored scalar arithmetic order and guest scratch writes.
    x[2]=_mm_mul_ss(Scalar(dv[1]),Scalar(tv[1]));
    x[0]=_mm_setzero_ps();x[4]=Scalar(av[1]);
    x[3]=_mm_mul_ss(Scalar(dv[2]),Scalar(tv[2]));
    x[1]=_mm_mul_ss(Scalar(dv[0]),Scalar(tv[0]));
    x[7]=_mm_mul_ss(Scalar(dv[5]),Scalar(tv[2]));
    x[6]=_mm_mul_ss(Scalar(dv[4]),Scalar(tv[1]));x[5]=Scalar(av[1]);
    x[4]=_mm_add_ss(x[4],x[2]);x[2]=_mm_add_ss(Scalar(av[2]),x[3]);
    x[3]=_mm_mul_ss(Scalar(tv[0]),Scalar(dv[3]));
    _mm_store_ss(reinterpret_cast<float*>(b+scratch),x[0]);
    x[0]=_mm_add_ss(Scalar(av[0]),x[1]);x[1]=_mm_add_ss(Scalar(av[0]),x[3]);
    x[3]=_mm_add_ss(Scalar(av[2]),x[7]);x[7]=_mm_mul_ss(x[4],x[4]);
    _mm_store_ss(reinterpret_cast<float*>(b+scratch+20),x[7]);
    x[7]=_mm_mul_ss(x[2],x[2]);_mm_store_ss(reinterpret_cast<float*>(b+scratch+24),x[7]);
    x[5]=_mm_add_ss(x[5],x[6]);x[7]=_mm_mul_ss(x[1],x[1]);
    _mm_store_ss(reinterpret_cast<float*>(b+scratch+4),x[7]);
    x[7]=_mm_mul_ss(x[5],x[5]);_mm_store_ss(reinterpret_cast<float*>(b+scratch+8),x[7]);
    x[7]=_mm_mul_ss(x[3],x[3]);x[6]=x[0];
    _mm_store_ss(reinterpret_cast<float*>(b+scratch+12),x[7]);
    x[7]=_mm_setzero_ps();x[6]=_mm_mul_ss(x[6],x[0]);
    const bool positive_x=_mm_comigt_ss(x[0],x[7]);x[0]=Scalar(limit);
    const auto publish=[&](uint32_t result){
        c.eax=result;c.esp=sp+4;
        for(unsigned i=0;i<8;++i)_mm_storeu_si128(reinterpret_cast<__m128i*>(&c.xmm[i]),_mm_castps_si128(x[i]));
        // The generated leaf keeps flags local and never X86_SAVE_FLAGS().
        return true;
    };
    bool compare_x=positive_x;
    if(!positive_x){
        if(_mm_comile_ss(x[7],x[1]))x[6]=Scalar(Word(b,scratch));
        else{x[1]=Scalar(Word(b,scratch+4));_mm_store_ss(reinterpret_cast<float*>(b+scratch),x[1]);x[6]=x[1];compare_x=true;}
    }
    if(compare_x&&_mm_comigt_ss(x[6],x[0]))return publish(a&0xffffff00u);
    if(_mm_comigt_ss(x[2],x[7]))x[1]=Scalar(Word(b,scratch+24));
    else if(_mm_comigt_ss(x[7],x[3]))x[1]=Scalar(Word(b,scratch+12));
    else goto after_z;
    x[1]=_mm_add_ss(x[1],x[6]);_mm_store_ss(reinterpret_cast<float*>(b+scratch),x[1]);x[6]=x[1];
after_z:
    if(_mm_comigt_ss(x[6],x[0]))return publish(a&0xffffff00u);
    if(_mm_comigt_ss(x[4],x[7]))x[1]=Scalar(Word(b,scratch+20));
    else if(_mm_comigt_ss(x[7],x[5]))x[1]=Scalar(Word(b,scratch+8));
    else goto after_y;
    x[1]=_mm_add_ss(x[1],x[6]);_mm_store_ss(reinterpret_cast<float*>(b+scratch),x[1]);x[6]=x[1];
after_y:
    return publish(_mm_comigt_ss(x[0],x[6])?1u:0u);
}
}
