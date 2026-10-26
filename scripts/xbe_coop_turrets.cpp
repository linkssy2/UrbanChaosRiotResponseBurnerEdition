#include "xbe_coop_turrets.h"
#include "xbe_coop_turret_mount.h"
#include "xbe_coop_turret_assets.h"
#include "xbe_coop.h"
#include "xbe_coop_guest.h"
#include "cpu/x86_ops.h"
#include "kernel/kernel.h"
#include "kernel/memory.h"
#include "xbe_input_memory.h"
#include <cmath>
#include <cstring>
#include <xmmintrin.h>

namespace {
thread_local TrialCoopTurretScope* current=nullptr;
thread_local bool relaying=false;
struct Mount {uint32_t world=0,vehicle=0,vehicle_id=0,owner=0,owner_id=0;bool equipped=false;};
std::array<Mount,2> mounts{};
uint32_t Word(const uint8_t* b,uint32_t p){uint32_t v;std::memcpy(&v,b+p,4);return v;}
float Real(const uint8_t* b,uint32_t p){float v;std::memcpy(&v,b+p,4);return v;}
void Put(uint8_t* b,uint32_t p,uint32_t v){std::memcpy(b+p,&v,4);}
bool Player(const uint8_t* b,uint32_t actor,uint32_t id,unsigned slot){
    return TrialGuestReadable(actor,0x890)&&Word(b,actor)==0x46fbe8u&&
        Word(b,actor+0x54)==id&&id!=0xffffffffu&&!(Word(b,actor+0x5c)&1)&&
        Word(b,actor+0x368)==slot&&TrialCoopPlayer(slot)==actor;
}
bool Alive(const uint8_t* b,uint32_t actor){
    const uint32_t health=Word(b,actor+0x9c);
    return health>0&&health<0x7f800000u&&Word(b,actor+0x170)!=12;
}
bool Matrix(const std::array<float,16>& m){
    for(float value:m)if(!std::isfinite(value))return false;
    const float determinant=m[0]*(m[5]*m[10]-m[6]*m[9])-
        m[4]*(m[1]*m[10]-m[2]*m[9])+m[8]*(m[1]*m[6]-m[2]*m[5]);
    return std::isfinite(determinant)&&std::fabs(determinant)>1.e-6f&&
        std::fabs(m[3])<1.e-5f&&std::fabs(m[7])<1.e-5f&&std::fabs(m[11])<1.e-5f&&std::fabs(m[15]-1.f)<1.e-5f;
}
bool Vehicle(const uint8_t* b,uint32_t actor,uint32_t id){
    return TrialGuestReadable(actor,0x1c0)&&Word(b,actor)==0x4753a0u&&
        Word(b,actor+0x54)==id&&id!=0xffffffffu&&!(Word(b,actor+0x5c)&1);
}
bool Pair(const uint8_t* b,TrialCoopTurretScope& s){
    if(!b||!trial_coop::Available()||!trial_coop::Requested()||!trial_coop::Active()||
       !TrialGuestReadable(0x51c4dc,4))return false;
    s.world=Word(b,0x51c4dc);
    if(!TrialGuestReadable(s.world,8))return false;
    const uint32_t state=Word(b,s.world+4);
    if(!TrialGuestReadable(state,12))return false;
    const uint32_t first=Word(b,state+4);
    if(first>0xfffffff7u||Word(b,state+8)!=first+8||!TrialGuestReadable(first,8))return false;
    for(unsigned slot=0;slot<2;++slot){
        s.actors[slot]=TrialCoopPlayer(slot);
        if(!TrialGuestReadable(s.actors[slot],0x890))return false;
        s.ids[slot]=Word(b,s.actors[slot]+0x54);
        const uint32_t layer=Word(b,first+4*slot);
        if(!Player(b,s.actors[slot],s.ids[slot],slot)||!TrialGuestReadable(layer,0x28)||
           Word(b,layer+4)!=s.actors[slot]||Word(b,layer+0x24)!=slot)return false;
    }
    return s.actors[0]!=s.actors[1]&&s.ids[0]!=s.ids[1];
}
void Remember(const TrialCoopTurretScope& s){
    for(unsigned i=0;i<2;++i){
        const auto& old=mounts[i];
        const bool equipped=old.world==s.world&&old.vehicle==s.vehicle&&old.vehicle_id==s.vehicle_id&&
            old.owner==s.actors[i]&&old.owner_id==s.ids[i]&&old.equipped;
        mounts[i]={s.world,s.vehicle,s.vehicle_id,s.actors[i],s.ids[i],equipped};
    }
}
bool Equipped(const TrialCoopTurretScope& s,unsigned slot){
    const auto& m=mounts[slot];
    return m.world==s.world&&m.vehicle==s.vehicle&&m.vehicle_id==s.vehicle_id&&
        m.owner==s.actors[slot]&&m.owner_id==s.ids[slot]&&m.equipped;
}
}

