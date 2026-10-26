// Actual native popup/root constructors, list-node allocation/copy and queue
// publication. Allocator storage and the unrelated HUD branches are boundaries.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cstdio>
#include <cfenv>
#include <stdexcept>
#include <vector>
#include <xmmintrin.h>
#include "kernel/kernel.h"
namespace kernel {static std::array<uint8_t,0x1000000> memory{};}
namespace mem {
bool IsCommitted(uint32_t p){return p<kernel::memory.size();}
static bool stack_live=false,stack_failure=false;static unsigned stack_allocations=0,stack_frees=0;
uint32_t AllocStack(uint32_t size){assert(size==0x10000&&!stack_live);if(stack_failure)return 0;stack_live=true;++stack_allocations;return 0xd10000;}
void FreeStack(uint32_t top){assert(top==0xd10000&&stack_live);stack_live=false;++stack_frees;}
}
#include "../common/xbe_coop_rewards.cpp"
constexpr uint32_t P[2]={0x610000,0x612000},W=0x620000,SP=0xfe0000,FS=0x6f0000,VALUE=0x6b0000;
constexpr uint32_t PROFILE=0x6c0000,STATS=0x6c1000;
constexpr uint32_t CONTROLLER=0x6d0000,VICTIM=0x6d1000,TABLE=0x44d9c0;
uint32_t next_allocation=0x700000,managers[2]{},roots[2]{};
unsigned native_publications=0;
bool route_popup=true;
bool throw_allocation=false,perturb_guest=false,consume_request=false,patched_sounds=true;
std::vector<uint32_t> played_sounds,freed_nodes;
uint8_t* B(){return kernel::memory.data();}
void Put(uint32_t p,uint32_t v){std::memcpy(B()+p,&v,4);}
uint32_t TrialCoopPlayer(unsigned i){return i<2?P[i]:0;}
uint32_t Allocate(uint32_t size){assert(size&&size<0x1000);const auto p=next_allocation;next_allocation+=(size+15)&~15u;assert(next_allocation<0x900000);return p;}
void x86_resume(X86Context&,uint8_t*,const X86Resume&){assert(false);}
X86_FUNC(sub_002D57FB){
    if(throw_allocation){Put(ctx.fs_base,0x12345678);_mm_setcsr(0x5f80);std::fesetround(FE_DOWNWARD);throw std::runtime_error("fixture allocation failure");}
    ctx.eax=Allocate(Read(base,ctx.esp+4));ctx.esp+=4;
}
X86_FUNC(sub_002D5CCD){freed_nodes.push_back(Read(base,ctx.esp+4));ctx.esp+=4;}
X86_FUNC(sub_00011EF0){assert(false);}
X86_FUNC(sub_00012050){assert(false);}
X86_FUNC(sub_002D619E){assert(false);}
X86_FUNC(sub_0004A840){ctx.eax=PROFILE;ctx.esp+=4;}
// ORIGINAL_00036760
X86_FUNC(sub_00036760){__imp__sub_00036760(ctx,base);}
// ORIGINAL_000DDCA0
// COOP_DAMAGE
// ORIGINAL_00036800
// COOP_DAMAGE_END
// ORIGINAL_001FB130
X86_FUNC(sub_001FB130){__imp__sub_001FB130(ctx,base);}
// ORIGINAL_002C3740
X86_FUNC(sub_002C3740){__imp__sub_002C3740(ctx,base);}
// ORIGINAL_001FB150
X86_FUNC(sub_001FB150){__imp__sub_001FB150(ctx,base);}
// ORIGINAL_000773D0
// ORIGINAL_00072B50
// ORIGINAL_00073B30
X86_FUNC(sub_00073B30){__imp__sub_00073B30(ctx,base);}
// ORIGINAL_00077610
X86_FUNC(sub_00077610){__imp__sub_00077610(ctx,base);}
// ORIGINAL_00077E60
X86_FUNC(sub_00077E60){++native_publications;__imp__sub_00077E60(ctx,base);}
namespace kernel {
uint8_t* g_base=memory.data();
// ORIGINAL_CALL_GUEST
}
X86_FUNC(sub_00208980);
// ORIGINAL_OBJECTIVE_SOUND
// COOP_OBJECTIVE_SOUND
// ORIGINAL_OBJECTIVE_SWOOSH
// COOP_OBJECTIVE_SWOOSH
X86_FUNC(sub_00208980){assert(Read(base,ctx.esp+4)==0x4535a8&&!Read(base,ctx.esp+8));ctx.eax=0x600301;ctx.esp+=12;}
void x86_icall(X86Context& c,uint8_t* b,uint32_t target){
    if(target==0x77e60){
        assert(mem::stack_live&&c.esp>0xd00000&&c.esp<0xd10000&&!c.df);
        sub_00077E60(c,b);
        if(perturb_guest){c.df=1;c.st[0]=999;c.xmm[5].u[2]=17;c.fcw=0x27f;c.fsw=3;Put(c.fs_base,0x87654321);_mm_setcsr(0x5f80);std::fesetround(FE_DOWNWARD);}
        return;
    }
    if(target==0x600014){assert(Read(b,c.esp+4)==0x453594&&!Read(b,c.esp+8));c.eax=0x600300;c.esp+=12;return;}
    assert(target==0x60002c);
    const uint32_t bank=Read(b,c.esp+4),event=Read(b,c.esp+8);
    assert(!Read(b,c.esp+12)&&((bank==0x600300&&event>=0x600100&&event<=0x600104)||
        (bank==0x600301&&event==0x4535c0)));
    played_sounds.push_back(event);c.esp+=16;
}
X86_FUNC(sub_00077720){
    // Widget construction/animation is the boundary; execute the original sound
    // branches and queue retirement against the request selected by native update.
    const uint32_t row=Read(base,ctx.esp+4),last=Read(base,ctx.esp+12);
    const bool silent=TrialCoopObjectiveSilent(ctx,base);
    auto sound=ctx;sound.esp-=0x200;sound.ebp=row;
    for(unsigned i=0;i<5;++i)Put(sound.esp+0x60+4*i,0x600100+i);
    B()[sound.esp+0x8c]=static_cast<uint8_t>(last);
    if(consume_request){if(patched_sounds)CoopObjectiveSwoosh(sound,base,silent);else OriginalObjectiveSwoosh(sound,base,silent);}
    else {if(patched_sounds)CoopObjectiveSound(sound,base,silent);else OriginalObjectiveSound(sound,base,silent);}
    assert(sound.esp==ctx.esp-0x200);ctx.eax=consume_request?0:1;ctx.esp+=16;
}
// ORIGINAL_00228AD0
X86_FUNC(sub_00228AD0){__imp__sub_00228AD0(ctx,base);}
// ORIGINAL_000776B0
// COOP_QUEUE_UPDATE
// ORIGINAL_00073060
// COOP_POPUP
X86_FUNC(sub_00073060){if(route_popup)CoopPopup(ctx,base);else __imp__sub_00073060(ctx,base);}
X86_FUNC(sub_0006BA40){ctx.esp+=4;}
X86_FUNC(sub_00262120){std::snprintf(reinterpret_cast<char*>(base+Read(base,ctx.esp+4)),8,"%u/40",Read(base,STATS+8));ctx.esp+=4;}
X86_FUNC(sub_0025CE20){assert(Read(base,ctx.esp+4)==1);ctx.eax=40;ctx.esp+=4;}
X86_FUNC(sub_001F92B0){ctx.eax=Read(base,ctx.esp+4);Put(ctx.eax,Read(base,ctx.esp+8));ctx.esp+=4;}
X86_FUNC(sub_002D6322){std::snprintf(reinterpret_cast<char*>(base+Read(base,ctx.esp+4)),8,"%u",Read(base,ctx.esp+12));ctx.esp+=4;}
// ORIGINAL_HEADSHOT_PUBLICATION
// ORIGINAL_HEADSHOT_ELIGIBILITY
X86Context Context(){
    X86Context c{};c.esp=SP;c.fs_base=FS;c.eax=19;c.ebx=21;c.ecx=23;c.edx=25;c.esi=27;c.edi=29;c.ebp=31;
    c.cf=1;c.zf=0;c.sf=1;c.of=1;c.pf=0;c.xflags=0x202;c.st[0]=1.25;c.top=3;
    c.mxcsr=0x3f80;c.xmm[5].u[2]=91;return c;
}
X86_FUNC(sub_00070D90){
    const uint32_t actor=Read(base,ctx.esp+4),slot=Read(base,actor+0x368);
    assert(slot<2);X86Context t=ctx;t.esp=SP-0x1000;t.ecx=Allocate(0x24);
    __imp__sub_000773D0(t,base);managers[slot]=t.eax;
    t.esp=SP-0x1000;Put(t.esp,0);Put(t.esp+4,actor);Put(t.esp+8,0);Put(t.esp+12,0);Put(t.esp+16,0);
    __imp__sub_00072B50(t,base);roots[slot]=t.eax;ctx.eax=t.eax;ctx.esp+=4;
}
// ORIGINAL_HUD_CREATION
void BuildHud(unsigned slot){
    auto c=Context();c.ecx=P[slot];OriginalHudCreation(c,B());assert(c.esp==SP+4);
    assert(Read(B(),P[slot]+0x300)==roots[slot]&&Read(B(),roots[slot]+4)==P[slot]);
}
void Setup(unsigned hud_mask=3,bool active=true,unsigned first=0){
    kernel::memory.fill(0);trial_coop::Reset();TrialCoopRewardsReset();native_publications=0;next_allocation=0x700000;
    managers[0]=managers[1]=roots[0]=roots[1]=0;assert(!mem::stack_live);mem::stack_failure=false;
    mem::stack_allocations=mem::stack_frees=0;throw_allocation=perturb_guest=consume_request=false;patched_sounds=true;
    played_sounds.clear();freed_nodes.clear();
    trial_coop::SetAvailable(true);trial_coop::Request();Put(0x51c4dc,W);std::memcpy(B()+VALUE,"3/40",5);
    Put(W+0x6c,0x6e0000);Put(0x6e0000,0x6e1000);Put(0x6e1014,0x600014);Put(0x6e102c,0x60002c);
    Put(PROFILE+0xc,STATS);Put(W+0xc,123);
    Put(CONTROLLER,TABLE);Put(TABLE+0x6c,0x371b0);Put(TABLE+0x70,0x37d10);Put(TABLE+0x74,0x36800);
    Put(CONTROLLER+0x88,VICTIM);Put(VICTIM+0x54,200);Put(CONTROLLER+0xd8,0x3f800000);
    B()[CONTROLLER+0x91]=1;
    Put(CONTROLLER+0xb8,100);route_popup=true;
    for(unsigned i=0;i<2;++i){Put(P[i],0x46fbe8);Put(P[i]+0x54,100+i);Put(P[i]+0x368,i);}
    for(unsigned n=0;n<2;++n){const unsigned i=(first+n)%2;if(hud_mask&(1u<<i))BuildHud(i);}
    assert(!trial_coop::Active()&&hud[0].root==roots[0]&&hud[1].root==roots[1]); // requested startup capture
    if(hud_mask==3)assert(managers[0]!=managers[1]&&Read(B(),0x51613c)==managers[1-first]);
    trial_coop::SetActive(active);
}
X86Context Request(uint32_t caller){
    auto c=Context();c.esi=CONTROLLER;Put(SP,caller);
    const uint32_t args[5]={caller==0x37632?3u:caller==0x3768c?1u:2u,0x440000,0x440040,VALUE,1};
    for(unsigned i=0;i<5;++i)Put(SP+4+i*4,args[i]);return c;
}
void Hit(unsigned slot,uint32_t flags=0x40080,uint32_t controller=CONTROLLER){
    auto c=Context();c.ecx=controller;Put(SP,0xdfc4d);
    Put(SP+4,flags);Put(SP+8,0x3f800000);Put(SP+12,5);Put(SP+16,slot<2?Read(B(),P[slot]+0x54):777);
    CoopDamage(c,B());assert(c.esp==SP+20);
}
void Publish(bool patched,uint32_t caller=0x3768c,bool prime=true){
    if(prime)Hit(0);
    auto c=Request(caller);if(patched)CoopPopup(c,B());else __imp__sub_00073060(c,B());assert(c.esp==SP+4);
}
void FinishDamage(){auto c=Context();c.esi=CONTROLLER;CoopDamageEnd(c,B());}
void Award(){
    // Execute the native pending-damage producer/reader, not an invented
    // fixture attacker slot. 38A70 invokes +6C before its +70 death handler.
    auto c=Context();
    c.esp=SP-0x1000;c.ecx=CONTROLLER;Put(c.esp,0x38adb);Put(c.esp+4,SP+0x64);Put(c.esp+8,SP+0x68);
    Put(SP+0x64,0);__imp__sub_00036800(c,B());assert(c.esp==SP-0x1000+12);
    assert(Read(B(),SP+0x64)==0x40080);
    // Feed normalized native damage through the retained player eligibility
    // gate, followed by the original count, goal branches and popup publisher.
    c.esp=SP;c.esi=CONTROLLER;c.ebx=Read(B(),SP+0x64);OriginalHeadshotEligibility(c,B());
    assert(B()[SP+0x12]==1);Put(SP+0x14,1); // collision awarded one eligible target
    OriginalHeadshotPublication(c,B());assert(c.esp==SP);
    FinishDamage();assert(!Read(B(),CONTROLLER+0x9c)&&!Read(B(),CONTROLLER+0xa0));
}
void Headshot(unsigned slot){Hit(slot);Award();}
uint32_t Count(unsigned i){return Read(B(),managers[i]+0xc);}
uint32_t Latest(unsigned i){const auto head=Read(B(),managers[i]+8);return Read(B(),head+4)+8;}
constexpr std::array<std::array<uint32_t,2>,10> objective_sites{{
    {0xe8ae9,0},{0xe8b6c,1},{0xe8ca4,3},{0xe8c58,4},{0x128f75,3},
    {0x128fd5,4},{0x1d66b5,3},{0x1d6715,4},{0x1e31bf,3},{0x1e321f,4}}};
