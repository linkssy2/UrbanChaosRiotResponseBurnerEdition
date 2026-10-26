// Execute the retail conversation state machine, range/facing leaf, owner
// lookup and control callback. Resource/audio/event services are boundaries;
// this does not claim live NPC animation, streaming or controller verification.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <vector>
#include "kernel/kernel.h"
namespace kernel {static std::array<uint8_t,0x800000> memory{};uint8_t* g_base=memory.data();}
using namespace kernel;
namespace mem {static uint32_t denied=0xffffffffu;bool IsCommitted(uint32_t page){return page<memory.size()&&page!=denied;}}
static constexpr uint32_t p1=0x600000,p2=0x602000,behavior=0x604000,npc=0x605000,controller=0x606000,
    test_world=0x607000,manager=0x609000,proxy=0x60a000,fade=0x60b000,event_root=0x60c000,
    events=0x60d000,animation=0x60e000,stack=0x700000;
uint32_t TrialCoopPlayer(unsigned slot){return slot==0?p1:slot==1?p2:0;}
static bool candidate=true,keyboard_mode=true,faces[2]{true,true},pressed[2]{},skip_edge[2]{},playing=false,busy=false,flags_ready=true,fail_position=false;
static unsigned speech_starts=0,script_calls=0,transitions=0,fade_calls=0,query_ports[2]{};
static uint32_t last_property_owner=0;
int xbox_KeyboardMode(){return keyboard_mode;}
#include "../common/xbe_coop_interactions.cpp"
#include "../common/xbe_conversation_skip.cpp"

