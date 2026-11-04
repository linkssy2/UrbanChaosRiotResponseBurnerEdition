#undef NDEBUG
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include "cpu/x86_ops.h"
#include "xbe_blink.h"
alignas(16) static std::array<uint8_t,0x800000> memory{};
constexpr uint32_t OBJ=0x600000,WORLD=0x610000,MATRICES=0x620000,SP=0x780000;
static unsigned random_calls,pose_calls;
uint8_t* B(){return memory.data();}
uint32_t Word(uint32_t p){return trial_blink::Word(B(),p);}
void Put(uint32_t p,uint32_t v){std::memcpy(B()+p,&v,4);}
void Float(uint32_t p,float v){std::memcpy(B()+p,&v,4);}
namespace mem {bool IsCommitted(uint32_t p){return p<memory.size();}}
X86_FUNC(sub_002D37B0){assert(Word(ctx.esp+4)==30&&Word(ctx.esp+8)==150);++random_calls;ctx.eax=30;ctx.esp+=4;}
// Pose math is a boundary; native lid indices, angles, timing and calls execute unchanged.
X86_FUNC(sub_002D3DC0){++pose_calls;Put(ctx.ecx,Word(ctx.esp+12));ctx.eax=ctx.ecx;ctx.esp+=20;}
X86_FUNC(sub_002B7230){const auto out=Word(ctx.esp+4),in=Word(ctx.esp+12);std::memcpy(B()+out,B()+in,64);ctx.eax=out;ctx.esp+=4;}
X86_FUNC(sub_002B5A10){ctx.esp+=4;}
void x86_icall(X86Context&,uint8_t*,uint32_t){std::abort();}
[[noreturn]]void x86_trap(X86Context&,uint32_t,const char*){std::abort();}
void x86_resume(X86Context&,uint8_t*,const X86Resume&){std::abort();}
// NATIVE_BODIES
static void Seed(){
    memory.fill(0);trial_blink::clocks.clear();trial_blink::world=0;random_calls=pose_calls=0;
    Put(0x51c4dc,WORLD);
    // NATIVE_CONSTANTS
    for(unsigned i=0;i<2;++i){Put(OBJ+i*64,0x48f1c4);Put(OBJ+i*64+0x10,0);Put(OBJ+i*64+0x14,1);}
}
static void Call(bool fixed,float delta,unsigned slot=0){
    X86Context c{};c.ecx=OBJ+slot*64;c.esp=SP;c.ebp=0x1111;c.ebx=0x2222;c.esi=0x3333;c.edi=0x4444;
    Float(SP+4,delta);Put(SP+8,MATRICES);const unsigned csr=_mm_getcsr();
    (fixed?FixedBlink:OriginalBlink)(c,B());
    assert(c.esp==SP+20&&c.ebp==0x1111&&c.ebx==0x2222&&c.esi==0x3333&&c.edi==0x4444&&(c.eax&255)==1);
    assert((_mm_getcsr()&~63u)==(csr&~63u));
}
int main(){
    for(int fps:{30,60,120,144,240,360}){
        Seed();Call(true,1.f/fps);Call(true,1.f/fps,1);
        for(unsigned frame=1;frame<=unsigned(fps*10);++frame){
            Float(WORLD+8,float(frame)/fps);
            for(unsigned slot=0;slot<2;++slot){
                Call(true,1.f/fps,slot);const auto count=Word(OBJ+slot*64+0x18),rng=random_calls,pose=pose_calls;
                assert(count==(unsigned(double(frame)*30/fps+1.e-5)%30));
                const auto top=Word(MATRICES),bottom=Word(MATRICES+64);
                Call(true,1.f/fps,slot); // The other viewport still needs the same fresh pose.
                assert(Word(OBJ+slot*64+0x18)==count&&random_calls==rng);
                assert(pose_calls-pose==(count<5?2:0));
                assert(Word(MATRICES)==top&&Word(MATRICES+64)==bottom);
            }
        }
        assert(random_calls==22);
        std::printf("blink %d FPS: 30-Hz cycle, two independent actors and duplicate views passed\n",fps);
    }
    Seed();for(unsigned i=0;i<301;++i)Call(false,1.f/30);const auto reference=random_calls;
    Seed();for(unsigned i=0;i<2401;++i)Call(false,1.f/240);assert(random_calls>reference*7); // Negative witness.
    Seed();Call(true,1.f/30);Float(WORLD+8,1);Call(true,.2f);assert(Word(OBJ+0x18)==6);
    Float(WORLD+8,2);Call(true,0);assert(Word(OBJ+0x18)==6);
    B()[OBJ+0x20]=1;Float(WORLD+8,3);const auto pose=pose_calls;Call(true,.1f);
    assert(Word(OBJ+0x18)==6&&pose_calls==pose+2); // Forced closed still applies both lids.
    B()[OBJ+0x20]=0;Float(WORLD+8,4);Call(true,1.f/30);assert(Word(OBJ+0x18)==7);
    X86Context c{};c.ecx=OBJ;c.esp=SP;Put(SP+4,0);DestroyBlink(c,B());assert(!trial_blink::clocks.count(OBJ));
    Put(OBJ+0x18,0);Put(OBJ+0x1c,0);Call(true,1.f/30);assert(Word(OBJ+0x18)==0); // Pool reuse.
    Float(WORLD+8,5);Call(true,std::numeric_limits<float>::quiet_NaN());assert(Word(OBJ+0x18)==1); // Unsupported clock retains retail.
    std::puts("blink: native timing/RNG branch and eyelid application passed; matrix math and scene scheduling are boundaries");
}