void TrialCoopTurretsReset(){mounts={};}

uint32_t TrialCoopTurretMount(const uint8_t* b,uint32_t owner){
    if(!b||!trial_coop::Active()||!TrialGuestReadable(0x51c4dc,4))return 0;
    for(unsigned slot=0;slot<2;++slot){
        const auto& m=mounts[slot];
        if(m.owner!=owner||Word(b,0x51c4dc)!=m.world||
           !Player(b,owner,m.owner_id,slot)||!Alive(b,owner)||
           !Vehicle(b,m.vehicle,m.vehicle_id)||!TrialCoopTurretAssetReady(b,m.vehicle)||
           !b[m.vehicle+0x171]||b[m.vehicle+0x172]||
           Word(b,owner+0x4fc)!=0x19)continue;
        return m.vehicle;
    }
    return 0;
}

TrialCoopTurretScope::TrialCoopTurretScope(X86Context& c,uint8_t* b,Phase p)
    :phase(p),previous_(current){
    current=nullptr;
    if(!Pair(b,*this)||!TrialGuestReadable(c.ecx,0x1c0))return;
    const uint32_t id=Word(b,c.ecx+0x54);
    if(!Vehicle(b,c.ecx,id)||!TrialCoopTurretAssetReady(b,c.ecx))return;
    vehicle=c.ecx;vehicle_id=id;
    // C3F19/C3F2B is the original "pitch" property queried by 199A10.
    pitch=Real(b,actors[1]+0x688);
    if(!std::isfinite(pitch)){vehicle=0;return;}
    current=this;
}
TrialCoopTurretScope::~TrialCoopTurretScope(){current=previous_;}
bool TrialCoopTurretScope::Valid(const uint8_t* b)const{
    return b&&vehicle&&trial_coop::Active()&&Word(b,0x51c4dc)==world&&
        Vehicle(b,vehicle,vehicle_id)&&TrialCoopTurretAssetReady(b,vehicle)&&
        Player(b,actors[0],ids[0],0)&&Player(b,actors[1],ids[1],1);
}

void TrialCoopTurretRecipient(X86Context& c,const uint8_t* b){
    // Only the seven pinned vehicle caller sites use this correction. The
    // original manager's 4ACD0 getter ignores its slot and returns the last actor.
    if(current&&!relaying&&current->Valid(b)&&c.esi==current->vehicle&&
       (c.eax==current->ids[0]||c.eax==current->ids[1]))c.eax=current->ids[0];
}

void TrialCoopTurretAim(const X86Context& c,const uint8_t* b){
    if(!current||relaying||current->phase!=TrialCoopTurretScope::Phase::Update||
       !current->Valid(b)||c.esi!=current->vehicle||!TrialGuestReadable(c.esp+0x18,4))return;
    current->yaw=Real(b,c.esp+0x18);current->aim=std::isfinite(current->yaw);
}

void TrialCoopTurretCameraSlot(const X86Context& c,uint8_t* b){
    // CC190 equips each real inventory. Its C8930 model setup must write the
    // selected weapon's projection to that owner's camera, not always slot 0.
    if(!current||!current->Valid(b)||c.esi!=current->actors[1]||
       Word(b,c.esi+0x4fc)!=0x19||!TrialGuestReadable(c.esp,4)||Word(b,c.esp))return;
    Put(b,c.esp,1);
}

