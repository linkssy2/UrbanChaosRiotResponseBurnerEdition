#include "xbe_coop_rewards.h"
#include "xbe_coop.h"
#include "xbe_coop_guest.h"
#include "kernel/memory.h"
#include "kernel/kernel.h"
#include "xbe_input_memory.h"
#include "cpu/x86_ops.h"
#include <cstring>
#include <array>
#include <algorithm>
#include <cfenv>
#include <vector>
#include <xmmintrin.h>

namespace {
uint32_t Read(const uint8_t* b,uint32_t p){uint32_t v;std::memcpy(&v,b+p,4);return v;}
struct Hud {uint32_t world=0,actor=0,id=0,root=0,manager=0;uint64_t generation=0;};
Hud hud[2];
uint64_t hud_generation=0;
struct ObjectiveTarget {uint32_t actor=0,id=0;uint64_t generation=0;};
struct Objective {
    uint32_t world=0;std::array<uint32_t,5> args{};
    ObjectiveTarget target[2];unsigned remaining=3;bool audible=false;
};
struct ObjectiveReplica {Hud owner;uint32_t row;unsigned slot;std::array<uint8_t,24> payload;};
uint32_t objective_world=0;
std::vector<Objective> objectives;
std::vector<ObjectiveReplica> objective_replicas;
struct Damage {uint32_t world=0,controller=0,victim=0,id=0,actor=0,actor_id=0,root=0;};
// shortcut: Keep at most 128 simultaneous pending victims. Overflow omits only
// the personal popup; native scoring proceeds and live records are not evicted.
std::array<Damage,128> damage;
bool Actor(const uint8_t* b,uint32_t actor,unsigned slot){
    return TrialGuestReadable(actor,0x890)&&Read(b,actor)==0x46fbe8&&
        Read(b,actor+0x368)==slot&&!(Read(b,actor+0x5c)&1)&&Read(b,actor+0x54)!=0xffffffff;
}
bool Manager(const uint8_t* b,uint32_t manager){
    if(!TrialGuestReadable(manager,0x24)||Read(b,manager)!=0x45356c)return false;
    const uint32_t head=Read(b,manager+8);
    return TrialGuestReadable(head,8)&&TrialGuestReadable(Read(b,head),8)&&
        TrialGuestReadable(Read(b,head+4),8)&&Read(b,manager+0xc)<0x7ffffff;
}
bool Owned(const uint8_t* b,unsigned slot){
    const auto& h=hud[slot];
    return h.world==Read(b,0x51c4dc)&&TrialGuestReadable(h.world,8)&&
        h.actor==TrialCoopPlayer(slot)&&Actor(b,h.actor,slot)&&Read(b,h.actor+0x54)==h.id&&
        Read(b,h.actor+0x300)==h.root&&TrialGuestReadable(h.root,0x18)&&
        Read(b,h.root)==0x4531a0&&Read(b,h.root+4)==h.actor&&Manager(b,h.manager);
}
bool SameHud(const Hud& a,const Hud& b){
    return a.world==b.world&&a.actor==b.actor&&a.id==b.id&&a.root==b.root&&
        a.manager==b.manager&&a.generation==b.generation;
}
uint32_t ObjectiveWorld(const uint8_t* b){
    const uint32_t world=b&&TrialGuestReadable(0x51c4dc,4)?Read(b,0x51c4dc):0;
    const uint32_t live=TrialGuestReadable(world,8)?world:0;
    if(objective_world!=live){objectives.clear();objective_replicas.clear();objective_world=live;}
    return live;
}
bool ObjectiveOwned(const uint8_t* b,unsigned slot){
    if(!Owned(b,slot))return false;
    const auto& h=hud[slot];const auto& other=hud[1-slot];
    return !Owned(b,1-slot)||(h.actor!=other.actor&&h.id!=other.id&&h.root!=other.root&&h.manager!=other.manager);
}
bool ObjectiveTargetLive(const uint8_t* b,const ObjectiveTarget& target,unsigned slot){
    return (!target.actor||(target.actor==TrialCoopPlayer(slot)&&Actor(b,target.actor,slot)&&
        Read(b,target.actor+0x54)==target.id))&&(!target.generation||target.generation==hud[slot].generation);
}
// Only the native authored-objective sites: added/updated/complete/failed and
// the three special-objective controllers. Personal reward types overlap.
bool ObjectiveRequest(const uint8_t* b,uint32_t stack){
    if(!TrialGuestReadable(stack,24)||Read(b,stack+16)||Read(b,stack+20)!=0xffffffff)return false;
    const uint32_t pc=Read(b,stack),type=Read(b,stack+4);
    switch(pc){
    case 0xe8ae9:return type==0;
    case 0xe8b6c:return type==1;
    case 0xe8ca4:case 0x128f75:case 0x1d66b5:case 0x1e31bf:return type==3;
    case 0xe8c58:case 0x128fd5:case 0x1d6715:case 0x1e321f:return type==4;
    default:return false;
    }
}
bool InsertObjective(const X86Context& c,uint8_t* b,unsigned slot,const Objective& event,bool silent){
    if(!ObjectiveOwned(b,slot)||!TrialGuestReadable(c.fs_base,4))return false;
    const auto owner=hud[slot];const uint32_t head=Read(b,owner.manager+8),count=Read(b,owner.manager+0xc);
    // Reserve before committing a native row so marker allocation cannot fail
    // after a successful silent insertion.
    if(silent)objective_replicas.reserve(objective_replicas.size()+1);
    const uint32_t stack=mem::AllocStack(0x10000);if(!stack)return false;
    struct Restore {
        uint32_t stack;uint8_t* base;uint32_t fs,seh;unsigned csr=_mm_getcsr();std::fenv_t fp{};
        Restore(uint32_t s,uint8_t* b,uint32_t f):stack(s),base(b),fs(f),seh(Read(b,f)){std::fegetenv(&fp);}
        ~Restore(){std::memcpy(base+fs,&seh,4);std::fesetenv(&fp);_mm_setcsr(csr);mem::FreeStack(stack);}
    } restore(stack,b,c.fs_base);
    if(stack<0x10000||!TrialGuestReadable(stack-0x10000,0x10000))return false;
    // Native allocation/SEH runs on a separate guest stack. The input context,
    // its entire borrowed stack, FP state and FS chain survive normal/unwind.
    X86Context call=c;call.esp=stack-16;call.ecx=owner.manager;call.df=0;_mm_setcsr(0x1f80);
    kernel::CallGuest(call,0x77e60,event.args.data(),event.args.size());
    if(!SameHud(owner,hud[slot])||!ObjectiveOwned(b,slot)||Read(b,owner.manager+8)!=head||
       Read(b,owner.manager+0xc)!=count+1)return false;
    const uint32_t node=Read(b,head+4);
    if(node==head||!TrialGuestReadable(node,0x28)||Read(b,node)!=head||
       !TrialGuestReadable(Read(b,node+4),4)||Read(b,Read(b,node+4))!=node)return false;
    if(silent){
        ObjectiveReplica replica{owner,node+8,slot,{}};
        std::memcpy(replica.payload.data(),b+replica.row+8,replica.payload.size());
        objective_replicas.push_back(replica);
    }
    return true;
}
bool Controller(const uint8_t* b,uint32_t controller){
    if(!TrialGuestReadable(controller,0xbc))return false;
    const uint32_t table=Read(b,controller),victim=Read(b,controller+0x88);
    if(!TrialGuestReadable(table,0x78)||!TrialGuestReadable(victim,0x60)||
       Read(b,victim+0x54)==0xffffffff||(Read(b,victim+0x5c)&1)||
       (Read(b,controller+0x5c)&1))return false;
    const uint32_t callback=Read(b,table+0x6c);
    return callback==0x371b0||callback==0x169670||callback==0x19dfc0;
}
bool Pending(const uint8_t* b,const Damage& d){
    return d.controller&&d.world==Read(b,0x51c4dc)&&Controller(b,d.controller)&&
        Read(b,d.controller+0x88)==d.victim&&Read(b,d.victim+0x54)==d.id&&
        (Read(b,d.controller+0x9c)&0x40080)==0x40080;
}
}
void TrialCoopRewardsReset(){hud[0]={};hud[1]={};damage={};objectives.clear();objective_replicas.clear();objective_world=0;}
void TrialCoopRewardHudCreated(const X86Context& c,const uint8_t* b){
    if(!b||!trial_coop::Requested())return;
    ObjectiveWorld(b);
    for(unsigned slot=0;slot<2;++slot)if(c.esi==TrialCoopPlayer(slot)&&Actor(b,c.esi,slot)){
        objective_replicas.erase(std::remove_if(objective_replicas.begin(),objective_replicas.end(),
            [slot](const ObjectiveReplica& r){return r.slot==slot;}),objective_replicas.end());
        // Startup has no prior generation and retains its pending notices.
        // Replacing an already captured tree retires notices bound to that tree.
        for(auto& e:objectives)if(e.target[slot].generation)e.remaining&=~(1u<<slot);
        hud[slot]={};
        const uint32_t world=Read(b,0x51c4dc),manager=Read(b,0x51613c);
        // 70D90 has just returned this owner's fresh tree. 773D0 published the
        // manager while building that same tree; 172396 assigns its root next.
        if(!TrialGuestReadable(world,8)||!TrialGuestReadable(c.eax,0x18)||Read(b,c.eax)!=0x4531a0||
           Read(b,c.eax+4)!=c.esi||!Manager(b,manager))return;
        hud[slot]={world,c.esi,Read(b,c.esi+0x54),c.eax,manager,++hud_generation};return;
    }
}
uint32_t TrialCoopRewardManager(const uint8_t* b,uint32_t actor){
    if(!b||!trial_coop::Active())return 0;
    for(unsigned slot=0;slot<2;++slot)if(hud[slot].actor==actor&&Owned(b,slot)){
        const auto& h=hud[slot];const auto& other=hud[1-slot];
        if(Owned(b,1-slot)&&(h.actor==other.actor||h.id==other.id||
           h.root==other.root||h.manager==other.manager))return 0;
        return h.manager;
    }
    return 0;
}
void TrialCoopHeadshotDamage(const X86Context& c,const uint8_t* b){
    if(!b||!trial_coop::Active()||!TrialGuestReadable(c.esp,8)||!Controller(b,c.ecx))return;
    const uint32_t flags=Read(b,c.esp+4);
    if((flags&0x1f0fe)!=0x80||!(flags&0x40000))return;
    const uint32_t world=Read(b,0x51c4dc),victim=Read(b,c.ecx+0x88),id=Read(b,victim+0x54);
    Damage* available=nullptr;
    for(auto& d:damage){
        if(d.controller&&!Pending(b,d))d={};
        if(d.controller==c.ecx){
            if(d.world==world&&d.victim==victim&&d.id==id)return;
            d={};available=&d;
        }else if(!d.controller&&!available)available=&d;
    }
    if(!available)return; // Never evict another victim's pending attribution.
    *available={world,c.ecx,victim,id};
    // DDCA0 is about to store EAX as the latest attacker. Its pending flags
    // combine hits, so preserve the first qualifying headshot independently:
    // a subsequent body/environment hit must not steal that same award.
    for(unsigned slot=0;slot<2;++slot){
        const uint32_t actor=TrialCoopPlayer(slot);
        if(Actor(b,actor,slot)&&Read(b,actor+0x54)==c.eax&&TrialCoopRewardManager(b,actor)){
            available->actor=actor;available->actor_id=c.eax;available->root=Read(b,actor+0x300);return;
        }
    }
}
void TrialCoopHeadshotCycleFinished(const X86Context& c,const uint8_t*){
    if(!trial_coop::Active())return;
    for(auto& d:damage)if(d.controller==c.esi){d={};return;}
}
void TrialCoopHeadshotRecipient(X86Context& c,const uint8_t* b){
    if(!b||!trial_coop::Active()||!TrialGuestReadable(c.esp,4))return;
    const uint32_t caller=Read(b,c.esp);
    if(caller!=0x37632&&caller!=0x3768c&&caller!=0x376de)return;
    // 371B0 retains the victim controller in ESI. Use the owner captured at
    // DDCA0's qualifying headshot, not B8 (the latest hit of any kind).
    // Invalid ownership skips this one notification, never chooses the most
    // recently constructed HUD. Native shared scoring already ran once.
    c.ecx=0;
    if(!Controller(b,c.esi))return;
    for(auto& d:damage)if(d.controller==c.esi){
        const auto pending=d;d={}; // One original headshot publication per cycle.
        if(pending.world!=Read(b,0x51c4dc)||pending.victim!=Read(b,c.esi+0x88)||
           pending.id!=Read(b,pending.victim+0x54)||!TrialGuestReadable(pending.actor,0x304)||
           pending.actor_id!=Read(b,pending.actor+0x54)||pending.root!=Read(b,pending.actor+0x300))return;
        c.ecx=TrialCoopRewardManager(b,pending.actor);return;
    }
    // 73060's original null test and 77E60 queue insertion follow this hook.
    // No singleton, arguments, counters, sound or guest call is duplicated.
}
void TrialCoopObjectiveFlush(const X86Context& c,uint8_t* b){
    if(objectives.empty())return; // This seam also runs on every ordinary HUD tick.
    if(!b||!trial_coop::Requested())return;
    const uint32_t world=ObjectiveWorld(b);if(!world)return;
    unsigned blocked=0;
    for(auto& event:objectives){
        if(event.world!=world){event.remaining=0;continue;}
        for(unsigned slot=0;slot<2;++slot){
            const unsigned bit=1u<<slot;if(!(event.remaining&bit))continue;
            if(!ObjectiveTargetLive(b,event.target[slot],slot)){event.remaining&=~bit;continue;}
            if(blocked&bit)continue;
            if(!ObjectiveOwned(b,slot)||!InsertObjective(c,b,slot,event,event.audible)){blocked|=bit;continue;}
            event.remaining&=~bit;event.audible=true;
        }
    }
    objectives.erase(std::remove_if(objectives.begin(),objectives.end(),
        [](const Objective& event){return !event.remaining;}),objectives.end());
}
void TrialCoopObjectiveRecipient(X86Context& c,uint8_t* b){
    if(!b||!trial_coop::Requested()||!ObjectiveRequest(b,c.esp))return;
    const uint32_t world=ObjectiveWorld(b);if(!world){c.ecx=0;return;}
    TrialCoopObjectiveFlush(c,b);
    Objective event;event.world=world;std::memcpy(event.args.data(),b+c.esp+4,20);
    unsigned blocked=0;for(const auto& prior:objectives)blocked|=prior.remaining;
    bool ready[2]{};
    for(unsigned slot=0;slot<2;++slot){
        const uint32_t actor=TrialCoopPlayer(slot);
        if(Actor(b,actor,slot))event.target[slot]={actor,Read(b,actor+0x54),0};
        if(hud[slot].world==world)event.target[slot].generation=hud[slot].generation;
        ready[slot]=!(blocked&(1u<<slot))&&ObjectiveOwned(b,slot);
    }
    // Keep the original singleton recipient audible when it is a live owner.
    // If it has disappeared, the first live owner takes the original call.
    int original=-1;
    for(unsigned slot=0;slot<2;++slot)if(ready[slot]&&(original<0||c.ecx==hud[slot].manager))original=int(slot);
    objectives.reserve(objectives.size()+1);
    if(original>=0){
        event.remaining&=~(1u<<original);event.audible=true;
        const unsigned other=1u-unsigned(original);
        if(ready[other]&&InsertObjective(c,b,other,event,true))event.remaining&=~(1u<<other);
        c.ecx=hud[original].manager;
    }else c.ecx=0;
    if(event.remaining)objectives.push_back(event);
    // The original 73060 still performs its single native publication. The
    // producer's objective state/scoring and all five arguments are untouched.
}
bool TrialCoopObjectiveSilent(const X86Context& c,const uint8_t* b){
    if(objective_replicas.empty()||!b||!trial_coop::Requested()||!TrialGuestReadable(c.esp,8))return false;
    if(!ObjectiveWorld(b))return false;
    const uint32_t row=Read(b,c.esp+4);
    for(auto it=objective_replicas.begin();it!=objective_replicas.end();++it){
        if(it->row!=row||it->owner.manager!=c.ecx)continue;
        const bool owner=SameHud(it->owner,hud[it->slot])&&ObjectiveOwned(b,it->slot);
        const uint32_t head=owner?Read(b,c.ecx+8):0;
        if(owner&&row>=8&&TrialGuestReadable(row-8,0x28)&&Read(b,head)==row-8&&Read(b,row-4)==head&&
           TrialGuestReadable(Read(b,row-8),8)&&Read(b,Read(b,row-8)+4)==row-8&&
           !std::memcmp(it->payload.data(),b+row+8,it->payload.size()))return true;
        // A removed/reused row or owner must never suppress a later notice.
        objective_replicas.erase(it);return false;
    }
    return false;
}
void TrialCoopObjectiveRetire(const X86Context& c,const uint8_t*){
    if(objective_replicas.empty())return;
    objective_replicas.erase(std::remove_if(objective_replicas.begin(),objective_replicas.end(),
        [&c](const ObjectiveReplica& r){return r.owner.manager==c.esi&&uint64_t(c.eax)+8==r.row;}),objective_replicas.end());
}
