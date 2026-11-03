#include "xbe_aim_mode.h"
#include "xbe_controls.h"
#include "xbe_coop.h"
#include "xbe_coop_guest.h"
#include "cpu/x86_ops.h"
#include "kernel/memory.h"
#include "xbe_input_memory.h"
#include <atomic>
#include <cmath>
#include <cstring>
#include <xmmintrin.h>

namespace aim_mode {
// One coherent host snapshot: availability, physical cycle key, edge sequence,
// and a cancellation epoch (including an open/close while guest ticks pause).
std::atomic<uint64_t> input[2]{};
constexpr uint64_t edge_mask=0x3fffffffu;
struct Player {
    uint32_t owner=0,id=0,world=0,level=0,weapon=0,epoch=0;
    unsigned mode=0;
    uint64_t edge=0;
    bool blocked=true,held=false;
};
Player players[2];
uint32_t Read(const uint8_t* b,uint32_t at){uint32_t v;std::memcpy(&v,b+at,4);return v;}
void Clear(uint8_t* b,uint32_t owner){const uint32_t zero=0;std::memcpy(b+owner+0x438,&zero,4);}
struct Identity {int slot=-1;uint32_t id=0,world=0,level=0;};
Identity Identify(const uint8_t* b,uint32_t owner){
    if(!b||!TrialGuestReadable(owner,0x890)||Read(b,owner)!=0x46fbe8||
       (Read(b,owner+0x5c)&1)||!TrialGuestReadable(0x51c4dc,4))return {};
    const uint32_t slot=Read(b,owner+0x368),id=Read(b,owner+0x54),world=Read(b,0x51c4dc);
    if(slot>1||id==0xffffffffu||!TrialGuestReadable(world,0x14)||
       (slot&&(!trial_coop::Active()||TrialCoopPlayer(1)!=owner)))return {};
    const uint32_t level=Read(b,world+4);
    if(!TrialGuestReadable(level,0x203c))return {};
    const uint32_t entry=level+0x3c+(id&1023)*8;
    if(Read(b,entry)!=id||Read(b,entry+4)!=owner)return {};
    return {int(slot),id,world,level};
}
bool Available(const uint8_t* b,uint32_t owner,uint32_t world){
    // D25B0 gates BE870; D2D5D bypasses it for authored camera state 13.
    // D2AD8 checks the native pause bit. Retain a live-player check as well.
    const uint32_t state=Read(b,owner+0x170),health=Read(b,owner+0x9c);
    return health>0&&health<0x7f800000u&&state!=3&&state!=4&&state!=7&&state!=8&&
        state!=11&&state!=12&&state!=13&&!(Read(b,world+0x10)&0x800u);
}
Player* Sync(uint8_t* b,uint32_t owner,Identity* result=nullptr,uint64_t* input_snapshot=nullptr){
    const auto identity=Identify(b,owner);if(result)*result=identity;
    if(identity.slot<0)return nullptr;
    auto& p=players[identity.slot];
    const uint64_t host=input[identity.slot].load();
    if(input_snapshot)*input_snapshot=host;
    const unsigned mode=trial_controls::AimMode(unsigned(identity.slot));
    const uint32_t weapon=Read(b,owner+0x4fc),epoch=uint32_t(host>>32);
    const bool same=p.owner==owner&&p.id==identity.id&&p.world==identity.world&&p.level==identity.level;
    const bool reset=!same||p.mode!=mode||p.weapon!=weapon||p.epoch!=epoch;
    if(reset){
        // Never touch an old/reused actor. Only clear this validated live actor.
        if(mode==1||(same&&p.mode==1))Clear(b,owner);
        p={owner,identity.id,identity.world,identity.level,weapon,epoch,mode,(host>>2)&edge_mask,true,false};
    }
    if(mode==1&&(!(host&1)||!Available(b,owner,identity.world))){
        Clear(b,owner);p.blocked=true;p.held=false;p.edge=(host>>2)&edge_mask;
    }
    return &p;
}
}

void TrialAimInput(unsigned slot,bool down,bool allowed){
    using namespace aim_mode;if(slot>1)return;
    uint64_t old=aim_mode::input[slot].load(),next;
    do{
        uint64_t edge=(old>>2)&edge_mask;
        uint32_t epoch=uint32_t(old>>32);
        if(!allowed&&(old&1))++epoch;
        if(allowed&&down&&!(old&2))edge=(edge+1)&edge_mask;
        next=(uint64_t(epoch)<<32)|(edge<<2)|(down?2u:0u)|(allowed?1u:0u);
    }while(!aim_mode::input[slot].compare_exchange_weak(old,next));
}
void TrialAimUpdate(const X86Context& c,uint8_t* b){aim_mode::Sync(b,c.ecx);}
TrialAimModeScope::TrialAimModeScope(const X86Context& c,uint8_t* b):base(b),owner(c.ecx){
    aim_mode::Identity identity;auto* p=aim_mode::Sync(b,owner,&identity);
    if(!p)return;
    slot=identity.slot;id=identity.id;world=identity.world;level=identity.level;hold=p->mode==1;
}
TrialAimModeScope::~TrialAimModeScope(){
    if(!hold||slot<0)return;
    const auto current=aim_mode::Identify(base,owner);
    if(current.slot!=slot||current.id!=id||current.world!=world||current.level!=level)return;
    if(!queried||(emitted&&!cycled)){
        // Reload/weapon eligibility rejected the native transition. A fresh
        // release is required instead of zooming later from a stale hold.
        aim_mode::Clear(base,owner);auto& p=aim_mode::players[slot];p.blocked=true;p.held=false;
    }
}
void TrialAimModeScope::Filter(X86Context& c){
    if(!hold)return;
    queried=true;uint64_t host=0;auto* p=aim_mode::Sync(base,owner,nullptr,&host);if(!p){c.st[c.top&7]=0;return;}
    // Match permission and edge handling to the epoch that Sync validated.
    const uint64_t edge=(host>>2)&aim_mode::edge_mask;
    const unsigned csr=_mm_getcsr();
    const double value=double(c.st[c.top&7]);const bool down=std::isfinite(value)&&value>0;
    _mm_setcsr(csr);
    const bool allowed=(host&1)&&aim_mode::Available(base,owner,world);
    const bool fresh=down&&!p->held,cycle=edge!=p->edge;
    p->held=down;p->edge=edge;
    c.st[c.top&7]=0;
    if(!down){aim_mode::Clear(base,owner);if(allowed)p->blocked=false;return;}
    if(!allowed||p->blocked)return;
    const uint32_t zoom=aim_mode::Read(base,owner+0x438);
    alternate=cycle&&(p->weapon==0x15||p->weapon==0x16)&&(zoom==1||zoom==2);
    emitted=(fresh&&zoom==0)||alternate;
    if(emitted)c.st[c.top&7]=1;
}
void TrialAimModeScope::Cycle(){
    if(!hold||!emitted)return;
    const auto current=aim_mode::Identify(base,owner);
    if(current.slot!=slot||current.id!=id||current.world!=world||current.level!=level)return;
    // BE700 retains all original magnification events. Its 2 -> 0 step becomes
    // 0 -> 1 only for an admitted held-scope cycle, never ordinary Toggle.
    if(alternate&&aim_mode::Read(base,owner+0x438)==2)aim_mode::Clear(base,owner);
    cycled=true;
}