X86Context ObjectiveRequest(unsigned index=0){
    auto c=Context();const auto& site=objective_sites[index];Put(SP,site[0]);
    const uint32_t args[5]={site[1],0x440000+index*0x40,0x441000+index*0x40,0,0xffffffff};
    for(unsigned i=0;i<5;++i)Put(SP+4+i*4,args[i]);return c;
}
void PublishObjective(unsigned index=0,bool patched=true){
    auto c=ObjectiveRequest(index);if(patched)CoopPopup(c,B());else __imp__sub_00073060(c,B());assert(c.esp==SP+4);
}
uint32_t First(unsigned slot){return Read(B(),Read(B(),managers[slot]+8))+8;}
bool IsSilent(unsigned slot,uint32_t row){
    auto c=Context();c.ecx=managers[slot];Put(SP+4,row);const auto before=c;
    const bool result=TrialCoopObjectiveSilent(c,B());assert(!std::memcmp(&c,&before,sizeof c));return result;
}
void UpdateQueue(unsigned slot,bool finish=false){
    auto c=Context();c.ecx=managers[slot];Put(SP+4,0x3f800000);consume_request=finish;
    CoopQueueUpdate(c,B());assert(c.esp==SP+8);
}
void ObjectiveTests(){
    for(unsigned last=0;last<2;++last)for(unsigned index=0;index<objective_sites.size();++index){
        Setup();Put(0x51613c,managers[last]);PublishObjective(index,false);
        assert(Count(last)==1&&!Count(1-last)); // original one-HUD failure
        Setup();Put(0x51613c,managers[last]);PublishObjective(index);
        assert(Count(0)==1&&Count(1)==1&&native_publications==2&&Read(B(),0x51613c)==managers[last]);
        const auto a=Latest(0),b=Latest(1);assert(a!=b);
        assert(Read(B(),a+8)==objective_sites[index][1]&&Read(B(),a+0xc)==0xffffffff);
        assert(!std::memcmp(B()+a+8,B()+b+8,0x11)&&!B()[a+0x18]);
        Put(a+4,0x3f800000);assert(!Read(B(),b+4)); // no shared mutable animation record
        assert(!IsSilent(last,Latest(last))&&IsSilent(1-last,Latest(1-last)));
        UpdateQueue(last);UpdateQueue(1-last);assert(played_sounds.size()==1);
        UpdateQueue(1-last,true);UpdateQueue(last,true);assert(played_sounds.size()==2);
        assert(!Count(0)&&!Count(1)&&freed_nodes.size()==2&&!IsSilent(1-last,Latest(1-last)));
        assert(mem::stack_allocations==mem::stack_frees&&!mem::stack_live);
    }
    Setup();PublishObjective();patched_sounds=false;UpdateQueue(0);UpdateQueue(1);
    assert(played_sounds.size()==2);UpdateQueue(0,true);UpdateQueue(1,true);
    assert(played_sounds.size()==4); // native sound blocks would play both copies

    // An objective may precede either HUD or the second camera's Active gate.
    for(unsigned first=0;first<2;++first)for(bool none:{false,true}){
        Setup(none?0:1u<<first,false,first);PublishObjective(0);PublishObjective(1);
        assert(native_publications==(none?0u:2u));if(none)BuildHud(first);
        assert(Count(first)==2&&!trial_coop::Active());BuildHud(1-first);
        assert(Count(0)==2&&Count(1)==2&&native_publications==4&&!trial_coop::Active());
        for(unsigned slot=0;slot<2;++slot){
            assert(Read(B(),First(slot)+0x10)==0x440000&&Read(B(),Latest(slot)+0x10)==0x440040);
            assert(IsSilent(slot,First(slot))==(slot!=first));BuildHud(slot);assert(Count(slot)==2);
        }
    }
    Setup(3,false);PublishObjective();assert(Count(0)==1&&Count(1)==1);
    // Only whitelisted objective payloads fan out, never personal awards.
    for(uint32_t caller:{0x37632u,0x3768cu,0x376deu,0xe25b6u,0xe261bu,0xe2677u,0x171f1fu,0xe8ae8u}){
        Setup();auto c=ObjectiveRequest();Put(SP,caller);c.ecx=managers[1];auto expected=c;
        TrialCoopObjectiveRecipient(c,B());assert(!std::memcmp(&c,&expected,sizeof c)&&!native_publications);
    }
    for(unsigned field:{0u,3u,4u}){
        Setup();auto c=ObjectiveRequest();Put(SP+4+field*4,field==0?4:field==3?VALUE:0);
        CoopPopup(c,B());assert(!Count(0)&&Count(1)==1&&native_publications==1);
    }
    Setup();trial_coop::Reset();PublishObjective();assert(!Count(0)&&Count(1)==1);
    // A personal popup ahead of the mirrored objective must remain audible;
    // the mirror remains silent when its independent queue reaches it later.
    Setup();Headshot(0);PublishObjective();assert(Count(0)==2&&Count(1)==1);
    assert(!IsSilent(0,First(0)));UpdateQueue(1);UpdateQueue(0);
    assert(played_sounds.size()==2);UpdateQueue(0,true);UpdateQueue(0);
    assert(played_sounds.size()==2&&Count(0)==1);UpdateQueue(0,true);UpdateQueue(1,true);
    assert(played_sounds.size()==3);

    // Retirement prevents a same-address, same-payload future request from
    // inheriting silence. A rebuilt tree or world must also revoke ownership.
    Setup();PublishObjective();const uint32_t old_row=First(0);UpdateQueue(0,true);
    assert(!IsSilent(0,old_row));next_allocation=old_row-8;
    auto native=ObjectiveRequest();Put(0x51613c,managers[0]);__imp__sub_00073060(native,B());
    assert(First(0)==old_row&&!IsSilent(0,old_row));
    for(unsigned stale=0;stale<5;++stale){
        Setup();PublishObjective();const uint32_t row=Latest(0);
        if(stale==0)Put(P[0]+0x54,777);
        if(stale==1)Put(row+0x14,0x442000);
        if(stale==2){Put(P[0]+0x300,0);BuildHud(0);assert(!Count(0));}
        if(stale==3)Put(0x51c4dc,W+0x100);
        if(stale==4)TrialCoopRewardsReset();
        assert(!IsSilent(0,row));
    }
    for(bool reset:{false,true}){
        Setup(1,false);PublishObjective();
        if(reset)TrialCoopRewardsReset();else Put(0x51c4dc,W+0x100);
        BuildHud(1);assert(!Count(1));
    }
    for(bool rebuild:{false,true}){
        Setup();mem::stack_failure=true;PublishObjective(0);PublishObjective(1);
        assert(!Count(0)&&Count(1)==2);
        mem::stack_failure=false;if(rebuild)Put(P[0]+0x300,0);BuildHud(0);
        assert(Count(0)==(rebuild?0u:2u)&&Count(1)==2);
        if(!rebuild)assert(Read(B(),First(0)+0x10)==0x440000&&
            Read(B(),Latest(0)+0x10)==0x440040&&IsSilent(0,First(0)));
        assert(objectives.empty());
    }
    // Additional native queue calls cannot leak their guest state, SEH link,
    // borrowed parent stack or host floating-point environment into the caller.
    for(bool throwing:{false,true}){
        Setup();auto c=ObjectiveRequest();c.ecx=managers[1];c.df=1;
        const auto expected=c;Put(FS,0x76543210);std::array<uint8_t,0x1200> parent_stack{};
        std::memcpy(parent_stack.data(),B()+SP-0x1000,parent_stack.size());
        const unsigned saved_csr=_mm_getcsr();const int saved_round=std::fegetround();
        std::fesetround(FE_UPWARD);_mm_setcsr(_mm_getcsr()|0x8000);const unsigned wanted_csr=_mm_getcsr();
        perturb_guest=!throwing;throw_allocation=throwing;bool caught=false;
        try{TrialCoopObjectiveRecipient(c,B());}catch(const std::runtime_error&){caught=true;}
        assert(caught==throwing&&!std::memcmp(&c,&expected,sizeof c));
        assert(!std::memcmp(parent_stack.data(),B()+SP-0x1000,parent_stack.size()));
        assert(Read(B(),FS)==0x76543210&&_mm_getcsr()==wanted_csr&&std::fegetround()==FE_UPWARD);
        assert(!mem::stack_live&&mem::stack_allocations==mem::stack_frees);
        throw_allocation=perturb_guest=false;std::fesetround(saved_round);_mm_setcsr(saved_csr);
    }
    puts("co-op objectives: all ten native callers, both HUD startup orders, deferred queue order, independent records, silent mirror sound branches, personal exclusions, native retirement/reuse, reset/rebuilt owners and guest-call preservation passed (offline; live popup acceptance pending)");
}
int main(){
    ObjectiveTests();
    for(unsigned last=0;last<2;++last)for(unsigned actor=0;actor<2;++actor)for(uint32_t initial:{0u,39u,40u}){
        Setup();Put(0x51613c,managers[last]);Put(STATS+8,initial);route_popup=false;Headshot(actor);
        assert(Count(last)==1&&!Count(1-last)&&native_publications==1); // latest-HUD-only defect
        Setup();Put(0x51613c,managers[last]);Put(STATS+8,initial);Headshot(actor);
        assert(Count(actor)==1&&!Count(1-actor)&&native_publications==1);
        assert(Read(B(),STATS+8)==initial+1&&Read(B(),STATS+0x40)==1);
        assert(Read(B(),0x51613c)==managers[last]); // shared singleton never changed
        Put(W+0xc,124);Headshot(1-actor);
        assert(Count(0)==1&&Count(1)==1&&native_publications==2);
        assert(Read(B(),STATS+8)==initial+2&&Read(B(),STATS+0x40)==2);
    }
    for(uint32_t callback:{0x371b0u,0x169670u,0x19dfc0u}){
        Setup();Put(TABLE+0x6c,callback);Publish(true);assert(Count(0)==1&&!Count(1));
    }
    for(unsigned actor=0;actor<2;++actor)for(unsigned sequence=0;sequence<4;++sequence){
        Setup();
        if(sequence==0){Hit(actor);Hit(1-actor,0x40040);} // later partner default body region (DFCA8/DFD4A)
        if(sequence==1){Hit(1-actor,0x40040);Hit(actor);} // body shot before headshot
        if(sequence==2){Hit(actor);Hit(1-actor);} // first qualifying headshot owns shared award
        if(sequence==3){Hit(actor);Hit(2,0x02);} // later environmental damage
        Award();assert(Count(actor)==1&&!Count(1-actor)&&native_publications==1);
        assert(Read(B(),STATS+8)==1&&Read(B(),STATS+0x40)==1);
        Put(W+0xc,124);Headshot(1-actor);assert(Count(0)==1&&Count(1)==1&&Read(B(),STATS+8)==2);
    }
    Setup();Hit(0);FinishDamage();Publish(true,0x3768c,false);assert(!native_publications);
    // Removed/reused victims and canceled records cannot exhaust the bounded
    // table when their normal controller update never reaches cycle cleanup.
    for(unsigned retired=0;retired<3;++retired){
        Setup();
        for(unsigned i=0;i<200;++i){
            const uint32_t controller=0xa00000+i*0x200,victim=0xb00000+i*0x200;
            std::memcpy(B()+controller,B()+CONTROLLER,0xbc);std::memcpy(B()+victim,B()+VICTIM,0x60);
            Put(controller+0x88,victim);Put(victim+0x54,300+i);Hit(0,0x40080,controller);
            if(retired==0)Put(controller+0x9c,0);
            if(retired==1)Put(victim+0x5c,1);
            if(retired==2)Put(victim+0x54,1000+i);
        }
        Hit(0);unsigned live=0;for(const auto& d:damage)live+=d.controller!=0;assert(live==1);
        Publish(true,0x3768c,false);assert(Count(0)==1&&!Count(1));
    }
    Setup();
    for(unsigned i=0;i<128;++i){
        const uint32_t controller=0xa00000+i*0x200,victim=0xb00000+i*0x200;
        std::memcpy(B()+controller,B()+CONTROLLER,0xbc);std::memcpy(B()+victim,B()+VICTIM,0x60);
        Put(controller+0x88,victim);Put(victim+0x54,300+i);Hit(i%2,0x40080,controller);
    }
    Hit(0);Publish(true,0x3768c,false);assert(!native_publications); // live ownership never evicted
    for(unsigned i=0;i<128;++i)Put(0xb00000+i*0x200+0x5c,1);
    Hit(1);Publish(true,0x3768c,false);assert(!Count(0)&&Count(1)==1);
    Setup();Hit(0);auto c=Request(0x3768c);c.df=1;c.ecx=managers[1];auto expected=c;expected.ecx=managers[0];
    const auto csr=_mm_getcsr();_mm_setcsr(0x3f80);TrialCoopHeadshotRecipient(c,B());
    assert(!std::memcmp(&c,&expected,sizeof c)&&_mm_getcsr()==0x3f80);_mm_setcsr(csr);
    assert(!Count(0)&&!Count(1)&&Read(B(),0x51613c)==managers[1]);
    // Consumed events cannot republish to either player without a fresh hit.
    Publish(true,0x3768c,false);assert(!native_publications);
    for(unsigned invalid=0;invalid<13;++invalid){
        Setup();Hit(0);
        if(invalid==0)damage={};
        if(invalid==1)Put(P[0]+0x300,0);
        if(invalid==2)Put(P[0]+0x54,777);
        if(invalid==3)Put(P[0]+0x5c,1);
        if(invalid==4)Put(managers[0],0x45356d);
        if(invalid==5)Put(roots[0]+4,P[1]);
        if(invalid==6)TrialCoopRewardsReset();
        if(invalid==7)Put(0x51c4dc,W+0x100);
        if(invalid==8)Put(TABLE+0x6c,0x11030);
        if(invalid==9)Put(roots[0],0x4531a1);
        if(invalid==10)Put(CONTROLLER+0x88,0);
        if(invalid==11)Put(VICTIM+0x54,0xffffffff);
        if(invalid==12)Put(CONTROLLER,0);
        auto q=Request(0x3768c);q.ecx=managers[1];auto wanted=q;wanted.ecx=0;TrialCoopHeadshotRecipient(q,B());
        assert(!std::memcmp(&q,&wanted,sizeof q)&&!Count(0)&&!Count(1));
        Publish(true,0x3768c,false);assert(!Count(0)&&!Count(1)&&!native_publications);
    }
    Setup();Publish(true,0xe25b6);assert(!Count(0)&&Count(1)==1); // other notification callers unchanged
    Setup();trial_coop::SetActive(false);Publish(true);assert(!Count(0)&&Count(1)==1);
    Setup();TrialCoopRewardsReset();Publish(true);assert(!Count(0)&&!Count(1));FinishDamage();
    for(unsigned i=0;i<2;++i){Put(P[i]+0x300,0);BuildHud(i);}Publish(true);assert(Count(0)==1&&!Count(1));
    for(unsigned stale=0;stale<3;++stale){
        Setup();if(stale==0)Put(P[1]+0x300,0);if(stale==1)Put(P[1]+0x5c,1);if(stale==2)hud[1]={};
        assert(TrialCoopRewardManager(B(),P[0])==managers[0]&&!TrialCoopRewardManager(B(),P[1]));
        Publish(true);assert(Count(0)==1&&!Count(1));
    }
    Setup();hud[1].manager=managers[0];assert(!TrialCoopRewardManager(B(),P[0]));
    Setup();assert(!TrialCoopRewardManager(B(),VICTIM));
    puts("co-op rewards: native damage attribution, mixed-hit ownership, cycle cleanup, HUD construction, shared count and all three native headshot popup branches route once to the causing player; both singleton orders, unrelated/single-player callers, stale/rebuilt owners and ECX-only preservation passed (offline; live popup acceptance pending)");
}
