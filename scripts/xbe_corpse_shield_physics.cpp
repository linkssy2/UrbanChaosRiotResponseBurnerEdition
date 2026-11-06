#include "xbe_corpse_shield_physics.h"
#include "cpu/x86_ops.h"
#include "kernel/memory.h"
#include "xbe_input_memory.h"
#include <cmath>
#include <cstring>
#include <initializer_list>

namespace corpse_physics {
using Matrix=TrialCorpseShieldMatrix;
uint32_t Word(const uint8_t* b,uint32_t p){uint32_t v;std::memcpy(&v,b+p,4);return v;}
void Put(uint8_t* b,uint32_t p,uint32_t v){std::memcpy(b+p,&v,4);}
uint32_t Bits(float v){uint32_t u;std::memcpy(&u,&v,4);return u;}
bool ReadMatrix(const uint8_t* b,uint32_t p,Matrix& m){
    if(!TrialGuestReadable(p,64))return false;
    std::memcpy(m.data(),b+p,64);
    for(float v:m)if(!std::isfinite(v)||std::abs(v)>1.e7f)return false;
    return true;
}
bool Finite(const Matrix& m){for(float v:m)if(!std::isfinite(v)||std::abs(v)>1.e7f)return false;return true;}
Matrix Product(const Matrix& a,const Matrix& b){
    Matrix out{};
    for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c)
        out[r*4+c]=(a[r*4]*b[c]+a[r*4+1]*b[4+c])+(a[r*4+2]*b[8+c]+a[r*4+3]*b[12+c]);
    return out;
}
uint32_t World(const uint8_t* b){
    if(!b||!TrialGuestReadable(0x51d910,4))return 0;
    const uint32_t p=Word(b,0x51d910);
    return TrialGuestReadable(p,0x14)?Word(b,p+0x10):0;
}
bool Clone(const uint8_t* b,uint32_t r){
    return TrialGuestReadable(r,0xa0)&&Word(b,r)==0x48dd60&&Word(b,r+0x74)==12&&Word(b,r+0x80)==11&&
        TrialGuestReadable(Word(b,r+0x70),48)&&TrialGuestReadable(Word(b,r+0x7c),44);
}
// Separate guest stack avoids overwriting the original world-step caller's
// live frame. It is allocated once per held body, not once per physics step.
struct Calls {
    X86Context original,c;uint8_t* b;uint32_t sp,data,seh;unsigned csr=_mm_getcsr();
    Calls(const X86Context& context,uint8_t* base,uint32_t stack):original(context),c(context),b(base),
        sp(stack-1024),data(stack-512),seh(Word(b,c.fs_base)){_mm_setcsr(0x1f80);}
    ~Calls(){Put(b,original.fs_base,seh);_mm_setcsr(csr);}
    uint32_t Run(uint32_t fn,uint32_t self,std::initializer_list<uint32_t> args){
        c=original;c.esp=sp;c.ecx=self;c.df=0;
        for(auto p=args.end();p!=args.begin();){c.esp-=4;Put(b,c.esp,*--p);}
        c.esp-=4;Put(b,c.esp,0x29e5f3);x86_icall(c,b,fn);return c.eax;
    }
};
bool CanCall(const X86Context& c,const TrialCorpseShieldPhysics& s){
    return s.stack>=0x10000&&TrialGuestReadable(s.stack-0x10000,0x10000)&&TrialGuestReadable(c.fs_base,4);
}
bool ChestLocal(Calls& call,uint32_t model,Matrix& out){
    if(!TrialGuestReadable(model,4))return false;
    const uint32_t render=Word(call.b,model);
    if(!TrialGuestReadable(render,4))return false;
    const uint32_t table=Word(call.b,render);
    if(!TrialGuestReadable(table,0x28))return false;
    const uint32_t skeleton=call.Run(Word(call.b,table+0x24),render,{});
    if(!TrialGuestReadable(skeleton,4))return false;
    const uint32_t methods=Word(call.b,skeleton);
    if(!TrialGuestReadable(methods,12))return false;
    // Native2A4F90 maps physical body4 to original Spine2, model bone5.
    return ReadMatrix(call.b,call.Run(Word(call.b,methods+8),skeleton,{5,1}),out);
}
}

bool TrialCorpseShieldPhysicsValid(const uint8_t* b,const TrialCorpseShieldPhysics& s){
    using namespace corpse_physics;
    if(!b||!s.world||World(b)!=s.world||!Clone(b,s.ragdoll)||Word(b,s.ragdoll+0x88)!=s.world||
       Word(b,s.ragdoll+0x1c)!=s.model||!Word(b,s.ragdoll+0x20)||!b[s.ragdoll+0x24]||
       Word(b,Word(b,s.ragdoll+0x70)+16)!=s.chest||!TrialGuestReadable(s.chest,0x54))return false;
    const uint32_t motion=Word(b,s.chest+0x50);
    return TrialGuestReadable(motion,0x100)&&Word(b,motion)==0x49ae58;
}

void TrialCorpseShieldPhysicsDestroy(const X86Context& c,uint8_t* b,TrialCorpseShieldPhysics& s){
    using namespace corpse_physics;
    if(!s.stack){s={};return;}
    if(s.ragdoll){
        // World teardown calls this before retiring the native world. A stale
        // identity is a lifecycle error, never a reason to dereference freed RAM.
        if(!CanCall(c,s)||World(b)!=s.world||!Clone(b,s.ragdoll)){
            X86Context fault=c;x86_trap(fault,0x29e930,"Corpse shield lifetime escaped physics world");
        }
        Calls call(c,b,s.stack);call.Run(0x2a6920,s.ragdoll,{1});
    }
    mem::FreeStack(s.stack);s={};
}

