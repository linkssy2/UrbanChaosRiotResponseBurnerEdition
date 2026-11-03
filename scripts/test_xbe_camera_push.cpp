// Execute the pinned, active original and corrected integrators and Euler matrix.
#undef NDEBUG
#include "cpu/x86_ops.h"
#include "xbe_camera_push.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>

alignas(16) static std::array<uint8_t,0x600000> memory{};
constexpr uint32_t P=0x560000,Q=0x560100,S=0x5f0000;
void Put(uint32_t at,uint32_t value){std::memcpy(memory.data()+at,&value,4);}
void Real(uint32_t at,float value){std::memcpy(memory.data()+at,&value,4);}
float Real(uint32_t at){float value;std::memcpy(&value,memory.data()+at,4);return value;}
[[noreturn]] void x86_trap(X86Context&,uint32_t,const char*){std::abort();}
// ORIGINAL_AND_CORRECTED_INTEGRATORS

using Body=void(*)(X86Context&,uint8_t*);
using Snapshot=std::array<uint8_t,0x100>;
Snapshot Save(uint32_t owner){Snapshot out;std::memcpy(out.data(),memory.data()+owner,out.size());return out;}
void Load(uint32_t owner,const Snapshot& state){std::memcpy(memory.data()+owner,state.data(),state.size());}
void Seed(uint32_t owner,float angular=.1f,float linear=.3f){
    std::memset(memory.data()+owner,0,0x100);Put(owner,0x4500ac);Put(owner+0x80,0x12345678);
    for(unsigned axis=0;axis<3;++axis){Real(owner+0x10+axis*4,angular*(axis==1?-1.f:1.f));Real(owner+0x28+axis*4,linear);}
}
void Tick(Body body,uint32_t owner,float delta){
    auto before=Save(owner);X86Context ctx{};ctx.ecx=owner;ctx.esp=S;ctx.ebp=41;ctx.ebx=43;ctx.esi=47;ctx.edi=53;
    Put(S,0x1234);Real(S+4,delta);body(ctx,memory.data());
    assert(ctx.esp==S+8&&ctx.ebp==41&&ctx.ebx==43&&ctx.esi==47&&ctx.edi==53&&ctx.top==0);
    auto after=Save(owner);
    for(unsigned i=0;i<0x100;++i)if(!(i>=4&&i<0x34)&&!(i>=0x40&&i<0x80))assert(after[i]==before[i]);
    for(unsigned i=0;i<12;++i)assert(std::isfinite(Real(owner+4+i*4)));
    for(unsigned i=0;i<16;++i)assert(std::isfinite(Real(owner+0x40+i*4)));
    assert(Real(S+4)==delta);
}
float Difference(uint32_t a,uint32_t b){
    float maximum=0;
    for(unsigned i=0;i<12;++i)maximum=std::max(maximum,std::abs(Real(a+4+i*4)-Real(b+4+i*4)));
    for(unsigned i=0;i<16;++i)maximum=std::max(maximum,std::abs(Real(a+0x40+i*4)-Real(b+0x40+i*4)));
    return maximum;
}
int main(){
    // ORIGINAL_CONSTANTS
    unsigned random=0x12345678,comparisons=0;
    for(unsigned trial=0;trial<1000;++trial){
        Seed(P);for(unsigned i=0;i<12;++i){random=random*1664525+1013904223;Real(P+4+i*4,(float(random>>8)/16777216.f-.5f)*(i<6?8.f:80.f));}
        Load(Q,Save(P));Tick(Original,P,1.f/30);Tick(Corrected,Q,1.f/30);
        assert(Save(P)==Save(Q));++comparisons;
    }
    float worst=0;
    for(int fps:{30,60,120,144,240,360,1000}){
        float peak=0,old_peak=0;
        Seed(P);Seed(Q);
        for(int i=0;i<fps*2;++i){
            Tick(Original,P,1.f/fps);Tick(Corrected,Q,1.f/fps);
            old_peak=std::max(old_peak,std::abs(Real(P+4)));peak=std::max(peak,std::abs(Real(Q+4)));
        }
        assert(peak>=.139f&&peak<.144f);
        std::printf("fps=%d original_peak=%.9f corrected_peak=%.9f\n",fps,old_peak,peak);
        // End each group at the same original 30 Hz boundary, including a short
        // remainder when the display cadence is not a multiple of 30.
        for(float impulse:{.01f,.1f,.3f}){
            Seed(P,impulse);Seed(Q,impulse);
            for(unsigned frame=0;frame<180;++frame){
                if(frame&&frame%3==0){Real(P+0x10,Real(P+0x10)+impulse);Real(Q+0x10,Real(Q+0x10)+impulse);}
                Tick(Original,P,1.f/30);
                double remaining=double(1.f/30);
                while(remaining>1e-9){float dt=float(std::min(remaining,1.0/fps));Tick(Corrected,Q,dt);remaining-=dt;}
                const float error=Difference(P,Q);worst=std::max(worst,error);assert(error<.00002f);++comparisons;
            }
        }
    }
    // Different per-player impulses, variable frame times, zero delta, hitch,
    // and large limited pushes; no shared cached state may couple the cameras.
    Seed(P);Seed(Q,-.23f,1.7f);
    for(float dt:{0.f,.001f,.006f,.01f,.09f,.5f,1.f}){
        auto other=Save(Q);Tick(Corrected,P,dt);assert(Save(Q)==other);
        auto first=Save(P);Tick(Corrected,Q,dt);assert(Save(P)==first);
    }
    Seed(P);auto before=Save(P);Tick(Corrected,P,0);auto after=Save(P);
    assert(std::memcmp(before.data()+4,after.data()+4,48)==0);
    for(int fps:{30,60,144,240}){
        Seed(P,5,50);
        for(int i=0;i<fps*4;++i)Tick(Corrected,P,1.f/fps);
        for(unsigned i=0;i<12;++i)assert(std::abs(Real(P+4+i*4))<.001f);
    }
    std::printf("PASS: %u reference comparisons, max state/matrix error %.9g; ABI, bounds, two owners, hitches and settling\n",comparisons,worst);
}
