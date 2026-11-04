#undef NDEBUG
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "cpu/x86_ops.h"
#include "xbe_ragdoll_force.h"
alignas(16) static std::array<uint8_t,0x800000> memory{};
constexpr uint32_t F=0x600000,R=0x601000,LIST=0x602000,BODY=0x603000,MOTION=0x604000,VT=0x605000,TIME=0x606000,TRANSFORM=0x607000,SP=0x780000;
static bool corrected;static unsigned deleted;
uint8_t* B(){return memory.data();}
uint32_t Word(uint32_t p){return trial_ragdoll_force::Word(B(),p);}
float Real(uint32_t p){return trial_ragdoll_force::Real(B(),p);}
void Put(uint32_t p,uint32_t v){std::memcpy(B()+p,&v,4);}
void Float(uint32_t p,float v){std::memcpy(B()+p,&v,4);}
// NATIVE_DECLARATIONS
X86_FUNC(OriginalForce);X86_FUNC(FixedForce);
X86_FUNC(sub_002F4FF0);
// Collision/pose stepping and force spatial falloff are explicit boundaries.
// The force update, application loop and final inverse-mass integration execute retail code.
X86_FUNC(sub_002A51C0){ctx.esp+=8;}
X86_FUNC(sub_002A55A0){ctx.esp+=4;}
X86_FUNC(sub_0029E500){ctx.eax=TIME;ctx.esp+=4;}
X86_FUNC(sub_002ECAA0){assert(ctx.ecx==BODY);ctx.esp+=4;}
void x86_icall(X86Context& c,uint8_t* base,uint32_t fn){
    if(fn==0x2a33a0){(corrected?FixedForce:OriginalForce)(c,base);return;}
    if(fn==0x2f4ff0){sub_002F4FF0(c,base);return;}
    if(fn==0xf001){assert(c.ecx==F);const uint32_t out=Word(c.esp+4);std::memcpy(base+out,base+F+0x30,16);c.eax=out;c.esp+=12;return;}
    if(fn==0xf002){assert(c.ecx==F&&Word(c.esp+4)==1);++deleted;c.esp+=8;return;}
    std::abort();
}
[[noreturn]]void x86_trap(X86Context&,uint32_t,const char*){std::abort();}
void x86_resume(X86Context&,uint8_t*,const X86Resume&){std::abort();}
// NATIVE_BODIES
static float Run(float duration,int fps,bool fix){
    memory.fill(0);corrected=fix;deleted=0;
    Float(0x44bf80,-1);Put(0x48d9ec,0xf002);Put(0x48d9f0,0xf001);Put(0x48d9f4,0x2a33a0);
    Put(F,0x48d9ec);Float(F+4,duration);Float(F+0x40,duration);Float(F+0x50,1200);Float(F+0x54,900);
    Put(R+0x1c,1);B()[R+0x24]=1;Put(R+0x8c,LIST);Put(R+0x90,1);Put(LIST,F);
    Put(R+0x70,LIST+16);Put(R+0x74,1);Put(LIST+16,BODY);Put(BODY+0x50,MOTION);
    Put(MOTION,VT);Put(VT+0x6c,0x2f4ff0);Float(MOTION+0xc0,.5f);
    for(unsigned tick=0;tick<unsigned(fps*duration+5)&&!deleted;++tick){
        const float dt=1.f/fps;Float(TIME+8,dt);
        X86Context c{};c.ecx=R;c.esp=SP;c.ebp=0x1111;c.esi=0x2222;c.edi=0x3333;c.ebx=0x4444;
        Put(SP,0xbeef);Float(SP+4,dt);Put(SP+8,TRANSFORM);
        (fix?FixedRagdoll:OriginalRagdoll)(c,B());
        assert(c.esp==SP+12&&c.ebp==0x1111&&c.esi==0x2222&&c.edi==0x3333&&c.ebx==0x4444);
    }
    assert(deleted==1&&Word(R+0x90)==0);return Real(MOTION+0xd0);
}
int main(){
    for(float duration:{.4f,.6f,.117f,1.f}){
        const float reference=Run(duration,60,false);
        for(int fps:{30,60,120,144,240,360}){
            const float result=Run(duration,fps,true);
            assert(std::fabs(result-reference)<.01f);
            std::printf("duration %.3f, %d FPS: impulse %.6f (native 60 %.6f)\n",duration,fps,result,reference);
        }
        assert(Run(duration,240,false)>reference);
    }
    // Frame partitions crossing the last nonzero slice retain that slice once.
    assert(trial_ragdoll_force::Average(.117,.017,.03)>0);
    assert(trial_ragdoll_force::Average(.6,0,.016)==0);
    X86Context c{};c.ecx=F;c.esp=SP;Put(F,0x48d9ec);Float(F+0x40,.6f);Float(F+0x50,100);
    for(float lifetime:{-1.f,.6f}){
        Float(F+4,lifetime);Float(F+0x30,42);Float(SP+4,0);
        {trial_ragdoll_force::Scope scope(c,B());}
        assert(Real(F+0x30)==42); // Permanent force and a paused step retain retail behavior.
    }
    Float(F+4,0);Float(F+0x30,42);
    {trial_ragdoll_force::Scope scope(c,B());}
    assert(!trial_ragdoll_force::Tail(B(),F)); // Even a zero-dt update releases the final slice.
    std::puts("ragdoll force: native lifetime/removal/application/motion at 30–360 FPS passed; spatial falloff and collision stepping are boundaries.");
}