bool TrialCorpseShieldPhysicsCreate(const X86Context& c,uint8_t* b,TrialCorpseShieldPhysics& s,
                                  uint32_t model,const TrialCorpseShieldMatrix& world,uint32_t owner){
    using namespace corpse_physics;
    if(s.stack||s.ragdoll||!b||!Finite(world)||!TrialGuestReadable(c.fs_base,4)||
       !TrialGuestReadable(0x51da44,48)||!b[0x51da6c]||!TrialGuestReadable(owner,0x98))return false;
    const uint32_t holder_id=Word(b,owner+0x94)&0xffff;
    if(holder_id==0xffff)return false;
    const uint32_t source=Word(b,0x51da48),native_world=World(b);
    if(!native_world||!Clone(b,source))return false;
    const uint32_t stack=mem::AllocStack(0x10000);if(!stack)return false;
    TrialCorpseShieldPhysics pending{};pending.stack=stack;pending.world=native_world;pending.model=model;
    struct Cleanup {const X86Context& c;uint8_t* b;TrialCorpseShieldPhysics& s;
        ~Cleanup(){if(s.stack)TrialCorpseShieldPhysicsDestroy(c,b,s);}} cleanup{c,b,pending};
    Calls call(c,b,stack);
    if(!ChestLocal(call,model,pending.chest_local))return false;
    const uint32_t allocation=call.Run(0x2d57fb,0,{0xa0});if(!allocation)return false;
    const uint32_t clone=call.Run(0x2a6130,allocation,{native_world,source+0x70,source+0x7c});
    if(clone!=allocation||!Clone(b,clone))x86_trap(call.c,0x2a6130,"Invalid corpse shield clone");
    pending.ragdoll=clone;pending.chest=Word(b,Word(b,clone+0x70)+16);
    if(!TrialGuestReadable(pending.chest,0x54))return false;
    // Original2A8920 excludes only this holder's collision group through
    // property18. Preserve each body's unique native filter/world contacts.
    for(unsigned i=0;i<12;++i){
        const uint32_t body=Word(b,Word(b,clone+0x70)+4*i);
        if(!TrialGuestReadable(body,0x54))return false;
        call.Run(0x2a3120,0,{body,0x18,holder_id});
    }
    // Type6 is the native keyframed motion. Converting before world insertion
    // keeps the eleven other dynamic bodies and all eleven native constraints.
    call.Run(0x2f1880,pending.chest,{6,1,0});
    std::memcpy(b+call.data,world.data(),64);std::memset(b+call.data+64,0,16);
    call.Run(0x2a5f00,clone,{model,0,call.data,call.data+64,0,0});
    call.Run(0x2a5da0,clone,{Bits(0.f),call.data});
    if(!TrialCorpseShieldPhysicsValid(b,pending))return false;
    s=pending;pending={};return true;
}

bool TrialCorpseShieldPhysicsBefore(const X86Context& c,uint8_t* b,TrialCorpseShieldPhysics& s,
                                  const TrialCorpseShieldMatrix& world,float dt){
    using namespace corpse_physics;
    if(!TrialCorpseShieldPhysicsValid(b,s)||!CanCall(c,s)||!Finite(world)||!std::isfinite(dt)||dt<=0||dt>.25f)return false;
    Matrix basis;if(!ReadMatrix(b,0x51da80,basis))return false;
    // Same row-matrix chain and cm-to-metres translation as native2A51C0.
    // Retain the original authored chest pose; never reseed the loose limbs.
    const Matrix target=Product(s.chest_local,Product(world,basis));
    if(!Finite(target))return false;
    Calls call(c,b,s.stack);std::memcpy(b+call.data,target.data(),64);
    const uint32_t quaternion=call.data+64,position=call.data+80;
    call.Run(0x2d5640,quaternion,{call.data});
    float q[4];std::memcpy(q,b+quaternion,16);float norm=0;
    for(float value:q){if(!std::isfinite(value))return false;norm+=value*value;}
    if(!std::isfinite(norm)||norm<.25f||norm>4.f)return false;
    for(float& value:q)value/=std::sqrt(norm);
    std::memcpy(b+quaternion,q,16);
    const float p[4]={target[12]*.01f,target[13]*.01f,target[14]*.01f,0};
    std::memcpy(b+position,p,16);
    // Native hard keyframe calculates COM-aware linear and angular velocity
    // from the target. It takes inverse dt, not dt. Solver owns every joint.
    call.Run(0x2e84a0,0,{position,quaternion,Bits(1.f/dt),s.chest});
    return true;
}

bool TrialCorpseShieldPhysicsAfter(const X86Context& c,uint8_t* b,TrialCorpseShieldPhysics& s,
                                 const TrialCorpseShieldMatrix& world,float dt){
    using namespace corpse_physics;
    if(!TrialCorpseShieldPhysicsValid(b,s)||!CanCall(c,s)||!Finite(world)||!std::isfinite(dt)||dt<=0||dt>.25f)return false;
    Calls call(c,b,s.stack);std::memcpy(b+call.data,world.data(),64);
    call.Run(0x2a5da0,s.ragdoll,{Bits(dt),call.data});
    return TrialCorpseShieldPhysicsValid(b,s);
}