#define DECLARE(a) extern "C" void sub_##a(X86Context&,uint8_t*)
DECLARE(0004A840);DECLARE(0004ACD0);DECLARE(00207D70);DECLARE(001F7A70);DECLARE(00033B60);
DECLARE(001F9010);DECLARE(001F90A0);DECLARE(000343B0);DECLARE(00034450);DECLARE(00032E60);
DECLARE(00036250);DECLARE(00036280);DECLARE(000329F0);DECLARE(000DEC10);DECLARE(000DEEC0);
DECLARE(0021A4C0);DECLARE(0021A4F0);DECLARE(0022D360);DECLARE(0017BB40);DECLARE(0017BC70);
DECLARE(000325A0);DECLARE(00032680);DECLARE(000328F0);
#undef DECLARE
extern "C" void OriginalControlLock(X86Context&,uint8_t*);
extern "C" void OriginalConversationEnd(X86Context&,uint8_t*);
extern "C" void CoopConversationEnd(X86Context&,uint8_t*);
extern "C" void OriginalRange(X86Context&,uint8_t*);
extern "C" void CoopRange(X86Context&,uint8_t*);
extern "C" void OriginalSkip(X86Context&,uint8_t*);
extern "C" void CoopSkip(X86Context&,uint8_t*);
extern "C" void OriginalTurn(X86Context&,uint8_t*);
extern "C" void CoopTurn(X86Context&,uint8_t*);
[[noreturn]] void x86_trap(X86Context&,uint32_t,const char*){std::abort();}
void x86_icall(X86Context& c,uint8_t* b,uint32_t target){
    if(target==0x33b60){sub_00033B60(c,b);return;}
    if(target==0x1f90a0){sub_001F90A0(c,b);return;}
    if(target==0xf0001){
        assert(c.ecx==test_world);const auto slot=R32(c.esp+4),action=R32(c.esp+8),mode=R32(c.esp+12);
        assert(slot<2);++query_ports[slot];
        assert((action==24&&mode==2)||(action==22&&mode==1));
        x87_push(c,action==24?pressed[slot]:skip_edge[slot]);c.esp+=16;return;
    }
    if(target==0xf0002){if(fail_position)throw std::runtime_error("position boundary");assert(c.ecx==proxy);c.eax=npc+0x40;c.esp+=8;return;}
    if(target==0xf0003){assert(c.ecx==proxy);c.eax=animation;c.esp+=4;return;}
    if(target==0xf0004){assert(c.ecx==animation);c.esp+=4;return;}
    if(target==0xf0005){assert(c.ecx==fade);++fade_calls;c.esp+=32;return;}
    if(target==0xf0006){assert(c.ecx==fade);++fade_calls;TrialConversationPrompt(c,b);c.esp+=8;return;}
    assert(false);
}
X86_FUNC(sub_0004A840){ctx.eax=manager;ctx.esp+=4;}
X86_FUNC(sub_0004ACD0){assert(ctx.ecx==manager&&R32(ctx.esp+4)==0);ctx.eax=R32(manager+0x834);ctx.esp+=8;}
X86_FUNC(sub_00207D70){assert(ctx.ecx==test_world);const auto id=R32(ctx.esp+4);ctx.eax=id==11?p1:id==22?p2:0;ctx.esp+=8;}
X86_FUNC(sub_001F7A70){
    last_property_owner=R32(ctx.esp+4);const auto output=R32(ctx.esp+12);const unsigned slot=last_property_owner==11?0:1;
    assert(last_property_owner==11||last_property_owner==22);
    std::memset(base+output,0,96);
    for(unsigned axis=0;axis<3;++axis)G<float>(output+0x40+axis*4)=(G<float>(npc+0x40+axis*4)-G<float>((slot?p2:p1)+0x40+axis*4))*(faces[slot]?1.f:-1.f);
    ctx.eax=1;ctx.esp+=4; // cdecl, original caller removes all three arguments
}
X86_FUNC(sub_00033B60){if(candidate)CoopRange(ctx,base);else OriginalRange(ctx,base);}
X86_FUNC(sub_001F9010){x87_push(ctx,.25);ctx.esp+=4;}
X86_FUNC(sub_000343B0){ctx.eax=busy;ctx.esp+=4;}
X86_FUNC(sub_00034450){ctx.esp+=4;}
X86_FUNC(sub_00032E60){ctx.eax=flags_ready;ctx.esp+=12;}
X86_FUNC(sub_00036250){ctx.eax=1;ctx.esp+=4;}
X86_FUNC(sub_00036280){ctx.esp+=4;}
X86_FUNC(sub_000329F0){assert(ctx.ecx==behavior);++script_calls;ctx.esp+=16;}
X86_FUNC(sub_000DEC10){assert(ctx.ecx==npc);++speech_starts;playing=true;ctx.esp+=12;}
X86_FUNC(sub_000DEEC0){assert(ctx.ecx==npc);playing=false;ctx.esp+=4;}
X86_FUNC(sub_0021A4C0){ctx.eax=playing;ctx.esp+=4;}
X86_FUNC(sub_0021A4F0){ctx.eax=1;ctx.esp+=4;}
X86_FUNC(sub_0022D360){assert(ctx.ecx==test_world);ctx.eax=fade;ctx.esp+=4;}
X86_FUNC(sub_0017BB40){if(candidate)CoopSkip(ctx,base);else OriginalSkip(ctx,base);}
X86_FUNC(sub_0017BC70){ctx.esp+=8;} // Turning animation is tested separately at its actor-property boundary.
X86_FUNC(sub_00032680){ctx.esp+=8;}
X86_FUNC(sub_000328F0){if(candidate)TrialCoopInteractionClose(ctx,base,ctx.ecx);W32(ctx.ecx+0xd0,0);W32(ctx.ecx+0xd4,0);ctx.esp+=4;}
X86_FUNC(sub_000325A0){
    assert(ctx.ecx==behavior);++transitions;X86Context temp=ctx;temp.esp-=128;W32(temp.esp,0x1234);
    if(candidate)CoopConversationEnd(temp,base);else OriginalConversationEnd(temp,base);
    ctx.esp+=8;
}
X86_FUNC(sub_001F90A0){
    assert(ctx.ecx==events&&R32(ctx.esp+8)==0x95&&R32(ctx.esp+16)==1);
    const auto owner=R32(ctx.esp+4);assert(owner==11||owner==22);
    X86Context event=ctx;event.ecx=owner==11?p1:p2;event.esp=ctx.esp-64;
    W32(event.esp,0x1234);W32(event.esp+4,R32(ctx.esp+12));W32(event.esp+8,R32(ctx.esp+20));
    OriginalControlLock(event,base);ctx.esp+=24;
}
namespace kernel {
// PRODUCTION_CALL_GUEST
}
// ORIGINAL_INTERACTIONS

