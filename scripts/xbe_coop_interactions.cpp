#include "xbe_coop_interactions.h"
#include "xbe_coop.h"
#include "xbe_coop_guest.h"
#include "cpu/x86_ops.h"
#include "kernel/kernel.h"
#include "kernel/memory.h"
#include "xbe_input_memory.h"
#include <array>
#include <atomic>
#include <cmath>
#include <cstring>
#include <xmmintrin.h>

namespace interactions {
uint32_t Word(const uint8_t* b,uint32_t p){uint32_t v;std::memcpy(&v,b+p,4);return v;}
float Real(const uint8_t* b,uint32_t p){float v;std::memcpy(&v,b+p,4);return v;}
struct Record {uint32_t behavior=0,id=0,source=0,actor=0,actor_id=0;int slot=-1;bool paused=false,waiting=false,skip=false,resume_lock=false;};
std::array<Record,64> records{};
std::atomic<unsigned> published[2]{};
uint32_t world=0,players[2]{},ids[2]{};
thread_local TrialCoopInteractionScope* scope=nullptr;
thread_local uint32_t lookup_owner=0;
struct HostRestore {unsigned csr=_mm_getcsr();~HostRestore(){_mm_setcsr(csr);}};
// Original range/property routines use less than 1 KiB; keep both their stack
// and out-parameters off the live translated frame, and restore every byte.
struct Call {
    X86Context c;uint8_t* b;uint32_t low,sp;std::array<uint8_t,4096> saved;
    Call(const X86Context& context,uint8_t* base):c(context),b(base),low(context.esp-4096),sp(context.esp-512){std::memcpy(saved.data(),b+low,saved.size());}
    ~Call(){std::memcpy(b+low,saved.data(),saved.size());}
    uint32_t Run(uint32_t fn,uint32_t self,std::initializer_list<uint32_t> args){c.ecx=self;c.esp=sp;return kernel::CallGuest(c,fn,args);}
};
bool Stack(const X86Context& c){return c.esp>=0x11000&&TrialGuestReadable(c.esp-4096,4096);}
bool Actor(const uint8_t* b,uint32_t p,unsigned slot){
    return TrialGuestReadable(p,0x890)&&Word(b,p)==0x46fbe8&&Word(b,p+0x368)==slot&&
        Word(b,p+0x54)!=0xffffffffu&&!(Word(b,p+0x5c)&1);
}
bool Alive(const uint8_t* b,uint32_t p){const float health=Real(b,p+0x9c);return std::isfinite(health)&&health>0&&Word(b,p+0x170)!=12;}
void Publish(){
    unsigned status[2]{};
    for(const auto& r:records)if(r.behavior&&r.slot>=0&&r.slot<2)status[r.slot]|=(r.skip?1u:0u)|(r.waiting?2u:0u);
    for(unsigned i=0;i<2;++i)published[i].store(status[i],std::memory_order_relaxed);
}
bool Pair(const uint8_t* b){
    if(!b||!trial_coop::Active()||!TrialGuestReadable(0x51c4dc,4)){
        records={};world=0;Publish();return false;
    }
    uint32_t w=Word(b,0x51c4dc),p[2]{TrialCoopPlayer(0),TrialCoopPlayer(1)},id[2];
    if(!TrialGuestReadable(w,0x38)||!Actor(b,p[0],0)||!Actor(b,p[1],1)||p[0]==p[1]){records={};Publish();return false;}
    for(unsigned i=0;i<2;++i)id[i]=Word(b,p[i]+0x54);
    if(id[0]==id[1]){records={};Publish();return false;}
    if(w!=world||p[0]!=players[0]||p[1]!=players[1]||id[0]!=ids[0]||id[1]!=ids[1]){
        records={};world=w;for(unsigned i=0;i<2;++i){players[i]=p[i];ids[i]=id[i];}Publish();
    }
    return true;
}
bool Behavior(const uint8_t* b,uint32_t p){
    if(!TrialGuestReadable(p,0x15c)||Word(b,p)!=0x471060||(Word(b,p+0x5c)&1)||Word(b,p+0x54)==0xffffffffu)return false;
    const uint32_t controller=Word(b,p+0xd0),actor=Word(b,p+0xd4);
    return TrialGuestReadable(controller,0x20)&&TrialGuestReadable(actor,0x15c)&&
        Word(b,controller+0xc)==actor&&Word(b,actor+0x54)!=0xffffffffu&&!(Word(b,actor+0x5c)&1);
}
Record* Find(const uint8_t* b,uint32_t p,bool create=false){
    if(!Behavior(b,p))return nullptr;
    for(auto& r:records)if(r.behavior==p){
        if(r.id==Word(b,p+0x54)&&r.source==Word(b,p+0x50)&&r.actor==Word(b,p+0xd4)&&r.actor_id==Word(b,r.actor+0x54))return &r;
        r={};
    }
    if(create)for(auto& r:records)if(!r.behavior){r.behavior=p;r.id=Word(b,p+0x54);r.source=Word(b,p+0x50);r.actor=Word(b,p+0xd4);r.actor_id=Word(b,r.actor+0x54);return &r;}
    return nullptr;
}
bool Current(const uint8_t* b,const Record& r){return r.slot>=0&&r.slot<2&&Actor(b,players[r.slot],unsigned(r.slot))&&Word(b,players[r.slot]+0x54)==ids[r.slot];}
struct Lookup {uint32_t previous;Lookup(uint32_t p):previous(lookup_owner){lookup_owner=p;}~Lookup(){lookup_owner=previous;}};
bool Position(Call& call,uint32_t behavior,float (&position)[3]){
    const uint32_t actor=Word(call.b,behavior+0xd4),proxy=Word(call.b,actor+0x118);
    uint32_t address=actor+0x40;
    if(proxy){
        if(!TrialGuestReadable(proxy,4)||!TrialGuestReadable(Word(call.b,proxy),0x30))return false;
        address=call.Run(Word(call.b,Word(call.b,proxy)+0x2c),proxy,{call.sp+128});
    }
    if(!TrialGuestReadable(address,12))return false;
    std::memcpy(position,call.b+address,12);for(float v:position)if(!std::isfinite(v))return false;
    return true;
}
bool Together(Call& call,uint32_t behavior){
    const float range=Real(call.b,behavior+0x114);if(!std::isfinite(range))return false;
    // Negative range is the original deliberate remote/radio interaction rule.
    if(range<0)return Alive(call.b,players[0])&&Alive(call.b,players[1]);
    float npc[3];if(!Position(call,behavior,npc))return false;
    for(unsigned i=0;i<2;++i){
        if(!Alive(call.b,players[i]))return false;
        float square[3];
        for(unsigned axis=0;axis<3;++axis){const float v=Real(call.b,players[i]+0x40+axis*4);if(!std::isfinite(v))return false;const float d=v-npc[axis];square[axis]=d*d;}
        const float distance=(square[2]+square[1])+square[0];
        // Same strict authored sphere as 33C4E..33C51. Facing belongs only to
        // the initiator; the companion can look elsewhere and continue moving.
        if(!(range*range>distance))return false;
    }
    return true;
}
void Lock(Call& call,Record& r,bool value){
    if(!Current(call.b,r)||!TrialGuestReadable(0x51c398,4)||!TrialGuestReadable(Word(call.b,0x51c398),12))return;
    const uint32_t events=Word(call.b,Word(call.b,0x51c398)+8);
    if(!TrialGuestReadable(events,4))return;
    call.b[call.sp+128]=value?1:0;std::memcpy(call.b+call.sp+132,&r.id,4);
    // Use the original event95 delivery and BD670 actor-local control byte.
    // No campaign-global input state or companion control state is changed.
    call.Run(0x1f90a0,events,{ids[r.slot],Word(call.b,0x4f9f20),call.sp+128,1,call.sp+132});
    call.b[r.behavior+0x129]=value?1:0;
}
}
using namespace interactions;