bool TrialCoopTurretEvent(X86Context& c,uint8_t* b){
    auto* s=current;
    if(!s||relaying||!s->Valid(b)||c.esp<0x11000||
       !TrialGuestReadable(c.esp-0x400,0x414))return true;
    // This hook precedes the actual call instruction: no return address is on
    // the stack yet. 1F90A0/20A540 dispatch the copied payload synchronously.
    const uint32_t target=Word(b,c.esp),event=Word(b,c.esp+4),data=Word(b,c.esp+8);
    const uint32_t size=Word(b,c.esp+12),sender=Word(b,c.esp+16);
    if(target!=s->ids[0]||!TrialGuestReadable(sender,4)||Word(b,sender)!=s->vehicle_id||
       !size||size>0x60||!TrialGuestReadable(data,size)||!TrialGuestReadable(0x51c398,4))return true;
    const uint32_t services=Word(b,0x51c398);
    if(!TrialGuestReadable(services,12)||c.ecx!=Word(b,services+8))return true;
    const bool matrix=event==Word(b,0x4f9f2c)&&size==0x60;
    const bool equip=event==Word(b,0x4f9e0c)&&size==8&&Word(b,data)==0x19&&b[data+4]==1;
    const bool remove=event==Word(b,0x4f9fec)&&size==4&&Word(b,data)==0x19;
    const bool yaw=event==Word(b,0x4f9f28)&&size==8;
    const bool pitch=event==Word(b,0x4f9f24)&&size==8;
    if(!(matrix||equip||remove||yaw||pitch))return true;
    const bool relay=remove?Equipped(*s,1):Alive(b,s->actors[1]);
    const bool primary_alive=Alive(b,s->actors[0]);
    const bool primary_dispatch=remove?Equipped(*s,0):primary_alive;
    if(matrix&&(s->phase==TrialCoopTurretScope::Phase::Exit||
               (s->phase==TrialCoopTurretScope::Phase::State&&b[data]!=0)||
               (s->phase==TrialCoopTurretScope::Phase::Update&&b[data]!=1)))return true;
    if((yaw||pitch)&&(s->phase!=TrialCoopTurretScope::Phase::Update||!s->aim))return true;
    if(equip&&s->phase!=TrialCoopTurretScope::Phase::State)return true;
    if(remove&&s->phase!=TrialCoopTurretScope::Phase::Exit)return true;

    const uint32_t scratch=(c.esp-0x400)&~15u;
    if(!TrialGuestReadable(scratch,0x400))return true;
    struct Restore {
        uint8_t* b;uint32_t scratch;std::array<uint8_t,0x400> bytes;unsigned csr;bool old;
        ~Restore(){std::memcpy(b+scratch,bytes.data(),bytes.size());_mm_setcsr(csr);relaying=old;}
    } restore{b,scratch,{},_mm_getcsr(),relaying};
    std::memcpy(restore.bytes.data(),b+scratch,restore.bytes.size());
    std::memcpy(b+scratch,b+data,size);Put(b,scratch+0x70,s->vehicle_id);
    std::array<float,16> primary{};
    if(matrix){
        std::array<float,16> original{};std::memcpy(original.data(),b+scratch+0x10,64);
        if(!Matrix(original))return true;
        for(unsigned slot=0;slot<2;++slot){
            auto m=original;const auto& d=coop_turret_mount::kPlayerOffset[slot];
            for(unsigned axis=0;axis<3;++axis){
                m[12+axis]+=m[axis]*d[0]+m[4+axis]*d[1]+m[8+axis]*d[2];
                if(!std::isfinite(m[12+axis]))return true;
            }
            if(slot)std::memcpy(b+scratch+0x10,m.data(),64);else primary=m;
        }
        Remember(*s);
    }
    if(yaw){
        // Preserve 199C74..199CA9's SSE operation order, using P2's own pitch
        // captured before either transform callback clamps it.
        const __m128 amount=_mm_mul_ss(_mm_set_ss(s->pitch),_mm_load_ss(reinterpret_cast<const float*>(b+0x47544c)));
        const __m128 center=_mm_set_ss(s->yaw);
        const __m128 lo=_mm_sub_ss(_mm_sub_ss(center,_mm_load_ss(reinterpret_cast<const float*>(b+0x475448))),amount);
        const __m128 hi=_mm_add_ss(_mm_add_ss(amount,center),_mm_load_ss(reinterpret_cast<const float*>(b+0x475444)));
        _mm_store_ss(reinterpret_cast<float*>(b+scratch),lo);
        _mm_store_ss(reinterpret_cast<float*>(b+scratch+4),hi);
    }
    if(relay){
        X86Context temporary=c;temporary.esp=scratch+0x300;temporary.df=0;relaying=true;
        kernel::CallGuest(temporary,0x1f90a0,{s->ids[1],event,scratch,size,scratch+0x70});
        if(equip)mounts[1].equipped=true;
    }
    // The two pinned callers build this local payload afresh from PLAYERBONE
    // every frame. Adjust its primary position only after copying/dispatching
    // P2, so neither a moving mount nor repeated updates accumulate an offset.
    if(matrix&&primary_alive)std::memcpy(b+data+0x10,primary.data(),64);
    if(equip&&primary_dispatch)mounts[0].equipped=true;
    if(remove)for(auto& m:mounts)if(m.world==s->world&&m.vehicle==s->vehicle&&m.vehicle_id==s->vehicle_id)m={};
    return primary_dispatch;
}

