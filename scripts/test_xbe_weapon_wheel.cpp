// Actual retail inventory/eligibility/consumer bodies, production host state and
// CallGuest adapter. Downstream HUD/animation/profile property sinks are explicit.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <thread>
#include <vector>
#include "kernel/kernel.h"
namespace kernel {static std::array<uint8_t,0x800000> memory{};uint8_t* g_base=memory.data();}
static bool mapped=true;
static bool conversation_locked=false;
bool TrialCoopInteractionWeaponsBlocked(const uint8_t*,uint32_t){return conversation_locked;}
namespace mem {bool IsCommitted(uint32_t a){return mapped&&a>=0x10000&&a<kernel::memory.size();}}
static constexpr uint32_t P=0x600000,Q=0x602000,W=0x610000,L=0x620000,I=0x630000,O=0x640000,R=0x650000,S=0x700000;
uint32_t TrialCoopPlayer(unsigned slot){return slot<2?(slot?Q:P):0;}
#include "../common/xbe_weapon_wheel.cpp"
using namespace kernel;
using namespace trial_weapon_wheel;
static unsigned publications=0,transitions=0,shield=0,eligibility_calls=0;
static unsigned query_action=0;
static bool fail_publish=false,native_press=false;
static unsigned native_category=0;
[[noreturn]] void x86_trap(X86Context&,uint32_t,const char*){std::abort();}
X86_FUNC(sub_000442D0){ctx.eax=0x4e55d0+R32(ctx.esp+4)*0xf0;ctx.esp+=4;}
X86_FUNC(sub_00044160){ctx.eax=R32(ctx.ecx+0x28);ctx.esp+=4;}
X86_FUNC(sub_00044210){ctx.eax=R32(ctx.ecx+0x24);ctx.esp+=4;}
X86_FUNC(sub_000421B0){
    assert(ctx.ecx==P+0x498||ctx.ecx==Q+0x498);++publications;
    // A real callback can poll input/UI without recursive locking.
    (void)GetSnapshot(ctx.ecx==Q+0x498);(void)Blocked(0);
    if(fail_publish)throw std::runtime_error("guest unwind");ctx.esp+=4;
}
X86_FUNC(sub_000BFE00){assert((ctx.ecx==P||ctx.ecx==Q)&&R32(ctx.esp+4)==8&&R32(ctx.esp+8)==0);++transitions;ctx.esp+=12;}
X86_FUNC(sub_000BC580){ctx.eax=1;ctx.esp+=4;}
X86_FUNC(sub_000BE210){ctx.eax=0x4e55d0+R32(ctx.ecx+0x4fc)*0xf0;ctx.esp+=4;}
X86_FUNC(sub_000D23E0){++transitions;ctx.esp+=4;}
// NATIVE_WEAPON_CONTRACTS
void x86_icall(X86Context& c,uint8_t* b,uint32_t target){
    switch(target){
    case 0x42da0:{
        ++eligibility_calls;const unsigned action=query_action;query_action=0;
        if(action){
            Cancel(0);
            if(action==2){Input(0,false,true,false,0,0,Kind::MeleeThrowables);Input(0,true,true,false,0,0,Kind::MeleeThrowables);}
        }
        sub_00042DA0(c,b);return;
    }
    case 0x42230:sub_00042230(c,b);return;
    case 0xbc560:++shield;sub_000BC560(c,b);return;
    case 0xf0001:{const auto action=R32(c.esp+8);assert(action>=4&&action<=6);x87_push(c,native_press&&action==native_category+4?1.:0.);c.esp+=16;return;}
    default:assert(false);
    }
}
namespace kernel {
// PRODUCTION_CALL_GUEST
}
static X86Context Context(uint32_t owner=P){X86Context c{};c.ecx=owner;c.esi=owner;c.esp=S;c.ebx=17;c.edi=23;c.ebp=29;W32(S,0x123456);return c;}
static uint32_t ItemAt(unsigned slot,unsigned id){return I+slot*0x2000+id*0x5c;}
static void Reset(){
    memory.fill(0);mapped=true;conversation_locked=false;publications=transitions=shield=eligibility_calls=query_action=0;fail_publish=native_press=false;
    trial_coop::Reset();trial_coop::SetActive(true);
    for(unsigned slot=0;slot<2;++slot){
        players[slot]={};const uint32_t p=slot?Q:P,id=100+slot,inventory=p+0x498;
        W32(p,0x46fbe8);W32(p+0x54,id);W32(p+0x368,slot);G<float>(p+0x9c)=100;W32(p+0x170,4);
        W32(L+0x3c+id*8,id);W32(L+0x40+id*8,p);W32(inventory,I+slot*0x2000);
        W32(inventory+0x64,17);W32(inventory+0x68,17);W32(inventory+0x6c,17);
        W32(inventory+0x70,17);W32(inventory+0x74,8);W32(inventory+0x78,1);
        const uint32_t order=O+slot*0x100;
        W32(inventory+0x80,order);W32(inventory+0x84,order+16);
        W32(inventory+0x90,order+16);W32(inventory+0x94,order+24);
        W32(inventory+0xa0,order+24);W32(inventory+0xa4,order+32);
        const uint32_t ids[]={17,2,5,10,8,11,1,9};
        for(unsigned n=0;n<8;++n){const auto id_=ids[n],item=ItemAt(slot,id_);W32(order+n*4,id_);G<uint8_t>(item)=1;W32(item+4,id_);W32(item+0xc,3);W32(item+0x10,R+slot*0x100+id_*4);}
    }
    for(unsigned id=0;id<27;++id){
        const auto d=0x4e55d0+id*0xf0;W32(d,id);
        W32(d+8,(id==8||(id>=11&&id<=14)||id==26)?1:(id==0||id==1||id==9||id==15)?2:0);
        W32(d+0x24,30);W32(d+0x28,1);
    }
    W32(0x51c4dc,W);W32(W+4,L);W32(W,0x660000);W32(0x660008,0xf0001);G<float>(0x44c174)=0;
    for(unsigned slot=0;slot<2;++slot){auto c=Context(slot?Q:P);GuestUpdate(c,g_base);Input(slot,false,true,true,0,0);}
}
static void Open(unsigned slot=0,bool keyboard=true,Kind kind=Kind::Guns){Input(slot,true,true,keyboard,0,0,kind);assert(GetSnapshot(slot).open);auto c=Context(slot?Q:P);GuestUpdate(c,g_base);assert(GetSnapshot(slot).count);}
static void Choose(unsigned index,unsigned slot=0){
    const float angle=float(index)*6.283185307179586f/float(GetSnapshot(slot).count);
    Input(slot,true,true,false,std::sin(angle),-std::cos(angle),GetSnapshot(slot).kind);
}
static void Queue(unsigned index,unsigned slot=0,Kind kind=Kind::Guns){Open(slot,false,kind);Choose(index,slot);assert(GetSnapshot(slot).selected==int(index));Input(slot,false,true,false,0,0,kind);}
static void Consume(unsigned slot=0){auto c=Context(slot?Q:P);WheelConsumer(c,g_base);assert(c.esp==S+4&&c.ebx==17&&c.edi==23&&c.ebp==29);}
static unsigned checks=0;
static void Check(bool value){assert(value);++checks;}
static void ConversationRestrictions(){
    // The interaction fixture owns the authored lock predicate. Here it is an
    // explicit boundary: both wheel groups must honor it for either actor.
    for(Kind kind:{Kind::Guns,Kind::MeleeThrowables})for(unsigned slot=0;slot<2;++slot){
        const uint32_t actor=slot?Q:P,other=slot?P:Q;
        const unsigned index=kind==Kind::Guns?1:2;
        const uint32_t chosen=kind==Kind::Guns?2:1;
        Reset();Open(slot,false,kind);Choose(index,slot);
        conversation_locked=true;auto c=Context(actor);GuestUpdate(c,g_base);
        Check(!GetSnapshot(slot).available&&!GetSnapshot(slot).open);
        Input(slot,false,true,false,0,0,kind);Input(slot,true,true,false,0,0,kind);
        Check(!GetSnapshot(slot).open);Consume(slot);
        Check(!publications&&!transitions&&R32(actor+0x500)==17&&R32(other+0x500)==17);
        conversation_locked=false;GuestUpdate(c,g_base);
        Check(GetSnapshot(slot).available);Input(slot,false,true,false,0,0,kind);
        Queue(index,slot,kind);Consume(slot);
        Check(publications==1&&transitions==1&&R32(actor+0x500)==chosen&&R32(other+0x500)==17);

        // Start the conversation after release queued a selection, without a
        // UI refresh. The native consumer must reject and discard that command.
        Reset();Queue(index,slot,kind);Check(Blocked(slot));
        conversation_locked=true;Consume(slot);
        Check(!publications&&!transitions&&R32(actor+0x500)==17&&R32(other+0x500)==17);
        conversation_locked=false;Consume(slot);
        Check(!publications&&!transitions&&R32(actor+0x500)==17);
        c=Context(actor);GuestUpdate(c,g_base);Input(slot,false,true,false,0,0,kind);
        Queue(index,slot,kind);Consume(slot);
        Check(publications==1&&transitions==1&&R32(actor+0x500)==chosen&&R32(other+0x500)==17);
    }
}
int main(){
    ConversationRestrictions();
    Reset();Check(GetSnapshot(0).available&&eligibility_calls==0);Open();auto s=GetSnapshot(0);Check(s.available&&s.kind==Kind::Guns&&s.count==4&&s.entries[0].slot==17&&s.entries[1].slot==2&&s.entries[3].slot==10&&s.current==0&&eligibility_calls==4);
    Reset();Open(0,false,Kind::MeleeThrowables);s=GetSnapshot(0);Check(s.kind==Kind::MeleeThrowables&&s.count==4&&s.current==-1&&s.entries[0].slot==8&&s.entries[1].slot==11&&s.entries[2].slot==1&&s.entries[3].slot==9&&eligibility_calls==4);
    for(Kind kind:{Kind::Guns,Kind::MeleeThrowables})for(unsigned slot=0;slot<2;++slot)for(unsigned index=0;index<4;++index){
        Reset();Queue(index,slot,kind);Check(Blocked(slot));Consume(slot);
        const auto actor=slot?Q:P;Check(R32(actor+0x500)==GetSnapshot(slot).entries[index].slot);Check(R32(actor+0x4fc)==17);
        const bool changed=kind==Kind::MeleeThrowables||index;
        Check(R32((slot?P:Q)+0x500)==17);Check(publications==unsigned(changed));Check(transitions==unsigned(changed));Check(Blocked(slot));PublishInput(slot);Check(Blocked(slot));PublishInput(slot);Check(!Blocked(slot));
    }
    // The equipped item can be absent from the selected group's UI. Its private
    // slot still qualifies the command (Taser -> pistol and pistol -> Taser).
    Reset();W32(P+0x4fc,1);W32(P+0x500,1);auto group_context=Context();GuestUpdate(group_context,g_base);Queue(1);
    Check(GetSnapshot(0).current==-1);Consume();Check(publications==1&&R32(P+0x500)==2&&R32(P+0x4fc)==1);
    Reset();Queue(2,0,Kind::MeleeThrowables);Check(GetSnapshot(0).current==-1);Consume();Check(R32(P+0x500)==1&&R32(P+0x4fc)==17);
    // A group/source handoff cancels both open and queued selections. Reopening
    // requires all controls released; no stale item can cross a group boundary.
    for(bool queued:{false,true}){
        Reset();if(queued)Queue(1);else{Open(0,false);Choose(1);}
        Input(0,true,true,false,0,-1,Kind::MeleeThrowables);Check(!GetSnapshot(0).open);Consume();Check(!publications);
        Input(0,true,true,false,0,-1,Kind::MeleeThrowables);Check(!GetSnapshot(0).open);
        Input(0,false,true,false,0,0,Kind::MeleeThrowables);Queue(2,0,Kind::MeleeThrowables);Consume();Check(R32(P+0x500)==1&&publications==1);
    }
    for(Kind kind:{Kind::Guns,Kind::MeleeThrowables}){
        Reset();Queue(kind==Kind::Guns?1:2,0,kind);const uint32_t definition=kind==Kind::Guns?2:1;
        W32(0x4e55d0+definition*0xf0+8,kind==Kind::Guns?2:0);Consume();Check(!publications);
        Reset();Queue(1,0,kind);Cancel(0);Input(0,true,false,false,0,0,kind);Consume();Check(!publications);
    }
    Reset();Open(0,false,Kind::MeleeThrowables);Choose(2);Input(0,false,true,false,0,0,Kind::MeleeThrowables);Consume();Check(R32(P+0x500)==1); // Released stick preserves hover.
    Reset();Input(0,true,true,false,0,0,Kind::MeleeThrowables);Input(0,false,true,false,0,0,Kind::MeleeThrowables);Consume();Check(!publications); // Tap before refresh.
    Reset();Open(0,false);Input(0,true,true,false,0,0,static_cast<Kind>(9));Check(!GetSnapshot(0).open);Consume();Check(!publications);
    // Reentrant input during native queries cannot publish an old group's
    // snapshot into a newly opened wheel or commit a cancelled command.
    Reset();Input(0,true,true,false,0,0);query_action=2;group_context=Context();GuestUpdate(group_context,g_base);
    s=GetSnapshot(0);Check(s.open&&s.kind==Kind::MeleeThrowables&&s.count==0);
    GuestUpdate(group_context,g_base);s=GetSnapshot(0);Check(s.count==4&&s.entries[2].definition==1);Choose(2);Input(0,false,true,false,0,0,Kind::MeleeThrowables);Consume();Check(R32(P+0x500)==1);
    Reset();Queue(1);query_action=1;Consume();Check(!publications&&!GetSnapshot(0).open);
    // Malformed unrelated lists do not add queries to the active group.
    Reset();W32(P+0x498+0x94,0xffffffffu);Open();Check(GetSnapshot(0).count==4&&eligibility_calls==4);
    Reset();for(unsigned id:{1u,8u,9u,11u})G<uint8_t>(ItemAt(0,id))=0;
    Input(0,true,true,false,0,0,Kind::MeleeThrowables);group_context=Context();GuestUpdate(group_context,g_base);Check(!GetSnapshot(0).open&&!publications);
    // Disabled wheel is bit-for-bit native consumer behavior, including memory.
    for(unsigned category=0;category<3;++category){
        Reset();native_press=true;native_category=category;auto a=Context();sub_000C4CB0(a,g_base);const std::vector<uint8_t> expected(memory.begin(),memory.end());const auto events=publications,changes=transitions;
        Reset();native_press=true;native_category=category;auto b=Context();WheelConsumer(b,g_base);
        Check(std::memcmp(&a,&b,sizeof a)==0);Check(std::memcmp(expected.data(),memory.data(),memory.size())==0);Check(events==publications&&changes==transitions);
    }
    Reset();Open();Check(MouseDelta(180,0));Check(GetSnapshot(0).selected==1);Input(0,false,true,true,0,0);Consume();Check(R32(P+0x500)==2);
    for(unsigned shield_gate:{0u,1u}){
        Reset();W32(P+0x4fc,0);W32(P+0x438,2);G<uint8_t>(P+0x498+0xf5)=uint8_t(shield_gate);
        auto shield_context=Context();GuestUpdate(shield_context,g_base);Queue(1);Consume();
        Check(shield==1&&publications==1&&transitions==shield_gate&&R32(P+0x500)==2&&R32(P+0x4fc)==0);
        Check(R32(P+0x438)==0&&G<uint8_t>(P+0x498+0xf5)==shield_gate);
    }
    Reset();Open();Input(0,false,true,true,0,0);Check(Blocked(0));Check(!GetSnapshot(0).open);PublishInput(0);Check(Blocked(0));PublishInput(0);Check(!Blocked(0));Consume();Check(!publications);
    Reset();Open();Cancel(0);Input(0,true,true,true,0,0);Check(!GetSnapshot(0).open);Input(0,false,true,true,0,0);Open();
    Reset();Open();Input(0,true,false,true,0,0);Input(0,true,true,true,0,0);Check(!GetSnapshot(0).open);
    Reset();Open();players[0].updated-=std::chrono::seconds(1);Check(!Blocked(0)&&!GetSnapshot(0).available);
    Reset();Open();Choose(1);Check(!GetSnapshot(0).open); // Changing input source cancels.
    // Native ammo authority: loaded, reserve, extra, empty and unowned.
    for(unsigned kind=0;kind<5;++kind){
        Reset();const auto item=ItemAt(0,2);W32(item+0xc,kind==0?1:0);W32(R+2*4,kind==1?1:0);W32(item+0x18,kind==2?1:0);if(kind==4)G<uint8_t>(item)=0;
        if(kind==4){Open();Check(GetSnapshot(0).count==3);continue;}Queue(1);Check(GetSnapshot(0).entries[1].usable==(kind<3));Consume();Check(publications==(kind<3?1u:0u));
    }
    // No stale release can cross inventory removal, map/actor identity reuse,
    // invalid memory, death, pause, authored camera, or original switch gates.
    for(unsigned mode=0;mode<14;++mode){
        Reset();Queue(1);
        switch(mode){
        case 0:G<uint8_t>(ItemAt(0,2))=0;break;
        case 1:W32(P+0x54,999);break;
        case 2:W32(L+0x40+100*8,Q);break;
        case 3:W32(W+0x10,0x800);break;
        case 4:G<float>(P+0x9c)=0;break;
        case 5:W32(P+0x170,13);break;
        case 6:G<uint8_t>(P+0x304)=1;break;
        case 7:G<uint8_t>(P+0x202)=1;break;
        case 8:G<uint8_t>(P+0x2ac)=1;break;
        case 9:W32(P+0x170,8);W32(P+0x174,1);break;
        case 10:W32(P+0x4fc,25);break;
        case 11:W32(O+4,3);break;
        case 12:W32(ItemAt(0,2)+4,5);break;
        case 13:W32(P+0x4fc,5);break;
        }
        Consume();Check(!publications);
    }
    Reset();Queue(1);mapped=false;auto c=Context();Check(!GuestSelect(c,g_base));mapped=true;Check(!publications);
    Reset();Open();c=Context();c.edi=P;G<float>(S+0x30)=3;G<float>(S+0x3c)=-2;const auto before=c;GuestLook(c,g_base);Check(G<float>(S+0x30)==0&&G<float>(S+0x3c)==0&&std::memcmp(&before,&c,sizeof c)==0);
    // Native exceptions retain failure semantics and restore borrowed stack/FP.
    Reset();Queue(1);c=Context();std::array<uint8_t,1024> stack_bytes;std::memcpy(stack_bytes.data(),g_base+S-1024,1024);const auto original=c;const unsigned csr=_mm_getcsr();fail_publish=true;
    bool caught=false;try{GuestSelect(c,g_base);}catch(const std::runtime_error&){caught=true;}
    Check(caught&&std::memcmp(&original,&c,sizeof c)==0&&_mm_getcsr()==csr);Check(std::memcmp(stack_bytes.data(),g_base+S-1024,1024)==0);
    // Concurrent host readers/input never read guest memory or block a guest callback.
    Reset();std::thread host([]{for(unsigned i=0;i<1000;++i){Input(1,false,true,false,0,0,i&1?Kind::Guns:Kind::MeleeThrowables);(void)GetSnapshot(0);(void)Blocked(1);}});
    for(unsigned i=0;i<1000;++i){c=Context();GuestUpdate(c,g_base);}host.join();Check(GetSnapshot(0).available&&eligibility_calls==0);
    for(unsigned state:{0u,1u,2u,4u,5u,6u,8u,9u,10u}){
        Reset();W32(P+0x170,state);W32(P+0x174,2);c=Context();GuestUpdate(c,g_base);Queue(1);Consume();Check(publications==1&&transitions==1);
    }
    Reset();Open();Check(!SlowTimeActive());Cancel(0);Input(0,false,true,true,0,0);trial_coop::SetActive(false);c=Context();GuestUpdate(c,g_base);Open();Check(SlowTimeActive()&&SlowTimeActive(W)&&!SlowTimeActive(W+4));Cancel(0);Check(!SlowTimeActive());
    Input(0,false,true,true,0,0);trial_coop::Request();GuestUpdate(c,g_base);Open();Check(!SlowTimeActive());
    Reset();trial_coop::SetActive(false);c=Context();GuestUpdate(c,g_base);Open(0,true,Kind::MeleeThrowables);Check(SlowTimeActive()&&SlowTimeActive(W));Cancel(0);Check(!SlowTimeActive());
    std::printf("weapon wheel: %u checks passed (two groups, native consumer, ammo, co-op, conversation locks, cancellation, reentrancy, concurrency)\n",checks);
}