TrialCoopInteractionStatus TrialCoopInteractionSnapshot(unsigned slot){const unsigned v=slot<2?published[slot].load(std::memory_order_relaxed):0;return {bool(v&1),bool(v&2)};}
void TrialCoopInteractionReset(){records={};world=0;lookup_owner=0;Publish();}
unsigned TrialCoopInteractionPlayer(const uint8_t* b,uint32_t behavior){
    if(!Pair(b))return 0;const auto* r=Find(b,behavior);return r&&Current(b,*r)?unsigned(r->slot+1):0;
}
void TrialCoopInteractionLookup(X86Context& c,const uint8_t* b){
    if(lookup_owner&&b&&TrialGuestReadable(lookup_owner,0x58)&&Word(b,lookup_owner+0x54)!=0xffffffffu)c.eax=Word(b,lookup_owner+0x54);
}
void TrialCoopInteractionInput(X86Context& c,uint8_t* b){
    const unsigned player=TrialCoopInteractionPlayer(b,c.esi);
    if(scope&&player&&TrialGuestReadable(c.esp,12)){const uint32_t port=player-1;std::memcpy(b+c.esp,&port,4);}
}
TrialCoopInteractionScope::TrialCoopInteractionScope(X86Context& c,uint8_t* b):context_(c),base_(b),previous_(scope){
    previous_lookup_=lookup_owner;lookup_owner=0;scope=this;
    if(Pair(b)&&Behavior(b,c.ecx)&&Stack(c)&&(Find(b,c.ecx)||Word(b,c.ecx+0x158)==0)){behavior_=c.ecx;id_=Word(b,behavior_+0x54);qualified_=true;}
}
TrialCoopInteractionScope::~TrialCoopInteractionScope(){scope=previous_;lookup_owner=previous_lookup_;Publish();}
bool TrialCoopInteractionScope::Update(){
    HostRestore preserve;
    if(!qualified_)return true;
    auto* r=Find(base_,behavior_);if(!r)return true;
    if(Current(base_,*r)){
        lookup_owner=players[r->slot];
        if(!Alive(base_,lookup_owner)){
            HostRestore host;Call call(context_,base_);if(base_[behavior_+0x129])Lock(call,*r,false);
            r->paused=true;r->skip=false;r->waiting=true;return false;
        }
        if(r->paused){HostRestore host;Call call(context_,base_);if(base_[behavior_+0x119]&&Word(base_,behavior_+0x158)>0)Lock(call,*r,true);r->paused=false;}
    }
    return true;
}
bool TrialCoopInteractionScope::Begin(){
    if(!qualified_)return true;
    auto* r=Find(base_,behavior_,true);if(!r)return false;
    HostRestore host;Call call(context_,base_);
    r->waiting=false;
    // A continuing phase-zero attempt is not ownership. Test both actors using
    // the unchanged range/facing leaf and the original authored button rule.
    for(unsigned i=0;i<2;++i){
        if(base_[players[i]+0x202])continue;
        bool busy=false;for(const auto& other:records)if(other.behavior!=behavior_&&other.slot==int(i)&&Behavior(base_,other.behavior)&&Word(base_,other.behavior+0x158)>0&&base_[other.behavior+0x129]){busy=true;break;}
        if(busy)continue;
        {
            Lookup selected(players[i]);
            if(Real(base_,behavior_+0x114)>=0&&!(call.Run(0x33b60,Word(base_,behavior_+0xd0),{Word(base_,behavior_+0x114),Word(base_,behavior_+0x124)})&255))continue;
            if(base_[behavior_+0x118]){
                const uint32_t vt=Word(base_,world);if(!TrialGuestReadable(vt,12))continue;
                call.Run(Word(base_,vt+8),world,{i,0x18,2});
                const double pressed=call.c.st[call.c.top&7];if(!(pressed>0))continue;
            }
        }
        r->slot=int(i);lookup_owner=players[i];
        r->waiting=!Together(call,behavior_);r->skip=false;
        return !r->waiting;
    }
    *r={};lookup_owner=0;return false;
}
bool TrialCoopInteractionScope::Commit(){
    if(!qualified_)return true;auto* r=Find(base_,behavior_);if(!r)return true;if(!Current(base_,*r))return false;
    HostRestore host;Call call(context_,base_);r->waiting=!Together(call,behavior_);
    if(r->waiting){
        r->skip=false;
        // A finished conversation must not pin its owner in place while the
        // partner needs retrieving or reviving. Keep the original phase and
        // script cursor; release only the lock this behavior actually owns.
        if(base_[behavior_+0x129]){
            r->resume_lock=Word(base_,behavior_+0x158)==2;
            Lock(call,*r,false);
        }
    }else if(r->resume_lock){
        // Phase 2 precedes the first script/speech. Resume its authored lock;
        // completed-speech and final-transition waits remain free to move.
        if(Word(base_,behavior_+0x158)==2&&base_[behavior_+0x119])Lock(call,*r,true);
        r->resume_lock=false;
    }
    return !r->waiting;
}
bool TrialCoopInteractionScope::Fades(bool prompt){
    if(!qualified_)return false;auto* r=Find(base_,behavior_);if(!r||!Current(base_,*r))return false;
    if(prompt)r->skip=base_[behavior_+0x119]&&base_[behavior_+0x148]&&!r->waiting&&!r->paused;
    return true;
}
void TrialCoopInteractionClose(X86Context& c,uint8_t* b,uint32_t behavior){
    if(!Pair(b)||!Stack(c)||!TrialGuestReadable(behavior,0x15c))return;
    Record* r=nullptr;for(auto& candidate:records)if(candidate.behavior==behavior&&candidate.id==Word(b,behavior+0x54)&&candidate.source==Word(b,behavior+0x50)){r=&candidate;break;}
    if(!r)return;
    HostRestore host;Call call(c,b);if(b[behavior+0x129]){
        // Original 17C476 clears skip readiness inside its owned-lock branch.
        // We clear that branch's state before avoiding its manager-based unlock.
        b[behavior+0x148]=0;Lock(call,*r,false);
    }
    *r={};Publish();
}
