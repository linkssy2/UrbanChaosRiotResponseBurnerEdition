#pragma once
#include "cpu/x86_context.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <xmmintrin.h>

namespace trial_ragdoll_force {
inline float Real(const uint8_t* b,uint32_t p){float v;std::memcpy(&v,b+p,4);return v;}
inline uint32_t Word(const uint8_t* b,uint32_t p){uint32_t v;std::memcpy(&v,b+p,4);return v;}
// Integrate the existing 60-Hz force envelope over this frame, including hitches.
inline double Average(double duration,double remaining,double dt){
    const double start=std::max(0.0,duration-remaining),end=std::min(duration,start+dt);
    double area=0;
    for(double t=start;t<end;){
        const double boundary=(std::floor(t*60.0)+1.0)/60.0;
        const double next=std::min(end,std::max(boundary,t+1.e-12));
        area+=(next-t)*std::max(0.0,1.0-boundary/duration);t=next;
    }
    return area/dt;
}
struct Scope {
    uint8_t* b;uint32_t force;float before,dt,duration;
    Scope(const X86Context& c,uint8_t* base):b(base),force(c.ecx),
        before(Real(b,force+4)),dt(Real(b,c.esp+4)),duration(Real(b,force+0x40)){}
    ~Scope(){
        const unsigned csr=_mm_getcsr();
        if(Word(b,force)==0x48d9ec&&std::isfinite(before)&&before>=0&&
           std::isfinite(duration)&&duration>0&&before<=duration&&
           (before==0||(std::isfinite(dt)&&dt>0&&dt<=.25f))){
            const float scale=before==0?0.f:float(Average(duration,before,dt));
            for(unsigned i=0;i<4;++i){const float v=Real(b,force+0x50+i*4)*scale;std::memcpy(b+force+0x30+i*4,&v,4);}
        }
        _mm_setcsr(csr);
    }
};
inline bool Tail(const uint8_t* b,uint32_t p){
    if(Word(b,p)!=0x48d9ec||Word(b,p+4)!=0)return false;
    // Keep the final partial impulse for this update; the next update deletes it.
    for(unsigned i=0;i<3;++i)if(Word(b,p+0x30+i*4)&0x7fffffff)return true;
    return false;
}
}
