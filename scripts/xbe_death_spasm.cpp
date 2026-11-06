#include "xbe_death_spasm.h"
#include "xbe_coop_death.h"
#include "kernel/memory.h"
#include "xbe_input_memory.h"
#include "cpu/x86_ops.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <initializer_list>

namespace trial_graphics {bool DeathConvulsions();}
namespace {
uint32_t Word(const uint8_t* b,uint32_t p){uint32_t v;std::memcpy(&v,b+p,4);return v;}
float Real(const uint8_t* b,uint32_t p){float v;std::memcpy(&v,b+p,4);return v;}
void Put(uint8_t* b,uint32_t p,uint32_t v){std::memcpy(b+p,&v,4);}
struct FP {unsigned csr=_mm_getcsr();~FP(){_mm_setcsr(csr);}};
struct Reaction {
    uint32_t ragdoll=0,actor=0,id=0,model=0,world=0;float time=0;bool player=false;
};
// shortcut: 64 simultaneous reactions; overflow omits this cosmetic
// effect. Raise only if a verified scene exceeds this bound.
std::array<Reaction,64> reactions{};
unsigned active=0;
void Clear(Reaction& r){if(r.ragdoll){r={};--active;}}
bool Actor(const uint8_t* b,uint32_t a){
    return TrialGuestReadable(a,0x19c)&&!(Word(b,a+0x5c)&1)&&Word(b,a+0x54)!=UINT32_MAX;
}
bool Dead(const uint8_t* b,uint32_t a){const float hp=Real(b,a+0x9c);return std::isfinite(hp)&&hp<=0;}
bool Current(const uint8_t* b,const Reaction& r){
    return Actor(b,r.actor)&&Word(b,r.actor+0x54)==r.id&&Word(b,0x51c4dc)==r.world&&
        TrialGuestReadable(r.ragdoll,0xa0)&&Word(b,r.ragdoll)==0x48dd60&&
        Word(b,r.ragdoll+0x1c)==r.model&&Word(b,r.ragdoll+0x20)&&
        (r.player?TrialCoopKeepDownedBody(b,r.actor):Dead(b,r.actor));
}
uint32_t Call(X86Context& c,uint8_t* b,uint32_t fn,uint32_t object,std::initializer_list<uint32_t> args){
    struct Restore {X86Context& c;X86Context saved;unsigned csr;
        ~Restore(){c=saved;_mm_setcsr(csr);}} restore{c,c,_mm_getcsr()};
    c.ecx=object;
    for(auto i=args.end();i!=args.begin();){c.esp-=4;Put(b,c.esp,*--i);}
    c.esp-=4;Put(b,c.esp,0x2a6119);x86_icall(c,b,fn);return c.eax;
}
uint32_t Variation(uint32_t seed){
    seed^=seed>>16;seed*=0x7feb352du;seed^=seed>>15;seed*=0x846ca68bu;return seed^(seed>>16);
}
float Pulse(float t,uint32_t actor,unsigned part){
    if(t<=.6f||t>=5.f)return 0;
    const float u=t-.6f;const auto burst=uint32_t(u/1.45f);
    if(burst>=3)return 0;
    // Cosmetic variation depends on time/identity, never frame rate or the game's RNG.
    const uint32_t group=Variation(actor^((burst+1)*0x85ebca6bu));
    const float local=u-float(burst)*1.45f-.15f*float(group&255)/255.f;
    const float length=.38f+.18f*float((group>>8)&255)/255.f;
    if(local<=0||local>=length)return 0;
    const float edge=std::min({1.f,local/.05f,(length-local)/.08f});
    const float envelope=edge*edge*(3.f-2.f*edge);
    const uint32_t seed=Variation(group^((part+1)*0x9e3779b9u));
    const float frequency=7.f+2.f*float((group>>16)&255)/255.f;
    const float phase=6.2831853f*(frequency*local+.2f*float(seed&255)/255.f);
    return (.8f+.2f*float((seed>>8)&255)/255.f)*envelope*std::sin(phase);
}
using Vector=std::array<float,3>;
float Dot(const Vector& a,const Vector& b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
Vector Rotate(const uint8_t* b,uint32_t motion,uint32_t vector){
    Vector result{};
    for(unsigned row=0;row<3;++row)for(unsigned col=0;col<3;++col)
        result[row]+=Real(b,motion+0x10+col*16+row*4)*Real(b,vector+col*4);
    return result;
}
float HingeAngle(const uint8_t* b,uint32_t ma,uint32_t mb,uint32_t data){
    const auto a=Rotate(b,ma,data+0x40),p=Rotate(b,mb,data+0x80),axis=Rotate(b,mb,data+0x70);
    const Vector perpendicular={p[1]*axis[2]-p[2]*axis[1],p[2]*axis[0]-p[0]*axis[2],p[0]*axis[1]-p[1]*axis[0]};
    // Same pivot/axis/perpendicular layout and angle convention as native 2F4AF0.
    return std::atan2(Dot(p,a),Dot(perpendicular,a));
}
uint32_t Motion(const uint8_t* b,uint32_t body){
    if(!TrialGuestReadable(body,0x92))return 0;
    const uint32_t m=Word(b,body+0x50);
    if(!TrialGuestReadable(m,0x100)||Word(b,m)!=0x49a8b8)return 0;
    for(unsigned row=0;row<3;++row){
        for(unsigned col=0;col<3;++col)if(!std::isfinite(Real(b,m+0x10+col*16+row*4)))return 0;
        const float inverse=Real(b,m+0xf0+row*4),velocity=Real(b,m+0xe0+row*4);
        if(!std::isfinite(inverse)||inverse<=0||inverse>100||!std::isfinite(velocity))return 0;
    }
    return m;
}
void Apply(X86Context& c,uint8_t* b,Reaction& r,float end){
    const uint32_t list=Word(b,r.ragdoll+0x70),joints=Word(b,r.ragdoll+0x7c),count=Word(b,r.ragdoll+0x80);
    if(Word(b,r.ragdoll+0x74)!=12||count!=11||!TrialGuestReadable(list,48)||!TrialGuestReadable(joints,44))return;
    // Authored chain: spine, hips, chest, knees, shoulders, neck and elbows.
    constexpr unsigned pairs[11][2]={{1,0},{2,0},{3,0},{4,1},{5,2},{6,3},{7,4},{8,4},{9,4},{10,9},{11,7}};
    constexpr float travel[11]={.23f,.184f,.184f,.253f,.12f,.12f,.276f,.10f,.276f,.18f,.18f};
    constexpr float strength=1.3f,acceleration=260.f*strength;
    for(unsigned part=0;part<11;++part){
        const float t=end,old=r.time;
        constexpr float begin=.6f,finish=5.1f;
        if(t<=begin||old>=finish)continue;
        const float dt=std::min(t,finish)-std::max(old,begin);if(dt<=0)continue;
        const float now=Pulse(t,r.id,part),before=Pulse(old,r.id,part);
        if(now==0&&before==0)continue; // Let native contacts/damping settle the body in each gap.
        const uint32_t a=Word(b,list+pairs[part][0]*4),parent=Word(b,list+pairs[part][1]*4);
        const uint32_t ma=Motion(b,a),mb=Motion(b,parent);if(!ma||!mb)continue;
        uint32_t data=0;
        for(unsigned j=0;j<count;++j){
            const uint32_t joint=Word(b,joints+j*4);
            if(!TrialGuestReadable(joint,0x18)||Word(b,joint+0x10)!=a||Word(b,joint+0x14)!=parent)continue;
            const uint32_t d=Word(b,joint+12);
            if(TrialGuestReadable(d,0x90)&&(Word(b,d)==0x49a9d4||Word(b,d)==0x49de88)){data=d;break;}
        }
        if(!data)continue;
        const bool hinge=Word(b,data)==0x49a9d4;
        // Reflected ConstraintBasisA: hinge axle +30; ragdoll planeAxis +20 (pivot is +10).
        const auto torque=Rotate(b,ma,data+(hinge?0x30:0x20));const float length=Dot(torque,torque);
        if(!std::isfinite(length)||length<.9f||length>1.1f)continue;
        float inverse=0,velocity=0;
        for(unsigned side=0;side<2;++side){const auto motion=side?mb:ma;
            for(unsigned axis=0;axis<3;++axis){
                float local=0;for(unsigned row=0;row<3;++row)local+=Real(b,motion+0x10+axis*16+row*4)*torque[row];
                inverse+=local*local*Real(b,motion+0xf0+axis*4);
                velocity+=(side?-1.f:1.f)*torque[axis]*Real(b,motion+0xe0+axis*4);
            }
        }
        if(!std::isfinite(inverse)||inverse<=.0001f||!std::isfinite(velocity))continue;
        // Difference the displacement waveform over elapsed time; no per-frame kick.
        const float speed=strength*(part==7?5.f:(hinge?8.f:9.f));
        float desired=now==0?0.f:std::clamp(strength*travel[part]*(now-before)/dt,-speed,speed);
        if(hinge){
            const float angle=HingeAngle(b,ma,mb,data),low=Real(b,data+12),high=Real(b,data+16);
            if(!std::isfinite(angle)||!std::isfinite(low)||!std::isfinite(high)||high-low<.1f)continue;
            const auto braking=[&](float distance){const float dv=acceleration*dt;
                return std::sqrt(dv*dv+2.f*acceleration*std::max(0.f,distance))-dv;};
            desired=std::clamp(desired,-braking(angle-low-.02f),braking(high-.02f-angle));
        }
        const float impulse=std::clamp(desired-velocity,-acceleration*dt,acceleration*dt)/inverse;
        // Equal and opposite joint impulses preserve net angular momentum;
        // the original solver owns limits/contacts. Never add linear velocity.
        const uint32_t saved_sp=c.esp,scratch=(saved_sp-16)&~15u;
        c.esp=scratch;
        for(unsigned side=0;side<2;++side){
            float value[4]={};for(unsigned axis=0;axis<3;++axis)value[axis]=torque[axis]*impulse*(side?-1.f:1.f);
            std::memcpy(b+scratch,value,16);
            Call(c,b,0x2ecaa0,side?parent:a,{});
            Call(c,b,0x2f5240,side?mb:ma,{scratch});
        }
        c.esp=saved_sp;
    }
}
}
void TrialDeathSpasmActivated(X86Context& c,uint8_t* b){
    FP fp;
    if(!trial_graphics::DeathConvulsions())return;
    if(!b||!TrialGuestReadable(c.ebp,32)||!TrialGuestReadable(c.esi,0xa0))return;
    for(auto& r:reactions)if(r.ragdoll==c.esi)Clear(r);
    const uint32_t model=Word(b,c.ebp+8),actor=Word(b,c.ebp+12);
    if(!model||!Actor(b,actor)||Word(b,c.esi)!=0x48dd60||Word(b,c.esi+0x1c)!=model||
       !Word(b,c.esi+0x20)||!TrialGuestReadable(0x51c4dc,4)||c.esp<1024||!TrialGuestReadable(c.esp-1024,1024))return;
    const bool player=TrialCoopImmediateRagdoll(b,actor);
    if(!player){
        if((Word(b,actor)!=0x45c770&&Word(b,actor)!=0x47bf50)||!Dead(b,actor))return;
        if(!TrialGuestReadable(Word(b,actor+0x114),0x20))return;
        // Retail E18C3 queries this same named property; class alone includes allies.
        if(!(Call(c,b,0x20a690,actor,{0x44da54,0})&0xff))return;
    }
    for(auto& r:reactions){
        if(r.ragdoll&&!Current(b,r))Clear(r);
        if(!r.ragdoll){r={c.esi,actor,Word(b,actor+0x54),model,Word(b,0x51c4dc),0,player};++active;return;}
    }
}
void TrialDeathSpasmStep(X86Context& c,uint8_t* b){
    if(!active||!b)return;
    FP fp;
    if(!trial_graphics::DeathConvulsions()){reactions={};active=0;return;}
    for(auto& r:reactions)if(r.ragdoll==c.esi){
        if(!Current(b,r)){Clear(r);return;}
        float dt;std::memcpy(&dt,&c.ebx,4);
        if(!std::isfinite(dt)||dt<=0)return;
        if(dt>1||c.esp<1024||!TrialGuestReadable(c.esp-1024,1024)){Clear(r);return;}
        const float end=std::min(5.4f,r.time+dt);Apply(c,b,r,end);r.time=end;
        if(end>=5.4f)Clear(r);
        return;
    }
}
