#include "xbe_ragdoll_drag.h"
#include "xbe_ragdoll_settings.h"
#include "xbe_coop.h"
#include "xbe_coop_guest.h"
#include "xbe_coop_death.h"
#include "xbe_coop_cinematics.h"
#include "cpu/x86_ops.h"
#include "kernel/memory.h"
#include "xbe_input_memory.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstring>
#include <initializer_list>

namespace ragdoll_drag {
using Vector=std::array<float,3>;
constexpr double hold_seconds=.45,retry_seconds=.1;
constexpr float lift_metres=.18f;
std::atomic<uint64_t> input[2]{};
struct Grab {
    uint32_t handle=UINT32_MAX,ragdoll=0,body=0,actor=0,id=0,model=0;
    unsigned limb=0;
    Vector offset{},target{},velocity{};
    float sampled=0;
};
struct Player {
    uint32_t owner=0,id=0,world=0,level=0;
    uint64_t host=0;
    double held=0;
    bool blocked=true,fired=false;
    Grab grab;
};
Player players[2];
uint32_t Word(const uint8_t* b,uint32_t p){uint32_t v;std::memcpy(&v,b+p,4);return v;}
float Real(const uint8_t* b,uint32_t p){float v;std::memcpy(&v,b+p,4);return v;}
void Put(uint8_t* b,uint32_t p,uint32_t v){std::memcpy(b+p,&v,4);}
struct FP {unsigned csr=_mm_getcsr();~FP(){_mm_setcsr(csr);}};
float Dot(const Vector& a,const Vector& b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
bool ReadVector(const uint8_t* b,uint32_t p,Vector& v){
    if(!TrialGuestReadable(p,12))return false;
    std::memcpy(v.data(),b+p,12);
    return std::isfinite(v[0])&&std::isfinite(v[1])&&std::isfinite(v[2]);
}
bool Entity(const uint8_t* b,uint32_t level,uint32_t actor,uint32_t id){
    if(id==UINT32_MAX||!TrialGuestReadable(level,0x203c)||!TrialGuestReadable(actor,0x60))return false;
    const uint32_t entry=level+0x3c+(id&1023)*8;
    return Word(b,entry)==id&&Word(b,entry+4)==actor&&Word(b,actor+0x54)==id&&!(Word(b,actor+0x5c)&1);
}
bool Identity(const uint8_t* b,uint32_t owner,unsigned& slot,uint32_t& world,uint32_t& level){
    if(!b||!TrialGuestReadable(owner,0x890)||Word(b,owner)!=0x46fbe8||!TrialGuestReadable(0x51c4dc,4))return false;
    slot=Word(b,owner+0x368);world=Word(b,0x51c4dc);
    if(slot>1||!TrialGuestReadable(world,0x880)||
       (slot&&(!trial_coop::Active()||TrialCoopPlayer(slot)!=owner)))return false;
    level=Word(b,world+4);
    return Entity(b,level,owner,Word(b,owner+0x54));
}
bool Available(const uint8_t* b,const Player& p){
    unsigned slot;uint32_t world,level;
    if(!Identity(b,p.owner,slot,world,level)||&players[slot]!=&p||world!=p.world||level!=p.level||Word(b,p.owner+0x54)!=p.id||
       input[slot].load()!=p.host||!(p.host&1)||TrialCoopPresentation()==2)return false;
    const float health=Real(b,p.owner+0x9c);const auto state=Word(b,p.owner+0x170);
    return std::isfinite(health)&&health>0&&state!=3&&state!=7&&state!=11&&state!=12&&state!=13&&
        !b[p.owner+0x202]&&!(Word(b,p.world+0x10)&0x800);
}
bool Draggable(const uint8_t* b,const Player& p,const Grab& g){
    if(!TrialGuestReadable(g.actor,0x19c)||!Entity(b,p.level,g.actor,g.id))return false;
    const auto type=Word(b,g.actor);
    if(type==0x45d040)return TrialCoopKeepDownedBody(b,g.actor);
    const float health=Real(b,g.actor+0x9c);
    if((type!=0x45c770&&type!=0x47bf50)||!std::isfinite(health))return false;
    if(health<=0)return true;
    // Retail DF380 marks knockouts with bit 4; only their currently owned ragdoll is movable.
    if(!(Word(b,g.actor+0x198)&4))return false;
    const auto owner=Word(b,g.actor+0x118);
    if(!TrialGuestReadable(owner,16)||Word(b,owner)!=0x48e6e0)return false;
    const auto data=Word(b,owner+12);
    return TrialGuestReadable(data,0x54)&&Word(b,data+0x50)==g.handle;
}
bool Current(const uint8_t* b,const Player& p){
    const auto& g=p.grab;
    return g.body&&Available(b,p)&&Draggable(b,p,g)&&
        TrialRagdollMatchesLimb(b,g.handle,g.ragdoll,g.limb,g.body)&&
        Word(b,g.ragdoll+0x1c)==g.model;
}
void Cancel(Player& p){p.grab={};p.held=0;p.blocked=true;p.fired=false;}
bool Stack(const X86Context& c){return c.esp>=0x11000&&TrialGuestReadable(c.esp-4096,4096);}
struct Calls {
    X86Context original,c;
    uint8_t* b;uint32_t low,sp,data;
    std::array<uint8_t,4096> saved;
    unsigned csr=_mm_getcsr();
    Calls(const X86Context& context,uint8_t* base):original(context),c(context),b(base),low(c.esp-4096),
        sp((c.esp-512)&~15u),data((c.esp-256)&~15u){std::memcpy(saved.data(),b+low,saved.size());}
    ~Calls(){std::memcpy(b+low,saved.data(),saved.size());_mm_setcsr(csr);}
    uint32_t Run(uint32_t fn,uint32_t self,std::initializer_list<uint32_t> args){
        c=original;c.esp=sp;c.ecx=self;
        for(auto it=args.end();it!=args.begin();){c.esp-=4;Put(b,c.esp,*--it);}
        c.esp-=4;Put(b,c.esp,0xd2ab0);x86_icall(c,b,fn);return c.eax;
    }
};
struct View {Vector eye;std::array<Vector,3> axis;};
bool Look(const uint8_t* b,uint32_t owner,View& view){
    if(!ReadVector(b,owner+0x6e0,view.eye))return false;
    for(unsigned i=0;i<3;++i){
        if(!ReadVector(b,owner+0x6b0+i*16,view.axis[i]))return false;
        const float length=Dot(view.axis[i],view.axis[i]);
        if(!std::isfinite(length)||length<.5f||length>1.5f)return false;
        for(auto& v:view.axis[i])v/=std::sqrt(length);
        for(unsigned j=0;j<i;++j)if(std::abs(Dot(view.axis[i],view.axis[j]))>.05f)return false;
    }
    return true;
}
uint32_t Motion(const uint8_t* b,uint32_t body,Vector& position,Vector& velocity){
    if(!TrialGuestReadable(body,0x93))return 0;
    const uint32_t m=Word(b,body+0x50);
    if(!TrialGuestReadable(m,0x100)||Word(b,m)!=0x49a8b8||
       !ReadVector(b,m+0x60,position)||!ReadVector(b,m+0xd0,velocity))return 0;
    const float inverse=Real(b,m+0xc0);
    return std::isfinite(inverse)&&inverse>0&&inverse<=100?m:0;
}
Vector Game(const Vector& v){return {v[0]*100.f,v[2]*100.f,v[1]*100.f};}
Vector Physics(const Vector& v){return {v[0]*.01f,v[2]*.01f,v[1]*.01f};}
struct Hit {uint32_t body=0,actor=0;Vector point{};float distance=0;bool present=false;};
bool Ray(Calls& call,uint32_t owner,const Vector& from,const Vector& to,Hit& hit){
    const uint32_t a=call.data,z=a+16;
    std::memcpy(call.b+a,from.data(),12);std::memcpy(call.b+z,to.data(),12);
    const uint32_t result=call.Run(0x214230,0,{a,z,9,Word(call.b,owner+0x94)&0xffff,0x4000,0});
    if(!result){hit={};return true;}
    if(!TrialGuestReadable(result,0x30)||!ReadVector(call.b,result,hit.point))return false;
    hit.present=true;
    hit.body=Word(call.b,result+0x24);hit.actor=Word(call.b,result+0x2c);hit.distance=Real(call.b,result+0x18);
    return std::isfinite(hit.distance)&&hit.distance>=0;
}
bool Visible(Calls& call,const Player& p,const View& view,const Vector& point){
    Hit hit;if(!Ray(call,p.owner,view.eye,point,hit))return false;
    if(!hit.present)return true;
    if(!hit.body)return false;
    uint32_t handle,ragdoll;unsigned limb;
    return TrialRagdollFindLimb(call.b,hit.body,handle,ragdoll,limb)&&
        handle==p.grab.handle&&ragdoll==p.grab.ragdoll&&hit.actor==p.grab.actor;
}
bool Target(const uint8_t* b,const Player& p,const View& view,Vector& target,Vector& position){
    Vector velocity;if(!Motion(b,p.grab.body,position,velocity))return false;
    const Vector game=Game(position);Vector delta;
    for(unsigned i=0;i<3;++i){
        delta[i]=game[i]-view.eye[i];target[i]=view.eye[i];
        for(unsigned j=0;j<3;++j)target[i]+=view.axis[j][i]*p.grab.offset[j];
    }
    if(!std::isfinite(Dot(delta,delta))||Dot(delta,delta)>350.f*350.f)return false;
    target=Physics(target);
    target[2]+=lift_metres;
    for(unsigned i=0;i<3;++i)delta[i]=target[i]-position[i];
    return std::isfinite(Dot(delta,delta))&&Dot(delta,delta)<=2.25f;
}
bool Acquire(Calls& call,Player& p,const View& view){
    Vector end;for(unsigned i=0;i<3;++i)end[i]=view.eye[i]+view.axis[2][i]*200.f;
    Hit hit;if(!Ray(call,p.owner,view.eye,end,hit)||!hit.body||!hit.actor||hit.distance>200.f)return false;
    Grab g;g.body=hit.body;g.actor=hit.actor;
    if(!TrialRagdollFindLimb(call.b,g.body,g.handle,g.ragdoll,g.limb))return false;
    // Authored limbs, Spine2/chest (4) and Head (8); pelvis/lower spine stay ordinary Use.
    if(g.limb!=2&&g.limb!=3&&g.limb!=4&&g.limb!=5&&g.limb!=6&&g.limb!=7&&g.limb!=8&&g.limb!=9&&g.limb!=10&&g.limb!=11)return false;
    if(!TrialGuestReadable(g.actor,0x19c))return false;
    g.id=Word(call.b,g.actor+0x54);g.model=Word(call.b,g.ragdoll+0x1c);
    if(!Draggable(call.b,p,g)||!Available(call.b,p))return false;
    for(auto& other:players)if(&other!=&p&&other.grab.ragdoll==g.ragdoll){
        if(Current(call.b,other))return false;
        Cancel(other);
    }
    Vector position,velocity;if(!Motion(call.b,g.body,position,velocity))return false;
    const auto game=Game(position);Vector delta;
    for(unsigned i=0;i<3;++i)delta[i]=game[i]-view.eye[i];
    if(!std::isfinite(Dot(delta,delta))||Dot(delta,delta)>225.f*225.f)return false;
    for(unsigned i=0;i<3;++i)g.offset[i]=Dot(delta,view.axis[i]);
    g.target=position;g.target[2]+=lift_metres;g.sampled=Real(call.b,p.world+8);p.grab=g;return true;
}
}

void TrialRagdollDragInput(unsigned slot,bool allowed){
    using namespace ragdoll_drag;if(slot>1)return;
    uint64_t old=input[slot].load(),next;
    do{next=(old&~uint64_t(1))+((!allowed&&(old&1))?2:0)+(allowed?1:0);}
    while(!input[slot].compare_exchange_weak(old,next));
}
void TrialRagdollDragUpdate(const X86Context& c,uint8_t* b){
    using namespace ragdoll_drag;FP fp;
    unsigned slot;uint32_t world,level;
    if(!Identity(b,c.ecx,slot,world,level)){for(auto& p:players)if(p.owner==c.ecx)Cancel(p);return;}
    auto& p=players[slot];const auto host=input[slot].load();const auto id=Word(b,c.ecx+0x54);
    if(p.owner!=c.ecx||p.id!=id||p.world!=world||p.level!=level||p.host!=host){
        p={};p.owner=c.ecx;p.id=id;p.world=world;p.level=level;p.host=host;
    }
    const float dt=Real(b,world+0x14),now=Real(b,world+8);
    if(!Available(b,p)||!std::isfinite(dt)||dt<=0||dt>.25f||!std::isfinite(now)||!Stack(c)){Cancel(p);return;}
    Calls call(c,b);View view;if(!Look(b,p.owner,view)){Cancel(p);return;}
    if(p.grab.body){
        Vector position,target;
        if(!Current(b,p)||!Target(b,p,view,target,position)||!Visible(call,p,view,Game(position))){Cancel(p);return;}
        const float elapsed=now-p.grab.sampled;
        for(unsigned i=0;i<3;++i)p.grab.velocity[i]=elapsed>0&&elapsed<=.25f?(target[i]-p.grab.target[i])/elapsed:0;
        const float speed=std::sqrt(Dot(p.grab.velocity,p.grab.velocity));
        if(speed>6.f)for(auto& value:p.grab.velocity)value*=6.f/speed;
        p.grab.target=target;p.grab.sampled=now;
    }
    // The host maps Interact to retail A (0x16); X (0x18) cycles firearms.
    call.Run(0x567f0,world,{slot,0x16,0});
    const double value=double(call.c.st[call.c.top&7]);
    if(!std::isfinite(value)||!Available(b,p)){Cancel(p);return;}
    if(value<=0){p.blocked=false;p.fired=false;p.held=0;return;}
    if(p.blocked||p.fired)return;
    p.held+=dt;
    if(p.held<hold_seconds)return;
    if(p.grab.body){p.grab={};p.fired=true;}else p.fired=Acquire(call,p,view);
    // A missed limb must not consume the hold; retry at most ten times per second.
    p.held=p.fired?0:hold_seconds-retry_seconds;
}
void TrialRagdollDragStep(X86Context& c,uint8_t* b){
    using namespace ragdoll_drag;FP fp;
    for(auto& p:players)if(p.grab.ragdoll&&p.grab.ragdoll==c.esi){
        float dt;std::memcpy(&dt,&c.ebx,4);
        if(!b||!Current(b,p)||!std::isfinite(dt)||dt<=0||dt>.25f||!Stack(c)){Cancel(p);return;}
        const float elapsed=Real(b,p.world+8)-p.grab.sampled;
        if(!std::isfinite(elapsed)||elapsed<0||elapsed>std::max(.1f,dt*2.f)){Cancel(p);return;}
        Vector position,velocity;const uint32_t motion=Motion(b,p.grab.body,position,velocity);
        if(!motion){Cancel(p);return;}
        Vector force,error;const float inverse=Real(b,motion+0xc0);
        for(unsigned i=0;i<3;++i)error[i]=p.grab.target[i]-position[i];
        if(!std::isfinite(Dot(error,error))||Dot(error,error)>2.25f){Cancel(p);return;}
        // Approach a bounded pull velocity, instead of storing a large elastic force.
        // The native solver still distributes this limb's impulse through contacts and joints.
        Vector desired;for(unsigned i=0;i<3;++i)desired[i]=error[i]*10.f;
        const float speed=std::sqrt(Dot(desired,desired));
        if(speed>5.f)for(auto& value:desired)value*=5.f/speed;
        // Follow camera travel as well as position error, so walking cannot outrun
        // a fixed pull-speed cap. Both contributions and acceleration remain bounded.
        for(unsigned i=0;i<3;++i)desired[i]+=p.grab.velocity[i];
        const float response=-1.4f*std::expm1(-90.f*dt)/(dt*inverse);
        for(unsigned i=0;i<3;++i)force[i]=(desired[i]-velocity[i])*response;
        const float magnitude=std::sqrt(Dot(force,force));
        if(!std::isfinite(magnitude)){Cancel(p);return;}
        const float limit=240.f/inverse;
        if(magnitude>limit)for(auto& value:force)value*=limit/magnitude;
        Calls call(c,b);float vector[4]={force[0],force[1],force[2],0};
        std::memcpy(b+call.data,vector,sizeof vector);
        if(!Available(b,p)){Cancel(p);return;}
        call.Run(0x2ecaa0,p.grab.body,{});
        if(!Current(b,p)){Cancel(p);return;}
        call.Run(0x2f4ff0,motion,{c.ebx,call.data});return;
    }
}