void TrialCoopTurretResume(const X86Context& c,uint8_t* b){
    auto* s=current;
    if(!s||relaying||s->phase!=TrialCoopTurretScope::Phase::Update||!s->Valid(b)||
       c.esi!=s->vehicle||!b[s->vehicle+0x171]||b[s->vehicle+0x172]||
       c.esp<0x11000||!TrialGuestReadable(0x51c398,4))return;
    const uint32_t services=Word(b,0x51c398);
    if(!TrialGuestReadable(services,12))return;
    const uint32_t scratch=(c.esp-0x400)&~15u;
    if(!TrialGuestReadable(scratch,0x400))return;
    struct Restore {
        uint8_t* b;uint32_t scratch;std::array<uint8_t,0x400> bytes;unsigned csr;bool old;
        ~Restore(){std::memcpy(b+scratch,bytes.data(),bytes.size());_mm_setcsr(csr);relaying=old;}
    } restore{b,scratch,{},_mm_getcsr(),relaying};
    std::memcpy(restore.bytes.data(),b+scratch,restore.bytes.size());
    Put(b,scratch,25);Put(b,scratch+4,1);Put(b,scratch+0x70,s->vehicle_id);
    // A downed actor misses the one-shot mount equip. After its native revive,
    // reuse the same original CC190 event once, after the current mount pose.
    // This never replays vehicle state or changes the other actor's inventory.
    for(unsigned slot=0;slot<2;++slot){
        const auto& m=mounts[slot];
        if(!s->Valid(b)||m.world!=s->world||m.vehicle!=s->vehicle||m.owner!=s->actors[slot]||
           m.owner_id!=s->ids[slot]||!Alive(b,s->actors[slot])||Word(b,s->actors[slot]+0x4fc)==25)continue;
        X86Context temporary=c;temporary.esp=scratch+0x300;temporary.df=0;
        temporary.ecx=Word(b,services+8);relaying=true;
        kernel::CallGuest(temporary,0x1f90a0,{s->ids[slot],Word(b,0x4f9e0c),scratch,8,scratch+0x70});
        mounts[slot].equipped=true;
    }
}

bool TrialCoopTurretKeepDowned(const X86Context& c,const uint8_t* b){
    const auto* s=current;
    if(!s||s->phase!=TrialCoopTurretScope::Phase::Exit||!s->Valid(b)||
       c.ecx!=c.esi||!TrialGuestReadable(c.ebx,4)||Word(b,c.ebx)!=25||
       !TrialGuestReadable(c.esp,8)||Word(b,c.esp)!=8||Word(b,c.esp+4)!=0)return false;
    for(unsigned slot=0;slot<2;++slot)
        if(c.esi==s->actors[slot]&&!Alive(b,c.esi)&&Word(b,c.esi+0x170)==12)return true;
    return false;
}