static void Seed(){
    std::memset(g_base+0x600000,0,0x100000);TrialCoopInteractionReset();trial_coop::Reset();trial_coop::Request();trial_coop::ResourcesReady(true);trial_coop::SetActive(true);candidate=true;
    keyboard_mode=true;faces[0]=faces[1]=true;pressed[0]=pressed[1]=skip_edge[0]=skip_edge[1]=false;
    playing=busy=fail_position=false;flags_ready=true;speech_starts=script_calls=transitions=fade_calls=query_ports[0]=query_ports[1]=0;
    for(unsigned i=0;i<2;++i){const auto p=i?p2:p1;W32(p,0x46fbe8);W32(p+0x54,i?22:11);W32(p+0x368,i);G<float>(p+0x9c)=1000;G<float>(p+0x40)=float(i*10);}
    W32(0x51c4dc,test_world);W32(test_world,0x60f000);W32(0x60f008,0xf0001);W32(manager+0x834,22);
    W32(behavior,0x471060);W32(behavior+0x54,33);W32(behavior+0x50,66);W32(behavior+0xd0,controller);W32(behavior+0xd4,npc);
    W32(controller+0xc,npc);W32(npc+0x54,44);W32(npc+0x118,proxy);G<float>(npc+0x48)=100;
    W32(proxy,0x60f100);W32(0x60f12c,0xf0002);W32(0x60f1a8,0xf0003);
    W32(animation,0x60f200);W32(0x60f224,0xf0004);
    W32(fade,0x48154c);W32(0x481554,0xf0005);W32(0x481564,0xf0006);
    W32(0x51c398,event_root);W32(event_root+8,events);
    G<float>(behavior+0x114)=300;G<float>(behavior+0x124)=20;
    g_base[behavior+0x119]=g_base[behavior+0x154]=1;
    W32(behavior+0xfc,1);W32(behavior+0x104,2);W32(behavior+0x10c,3);W32(behavior+0x150,4);
    W32(behavior+0x138,0x60ff00);g_base[0x60ff00]='S';W32(behavior+0x13c,0x60ff10);
    // ORIGINAL_CONSTANTS
    TrialConversationClose(behavior);TrialConversationPublish(false,true);
}
static X86Context Context(){X86Context c{};c.eax=p1+1;c.ecx=behavior;c.esi=0x1234;c.edi=0x9876;c.ebx=0x7777;c.ebp=0x8888;c.esp=stack;W32(stack,0xbeef);W32(stack+4,0);return c;}
static void CallBoundary(){
    for(const uint32_t stale:{0u,p1+1,0xdeadbeefu}){
        Seed();auto c=Context();c.eax=stale;interactions::Call call(c,g_base);
        const uint32_t result=call.Run(0xf0002,proxy,{call.sp+128});
        assert(result==npc+0x40&&call.c.eax==stale&&call.c.ecx==proxy&&call.c.esp==call.sp);
        assert(call.c.edx==c.edx&&call.c.ebx==c.ebx&&call.c.esi==c.esi&&call.c.edi==c.edi&&call.c.ebp==c.ebp);
    }
}
static void Tick(){auto c=Context();if(candidate)CoopConversation(c,g_base);else OriginalConversation(c,g_base);assert(c.esp==stack+8&&c.esi==0x1234&&c.edi==0x9876&&c.ebx==0x7777&&c.ebp==0x8888);}
static void Ownership(){
    // Negative witness: native manager points at P2, so P1 looking at an NPC
    // cannot start a talk while P2 looks away. The candidate admits either.
    Seed();candidate=false;faces[1]=false;Tick();assert(R32(behavior+0x158)==0&&!g_base[p1+0x202]);
    for(unsigned owner=0;owner<2;++owner){
        Seed();faces[1-owner]=false;Tick();assert(R32(behavior+0x158)==2);
        assert(g_base[(owner?p2:p1)+0x202]&&!g_base[(owner?p1:p2)+0x202]);
        assert(TrialCoopInteractionPlayer(g_base,behavior)==owner+1&&R32(manager+0x834)==22&&fade_calls==0);
        Tick();assert(R32(behavior+0x158)==1&&script_calls==1);Tick();assert(R32(behavior+0x158)==3&&speech_starts==1);
        for(unsigned i=0;i<8;++i)Tick();assert(TrialCoopInteractionSnapshot(owner).skip&&!TrialCoopInteractionSnapshot(1-owner).skip);
        // Walking the companion away does not lock them. Completion waits at
        // the script boundary and cannot replay speech or publish it twice.
        const auto partner=owner?p1:p2;G<float>(partner+0x40)=1000;playing=false;
        for(unsigned i=0;i<4;++i)Tick();assert(R32(behavior+0x158)==3&&script_calls==1&&speech_starts==1);
        assert(TrialCoopInteractionSnapshot(owner).waiting&&!g_base[partner+0x202]&&!g_base[(owner?p2:p1)+0x202]);
        G<float>(partner+0x40)=0;Tick();assert(R32(behavior+0x158)==4&&script_calls==2);
        Tick();assert(R32(behavior+0x158)==6&&script_calls==3&&!g_base[(owner?p2:p1)+0x202]);
        Tick();assert(transitions==1&&!TrialCoopInteractionSnapshot(owner).skip&&!TrialCoopInteractionSnapshot(owner).waiting);
    }
}
static void RendezvousRecovery(){
    for(unsigned owner=0;owner<2;++owner)for(bool before_speech:{false,true}){
        Seed();faces[1-owner]=false;Tick();
        const auto initiator=owner?p2:p1,partner=owner?p1:p2;
        if(!before_speech){Tick();Tick();assert(playing&&speech_starts==1);playing=false;}
        assert(g_base[initiator+0x202]);
        // The mobile companion is down outside the NPC's authored range.
        // Executing the native machine must let the owner reach them without
        // advancing/replaying the pending script or transferring ownership.
        G<float>(partner+0x40)=1000;G<float>(partner+0x9c)=0;W32(partner+0x170,12);
        const auto phase=R32(behavior+0x158),scripts=script_calls,speech=speech_starts;
        for(unsigned i=0;i<3;++i)Tick();
        assert(!g_base[initiator+0x202]&&!g_base[partner+0x202]&&!g_base[behavior+0x129]);
        assert(R32(behavior+0x158)==phase&&script_calls==scripts&&speech_starts==speech);
        assert(TrialCoopInteractionPlayer(g_base,behavior)==owner+1&&TrialCoopInteractionSnapshot(owner).waiting);
        // Model only the health/position boundary of an already tested revive:
        // healing alone outside the range still waits, then both return.
        G<float>(initiator+0x40)=1000;G<float>(partner+0x9c)=1000;W32(partner+0x170,0);Tick();
        assert(!g_base[initiator+0x202]&&R32(behavior+0x158)==phase&&script_calls==scripts);
        G<float>(initiator+0x40)=G<float>(partner+0x40)=0;Tick();
        assert(script_calls==scripts+1&&!TrialCoopInteractionSnapshot(owner).waiting);
        assert(bool(g_base[initiator+0x202])==before_speech&&!g_base[partner+0x202]);
        if(before_speech){assert(R32(behavior+0x158)==1);Tick();assert(speech_starts==1);playing=false;Tick();}
        assert(R32(behavior+0x158)==4);Tick();Tick();
        assert(transitions==1&&script_calls==3&&speech_starts==1&&!g_base[initiator+0x202]);
    }
}
static void Admission(){
    Seed();faces[1]=false;G<float>(p2+0x40)=1000;Tick();assert(R32(behavior+0x158)==0&&TrialCoopInteractionSnapshot(0).waiting);
    G<float>(p2+0x40)=0;Tick();assert(R32(behavior+0x158)==2);
    Seed();g_base[behavior+0x118]=1;faces[0]=false;pressed[0]=true;Tick();assert(R32(behavior+0x158)==0);
    pressed[1]=true;Tick();assert(R32(behavior+0x158)==2&&TrialCoopInteractionPlayer(g_base,behavior)==2&&query_ports[1]>=2);
    Seed();busy=true;Tick();assert(R32(behavior+0x158)==0);busy=false;flags_ready=false;Tick();assert(R32(behavior+0x158)==0);
    Seed();faces[0]=faces[1]=false;Tick();assert(R32(behavior+0x158)==0);
    Seed();G<float>(p2+0x9c)=0;Tick();assert(R32(behavior+0x158)==0);
    Seed();g_base[p1+0x202]=1;Tick();assert(TrialCoopInteractionPlayer(g_base,behavior)==2&&g_base[p1+0x202]);
    Seed();assert(interactions::Pair(g_base));
    constexpr uint32_t other=0x610000;
    std::memcpy(g_base+other,g_base+behavior,0x15c);W32(other+0x54,55);W32(other+0x158,2);g_base[other+0x129]=1;
    auto* occupied=interactions::Find(g_base,other,true);assert(occupied);occupied->slot=0;
    Tick();assert(TrialCoopInteractionPlayer(g_base,behavior)==2&&!g_base[p1+0x202]&&g_base[p2+0x202]);
    Seed();trial_coop::SetActive(false);Tick();assert(R32(behavior+0x158)==2&&g_base[p2+0x202]&&!g_base[p1+0x202]&&fade_calls>0);
    Seed();assert(interactions::Pair(g_base)); // Establish the current world before filling its record table.
    for(unsigned i=0;i<interactions::records.size();++i)interactions::records[i].behavior=0x610000+i*0x200;
    Tick();assert(R32(behavior+0x158)==0&&!g_base[p1+0x202]&&!g_base[p2+0x202]); // exhaustion cannot fall back to native ownership
}
static void SkipAndTurn(){
    for(unsigned owner=0;owner<2;++owner){
        Seed();faces[1-owner]=false;Tick();Tick();Tick();
        G<float>(behavior+0x144)=2;keyboard_mode=owner==1; // global mode intentionally belongs to the other player
        TrialConversationPublish(true,true);skip_edge[0]=true;skip_edge[1]=false;
        Tick();if(owner==1)assert(R32(behavior+0x158)==3);else assert(R32(behavior+0x158)==4);
        if(owner==1){TrialConversationPublish(false,true);skip_edge[1]=true;Tick();assert(R32(behavior+0x158)==4);}
        auto c=Context();c.esi=c.ecx=behavior;TrialCoopInteractionScope scoped(c,g_base);assert(scoped.Update());
        CoopTurn(c,g_base);assert(last_property_owner==(owner?22u:11u));
    }
}
static void LifecycleAndSafety(){
    Seed();faces[1]=false;Tick();assert(g_base[p1+0x202]);
    // Original abort would unlock registered P2 and strand P1. Execute its
    // actual native end routine with and without the selected-owner correction.
    auto c=Context();candidate=false;OriginalConversationEnd(c,g_base);assert(g_base[p1+0x202]);
    Seed();faces[1]=false;Tick();g_base[p2+0x202]=1;g_base[behavior+0x148]=1;c=Context();CoopConversationEnd(c,g_base);
    assert(!g_base[p1+0x202]&&g_base[p2+0x202]&&!g_base[behavior+0x148]&&!TrialCoopInteractionSnapshot(0).skip);
    Seed();faces[1]=false;Tick();Tick();Tick();G<float>(p1+0x9c)=0;W32(p1+0x170,12);Tick();
    assert(!g_base[p1+0x202]&&!g_base[p2+0x202]&&R32(behavior+0x158)==3);
    G<float>(p1+0x9c)=1000;W32(p1+0x170,0);Tick();assert(g_base[p1+0x202]&&R32(behavior+0x158)==3);
    W32(p1+0x54,99);TrialCoopInteractionPlayer(g_base,behavior);assert(!TrialCoopInteractionSnapshot(0).skip);
    Seed();c=Context();const auto before=c;std::array<uint8_t,4096> saved{};std::memcpy(saved.data(),g_base+stack-4096,4096);
    const auto csr=_mm_getcsr();{
        TrialCoopInteractionScope scoped(c,g_base);assert(scoped.Update());assert(scoped.Begin());
        assert(!std::memcmp(&c,&before,sizeof c)&&!std::memcmp(saved.data(),g_base+stack-4096,4096)&&_mm_getcsr()==csr);
    }
    fail_position=true;try{TrialCoopInteractionScope scoped(c,g_base);assert(scoped.Update());scoped.Begin();assert(false);}catch(const std::runtime_error&){}
    assert(!std::memcmp(&c,&before,sizeof c)&&!std::memcmp(saved.data(),g_base+stack-4096,4096)&&_mm_getcsr()==csr);
    X86Context untouched{};untouched.eax=777;TrialCoopInteractionLookup(untouched,g_base);assert(untouched.eax==777);
    TrialCoopInteractionReset();assert(!TrialCoopInteractionSnapshot(0).skip&&!TrialCoopInteractionSnapshot(1).waiting);
}
int main(){CallBoundary();Ownership();RendezvousRecovery();Admission();SkipAndTurn();LifecycleAndSafety();std::puts("PASS production CallGuest return/restore ABI and stale EAX isolation, original NPC range/facing and conversation lifecycle, either actor initiation, original per-actor lock/unlock, independent input/skip/turn target, partner-free movement with deferred script commitment, owner released to revive/retrieve a missing partner without repeated speech, native single-player fallback, abort/down/revive/reset, context/stack/MXCSR guards; live dialogue visuals pending");}
