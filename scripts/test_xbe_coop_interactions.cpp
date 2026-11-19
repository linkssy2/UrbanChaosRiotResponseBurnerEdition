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
static bool candidate=true,keyboard_mode=true,faces[2]{true,true},pressed[2]{},skip_edge[2]{},playing=false,busy=false,flags_ready=true,fail_position=false,fail_lock=false;
static unsigned speech_starts=0,script_calls=0,transitions=0,fade_calls=0,query_ports[2]{};
static uint32_t last_property_owner=0;
static bool mixed_input=false;
static bool weapon_done=true,manual_holster=false,fail_weapon=false,nested_weapon=false;
static unsigned lowered=0,raised=0,weapon_animations=0,weapon_checks=0,attack_sinks[4]{};
static constexpr uint32_t weapon_item=0x60f600,weapon_definition=0x60f680,weapon_animation=0x60f800;
namespace trial_controls {unsigned MixedInput(){return mixed_input;}}
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
extern "C" void OriginalWeaponDispatch(X86Context&,uint8_t*);
extern "C" void CoopWeaponDispatch(X86Context&,uint8_t*);
extern "C" void OriginalManualHolster(X86Context&,uint8_t*);
extern "C" void CoopManualHolster(X86Context&,uint8_t*);
extern "C" void OriginalForceHolster(X86Context&,uint8_t*);
extern "C" void OriginalToggleHolster(X86Context&,uint8_t*);
extern "C" void OriginalFireOrRaise(X86Context&,uint8_t*);
extern "C" void OriginalEquipAnimation(X86Context&,uint8_t*);
// WEAPON_GUARD_DECLARATIONS
[[noreturn]] void x86_trap(X86Context&,uint32_t,const char*){std::abort();}
static bool scratch_throw=false,scratch_nested=false;
static unsigned scratch_calls=0;
void x86_icall(X86Context& c,uint8_t* b,uint32_t target){
    if(target==0xff001){
        ++scratch_calls;const auto output=R32(c.esp+4);assert(output<kernel::memory.size()-64);
        std::memset(b+c.esp-128,0x5a,96);std::memset(b+output,0x6b,64);
        if(scratch_nested){scratch_nested=false;interactions::Call inner(c,b);inner.Run(0xff001,c.ecx,{inner.sp+128});}
        if(scratch_throw)throw std::runtime_error("written scratch boundary");
        c.eax=0xaabbccdd;c.esp+=8;return;
    }
    if(target==0x33b60){sub_00033B60(c,b);return;}
    if(target==0x1f90a0){sub_001F90A0(c,b);return;}
    if(target==0xd2830){OriginalForceHolster(c,b);return;}
    if(target==0xd23e0){OriginalToggleHolster(c,b);return;}
    if(target==0xf0001){
        assert(c.ecx==R32(0x51c4dc));const auto slot=R32(c.esp+4),action=R32(c.esp+8),mode=R32(c.esp+12);
        assert(slot<2);++query_ports[slot];
        assert((action==24&&mode==2)||(action==22&&mode==1)||(action==31&&mode==1));
        x87_push(c,action==31?manual_holster:action==24?pressed[slot]:skip_edge[slot]);c.esp+=16;return;
    }
    if(target==0xf0002){if(fail_position)throw std::runtime_error("position boundary");assert(c.ecx==proxy);c.eax=npc+0x40;c.esp+=8;return;}
    if(target==0xf0003){assert(c.ecx==proxy);c.eax=animation;c.esp+=4;return;}
    if(target==0xf0004){assert(c.ecx==animation);c.esp+=4;return;}
    if(target==0xf0005){assert(c.ecx==fade);++fade_calls;c.esp+=32;return;}
    if(target==0xf0006){assert(c.ecx==fade);++fade_calls;TrialConversationPrompt(c,b);c.esp+=8;return;}
    if(target==0xf0007){c.eax=1;c.esp+=4;return;}
    std::fprintf(stderr,"unexpected indirect service %x at weapon tick %u ecx %x esp %x\n",target,weapon_checks,c.ecx,c.esp);assert(false);
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
    if(fail_lock)throw std::runtime_error("lock boundary");
    assert(ctx.ecx==events&&R32(ctx.esp+8)==0x95&&R32(ctx.esp+16)==1);
    const auto owner=R32(ctx.esp+4);assert(owner==11||owner==22);
    X86Context event=ctx;event.ecx=owner==11?p1:p2;event.esp=ctx.esp-64;
    W32(event.esp,0x1234);W32(event.esp+4,R32(ctx.esp+12));W32(event.esp+8,R32(ctx.esp+20));
    OriginalControlLock(event,base);ctx.esp+=24;
}
namespace kernel {
// PRODUCTION_CALL_GUEST
}
// Weapon animation/property services are explicit boundaries. The dispatcher,
// manual toggle, force-lower and holster/raise completion bodies are retail.
X86_FUNC(sub_000D23E0){OriginalToggleHolster(ctx,base);}
X86_FUNC(sub_000D2510){if(candidate)CoopManualHolster(ctx,base);else OriginalManualHolster(ctx,base);}
X86_FUNC(sub_000D1AE0){OriginalFireOrRaise(ctx,base);}
X86_FUNC(sub_000CF8A0){OriginalEquipAnimation(ctx,base);}
X86_FUNC(sub_00041F80){ctx.eax=weapon_item;ctx.esp+=4;}
X86_FUNC(sub_000422C0){ctx.eax=weapon_item;ctx.esp+=4;}
X86_FUNC(sub_000442D0){ctx.eax=weapon_definition;ctx.esp+=4;}
X86_FUNC(sub_00043C00){ctx.eax=0;ctx.esp+=4;}
X86_FUNC(sub_000BFE00){W32(ctx.ecx+0x170,R32(ctx.esp+4));W32(ctx.ecx+0x174,R32(ctx.esp+8));ctx.esp+=12;}
X86_FUNC(sub_000C1E60){++weapon_animations;ctx.esp+=16;}
X86_FUNC(sub_00216BD0){const auto name=R32(ctx.esp+8);if(name==0x458d78)++lowered;else if(name==0x458d80)++raised;ctx.esp+=16;}
X86_FUNC(sub_001FB4E0){ctx.eax=weapon_done;ctx.esp+=4;}
X86_FUNC(sub_000C8930){ctx.esp+=4;}
X86_FUNC(sub_000BE230){ctx.esp+=4;}
X86_FUNC(sub_000C5510){ctx.esp+=4;}
X86_FUNC(sub_000CF770){ctx.eax=0;ctx.esp+=4;}
X86_FUNC(sub_000643E0){ctx.eax=0x60f900;ctx.esp+=4;}
X86_FUNC(sub_002084E0){ctx.eax=0x60fa00;ctx.esp+=4;}
X86_FUNC(sub_002107C0){ctx.eax=0;ctx.esp+=12;}
X86_FUNC(sub_000BC5B0){ctx.esp+=4;}
X86_FUNC(sub_000BE7D0){ctx.esp+=4;}
X86_FUNC(sub_000BE870){ctx.esp+=4;}
X86_FUNC(sub_000D1A20){auto gate=ctx;gate.esi=ctx.ecx;if(NativeAttackGuard(gate,base))++attack_sinks[0];ctx.esp+=4;}
X86_FUNC(sub_000D0C30){auto gate=ctx;gate.esi=ctx.ecx;ctx.eax=NativeShieldAndAlternateGuard(gate,base);if(ctx.eax)++attack_sinks[1];ctx.esp+=4;}
X86_FUNC(sub_000C4CB0){auto gate=ctx;gate.esi=ctx.ecx;if(NativeSwitchGuard(gate,base))++attack_sinks[2];ctx.esp+=4;}
X86_FUNC(sub_000CA930){auto gate=ctx;gate.esi=ctx.ecx;gate.edi=0;if(NativeInteractionGuard(gate,base))++attack_sinks[3];ctx.esp+=4;}
X86_FUNC(sub_000C64D0){
    if(nested_weapon){nested_weapon=false;const auto lock=base[ctx.ecx+0x202],fire=base[ctx.ecx+0x4ac];
        X86Context inner=ctx;{TrialCoopInteractionWeaponScope scope(inner,base);assert(base[ctx.ecx+0x202]&&!(base[ctx.ecx+0x4ac]&1));}
        assert(base[ctx.ecx+0x202]==lock&&base[ctx.ecx+0x4ac]==fire);}
    if(fail_weapon)throw std::runtime_error("weapon update boundary");ctx.esp+=4;
}
bool TrialHealthPackInjectionContact(X86Context&,uint8_t*){std::abort();}
void TrialHealthPackInjectionClip(X86Context&,uint8_t*){std::abort();}
void TrialLadderHands(X86Context&,uint8_t*){std::abort();}
// WEAPON_GUARD_BODIES
// ORIGINAL_INTERACTIONS

static void Seed(){
    std::memset(g_base+0x600000,0,0x100000);TrialCoopInteractionReset();trial_coop::Reset();trial_coop::Request();trial_coop::ResourcesReady(true);trial_coop::SetActive(true);candidate=true;
    keyboard_mode=true;faces[0]=faces[1]=true;pressed[0]=pressed[1]=skip_edge[0]=skip_edge[1]=false;
    playing=busy=fail_position=fail_lock=false;flags_ready=true;speech_starts=script_calls=transitions=fade_calls=query_ports[0]=query_ports[1]=0;
    weapon_done=true;manual_holster=fail_weapon=nested_weapon=false;lowered=raised=weapon_animations=0;for(auto& count:attack_sinks)count=0;
    for(unsigned i=0;i<2;++i){const auto p=i?p2:p1;W32(p,0x46fbe8);W32(p+0x54,i?22:11);W32(p+0x368,i);G<float>(p+0x9c)=1000;G<float>(p+0x40)=float(i*10);}
    W32(0x51c4dc,test_world);W32(test_world,0x60f000);W32(0x60f008,0xf0001);W32(manager+0x834,22);
    W32(manager,0x60f700);W32(0x60f720,0xf0007);W32(weapon_item+4,2);
    W32(weapon_animation,weapon_animation+16);
    for(const auto p:{p1,p2}){W32(p+0x4fc,2);W32(p+0x180,weapon_animation);W32(p+0x4ac,1);}
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
static void Tick(uint32_t subject=behavior){auto c=Context();c.ecx=subject;if(candidate)CoopConversation(c,g_base);else OriginalConversation(c,g_base);assert(c.esp==stack+8&&c.esi==0x1234&&c.edi==0x9876&&c.ebx==0x7777&&c.ebp==0x8888);}
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
    for(unsigned i=0;i<interactions::records.size();++i){const uint32_t p=0x610000+i*0x200;
        std::memcpy(g_base+p,g_base+behavior,0x15c);W32(p+0x54,100+i);assert(interactions::Find(g_base,p,true));}
    Tick();assert(R32(behavior+0x158)==0&&!g_base[p1+0x202]&&!g_base[p2+0x202]); // exhaustion cannot fall back to native ownership
}
static void SkipAndTurn(){
    for(bool mixed:{false,true})for(unsigned owner=0;owner<2;++owner){
        Seed();mixed_input=mixed;faces[1-owner]=false;Tick();Tick();Tick();
        G<float>(behavior+0x144)=2;keyboard_mode=owner==1; // global mode intentionally belongs to the other player
        TrialConversationPublish(true,true);skip_edge[0]=true;skip_edge[1]=false;
        Tick();if(owner==1)assert(R32(behavior+0x158)==3);else assert(R32(behavior+0x158)==4);
        if(owner==1){TrialConversationPublish(false,true);skip_edge[1]=true;Tick();assert(R32(behavior+0x158)==4);}
        auto c=Context();c.esi=c.ecx=behavior;TrialCoopInteractionScope scoped(c,g_base);assert(scoped.Update());
        CoopTurn(c,g_base);assert(last_property_owner==(owner?22u:11u));
    }
    mixed_input=false;
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
static void WaitingAdmission(){
    for(bool blocked_by_busy:{false,true}){
        Seed();faces[1]=false;G<float>(p2+0x40)=1000;Tick();
        assert(R32(behavior+0x158)==0&&TrialCoopInteractionSnapshot(0).waiting);
        // These retail branches bypass Begin entirely after its previous wait.
        busy=blocked_by_busy;flags_ready=blocked_by_busy;G<float>(p2+0x40)=0;
        for(unsigned i=0;i<8;++i){Tick();assert(!TrialCoopInteractionSnapshot(0).waiting);}
        assert(R32(behavior+0x158)==0&&!script_calls&&!speech_starts&&!g_base[p1+0x202]);
        busy=false;flags_ready=true;Tick();assert(R32(behavior+0x158)==2&&g_base[p1+0x202]);
    }
    Seed();faces[1]=false;G<float>(p2+0x40)=1000;Tick();
    faces[0]=false;Tick();assert(!TrialCoopInteractionSnapshot(0).waiting&&R32(behavior+0x158)==0);
}
static void WaitingRevival(){
    for(unsigned owner=0;owner<2;++owner){
        Seed();faces[1-owner]=false;Tick();Tick();Tick();
        const auto actor=owner?p2:p1;assert(playing&&R32(behavior+0x158)==3);
        G<float>(actor+0x9c)=0;W32(actor+0x170,12);Tick();assert(TrialCoopInteractionSnapshot(owner).waiting);
        G<float>(actor+0x9c)=1000;W32(actor+0x170,0);
        for(unsigned i=0;i<8;++i){Tick();assert(!TrialCoopInteractionSnapshot(owner).waiting);}
        assert(playing&&R32(behavior+0x158)==3&&script_calls==1&&speech_starts==1&&g_base[actor+0x202]);
        // The same live owner must still wait at the real completion boundary.
        const auto partner=owner?p1:p2;G<float>(partner+0x40)=1000;playing=false;
        for(unsigned i=0;i<8;++i){Tick();assert(TrialCoopInteractionSnapshot(owner).waiting&&R32(behavior+0x158)==3);}
        G<float>(partner+0x40)=0;Tick();assert(!TrialCoopInteractionSnapshot(owner).waiting&&script_calls==2);
    }
}
static constexpr uint32_t other_behavior=0x610000,other_controller=0x611000,other_npc=0x612000;
static void OtherConversation(){
    std::memcpy(g_base+other_behavior,g_base+behavior,0x15c);W32(other_behavior+0x54,55);W32(other_behavior+0x50,77);
    std::memcpy(g_base+other_controller,g_base+controller,0x20);W32(other_controller+0xc,other_npc);
    std::memcpy(g_base+other_npc,g_base+npc,0x15c);W32(other_npc+0x54,66);
    W32(other_behavior+0xd0,other_controller);W32(other_behavior+0xd4,other_npc);
    W32(other_behavior+0x158,0);g_base[other_behavior+0x129]=0;
}
static void WaitingRecords(){
    for(unsigned invalid=0;invalid<5;++invalid){
        Seed();faces[1]=false;G<float>(p2+0x40)=1000;Tick();assert(TrialCoopInteractionSnapshot(0).waiting);
        OtherConversation();busy=true;
        if(invalid==0)W32(npc+0x5c,1);
        if(invalid==1)W32(behavior+0x5c,1);
        if(invalid==2)W32(controller+0xc,other_npc);
        if(invalid==3)W32(npc+0x54,99);
        if(invalid==4)W32(behavior+0x54,99);
        // The retired conversation never updates again; another real guest
        // update must stop aggregating its stale publication and reclaim it.
        for(unsigned i=0;i<8;++i)Tick(other_behavior);
        assert(!TrialCoopInteractionSnapshot(0).waiting&&!TrialCoopInteractionSnapshot(1).waiting);
        for(const auto& r:interactions::records)assert(r.behavior!=behavior);
        assert(!script_calls&&!speech_starts&&!g_base[p1+0x202]&&!g_base[p2+0x202]);
    }
    for(unsigned replacement=0;replacement<3;++replacement){
        Seed();faces[1]=false;Tick();assert(g_base[p1+0x202]&&g_base[behavior+0x129]);
        OtherConversation();busy=true;g_base[behavior+0x148]=1;g_base[p2+0x202]=1;
        if(replacement==0)W32(npc+0x5c,1); // Same behavior still owns the old lock.
        if(replacement==1)W32(behavior+0x54,99); // Reused object: no old unlock is authorized.
        if(replacement==2){
            W32(npc+0x5c,1);W32(other_behavior+0x158,3);g_base[other_behavior+0x129]=1;playing=true;
            auto* current=interactions::Find(g_base,other_behavior,true);assert(current);current->slot=0;
        }
        const auto scripts=script_calls,speech=speech_starts;Tick(other_behavior);
        assert(bool(g_base[p1+0x202])==(replacement!=0)&&g_base[p2+0x202]);
        assert(bool(g_base[behavior+0x129])==(replacement==1));
        if(replacement!=1)assert(!g_base[behavior+0x148]);
        assert(script_calls==scripts&&speech_starts==speech);
        if(replacement==2)assert(TrialCoopInteractionPlayer(g_base,other_behavior)==1&&g_base[other_behavior+0x129]);
    }
    Seed();faces[1]=false;Tick();OtherConversation();busy=true;W32(npc+0x5c,1);fail_lock=true;
    auto c=Context();c.ecx=other_behavior;const auto before=c;const auto csr=_mm_getcsr();
    std::array<uint8_t,4096> saved{};std::memcpy(saved.data(),g_base+stack-4096,saved.size());
    interactions::lookup_owner=p2;
    try{TrialCoopInteractionScope scoped(c,g_base);assert(false);}catch(const std::runtime_error&){}
    assert(!interactions::scope&&interactions::lookup_owner==p2&&!std::memcmp(&c,&before,sizeof c));
    assert(_mm_getcsr()==csr&&!std::memcmp(saved.data(),g_base+stack-4096,saved.size()));
    interactions::lookup_owner=0;fail_lock=false;Tick(other_behavior);assert(!g_base[p1+0x202]);
}
static void ScratchContracts(){
    // No guest call leaves capture inactive. Repeated/nested calls and a throw
    // after callback writes must still restore the first original stack image.
    for(unsigned mode=0;mode<5;++mode){
        Seed();for(uint32_t i=0;i<8192;++i)g_base[stack-8192+i]=uint8_t(i*73+19);
        auto c=Context();const auto before=c;const auto csr=_mm_getcsr();
        std::array<uint8_t,8192> saved;std::memcpy(saved.data(),g_base+stack-8192,saved.size());
        scratch_throw=mode==3;scratch_nested=mode==4;scratch_calls=0;
        try{
            interactions::HostRestore host;interactions::Call call(c,g_base);assert(!call.captured);
            if(mode)assert(call.Run(0xff001,proxy,{call.sp+128})==0xaabbccddu);
            if(mode==2)assert(call.Run(0xff001,proxy,{call.sp+128})==0xaabbccddu);
            assert(call.captured==(mode!=0)&&mode!=3);
        }catch(const std::runtime_error&){assert(mode==3);}
        assert(!std::memcmp(saved.data(),g_base+stack-8192,saved.size()));
        assert(!std::memcmp(&c,&before,sizeof c)&&_mm_getcsr()==csr);
        assert(scratch_calls==(mode==0?0u:mode==2||mode==4?2u:1u));
    }
    scratch_throw=scratch_nested=false;
    // Lock writes its arguments before Run. A Run-only capture loses these
    // original bytes; nonzero sentinels reject that tempting incomplete change.
    for(bool throwing:{false,true})for(bool value:{false,true}){
        Seed();for(uint32_t i=0;i<8192;++i)g_base[stack-8192+i]=uint8_t(i*73+19);
        auto c=Context();const auto before=c;const auto csr=_mm_getcsr();assert(interactions::Pair(g_base));
        auto* r=interactions::Find(g_base,behavior,true);assert(r);r->slot=0;
        std::array<uint8_t,8192> saved;std::memcpy(saved.data(),g_base+stack-8192,saved.size());
        fail_lock=throwing;
        try{interactions::HostRestore host;interactions::Call call(c,g_base);interactions::Lock(call,*r,value);assert(call.captured&&!throwing);}
        catch(const std::runtime_error&){assert(throwing);}
        assert(!std::memcmp(saved.data(),g_base+stack-8192,saved.size()));
        assert(!std::memcmp(&c,&before,sizeof c)&&_mm_getcsr()==csr);
        assert(bool(g_base[p1+0x202])==(!throwing&&value));
    }
}
static void WeaponTick(uint32_t actor){
    auto c=Context();c.ecx=actor;
    if(candidate)CoopWeaponDispatch(c,g_base);else OriginalWeaponDispatch(c,g_base);
    if(c.esp!=stack+4||c.esi!=0x1234||c.edi!=0x9876||c.ebx!=0x7777||c.ebp!=0x8888)
        std::fprintf(stderr,"weapon tick %u actor %x phase %u/%u esp %x esi %x edi %x ebx %x ebp %x\n",weapon_checks,actor,R32(actor+0x170),R32(actor+0x174),c.esp,c.esi,c.edi,c.ebx,c.ebp);
    assert(c.esp==stack+4&&c.esi==0x1234&&c.edi==0x9876&&c.ebx==0x7777&&c.ebp==0x8888);++weapon_checks;
}
static void StartWeapons(unsigned owner){Seed();faces[1-owner]=false;Tick();assert(g_base[behavior+0x129]);}
static void EndWeapons(){auto c=Context();CoopConversationEnd(c,g_base);assert(!g_base[behavior+0x129]);W32(p1+0x4ac,0);W32(p2+0x4ac,0);}
static void WeaponRestrictions(){
    // Original local co-op protects only the initiator. The second actor reaches
    // the real native weapon gates with fire and switch still admitted.
    StartWeapons(0);candidate=false;WeaponTick(p2);assert(!lowered&&attack_sinks[0]&&attack_sinks[1]&&attack_sinks[2]);
    for(unsigned owner=0;owner<2;++owner)for(unsigned selected:{0u,1u,8u,17u}){
        StartWeapons(owner);for(const auto p:{p1,p2})W32(p+0x4fc,selected);
        WeaponTick(p1);WeaponTick(p2);assert(lowered==2&&!raised);
        for(const auto p:{p1,p2})assert(g_base[p+0x2ac]&&!g_base[p+0x2ad]&&!g_base[p+0x2ae]&&R32(p+0x170)==0&&R32(p+0x4fc)==selected);
        manual_holster=true;
        for(unsigned frame=0;frame<4;++frame){WeaponTick(p1);WeaponTick(p2);}
        assert(lowered==2&&!raised);for(auto count:attack_sinks)assert(!count);
        assert(g_base[(owner?p2:p1)+0x202]&&!g_base[(owner?p1:p2)+0x202]);
        assert(R32(p1+0x4ac)==1&&R32(p2+0x4ac)==1); // Movement sees original lock and input.
        manual_holster=false;EndWeapons();WeaponTick(p1);WeaponTick(p2);
        assert(raised==2);for(const auto p:{p1,p2})assert(!g_base[p+0x2ac]&&!g_base[p+0x2ad]&&!g_base[p+0x2ae]&&R32(p+0x4fc)==selected&&G<float>(p+0x9c)==1000);
    }
    // A locked conversation is required; admission waits and deliberately
    // unlocked conversations must not acquire a companion weapon restriction.
    Seed();g_base[behavior+0x119]=0;Tick();WeaponTick(p1);WeaponTick(p2);assert(!lowered&&!TrialCoopInteractionWeaponsBlocked(g_base,p1));
    Seed();G<float>(p2+0x40)=1000;Tick();WeaponTick(p1);WeaponTick(p2);assert(!lowered&&!TrialCoopInteractionWeaponsBlocked(g_base,p2));
    StartWeapons(0);trial_coop::SetActive(false);WeaponTick(p2);assert(!lowered); // Native single-player path.
    // Both in-progress native animation directions finish normally. Ending a
    // talk during holster defers the owned raise until the next settled tick.
    for(unsigned slot=0;slot<2;++slot){
        const auto p=slot?p2:p1;
        StartWeapons(1-slot);weapon_done=false;WeaponTick(p);assert(g_base[p+0x2ae]&&R32(p+0x170)==8);
        EndWeapons();WeaponTick(p);assert(!raised&&g_base[p+0x2ae]);
        weapon_done=true;WeaponTick(p);assert(!raised&&!g_base[p+0x2ae]);WeaponTick(p);assert(raised==1&&!g_base[p+0x2ac]&&!g_base[p+0x2ad]);
        StartWeapons(1-slot);W32(p+0x170,8);W32(p+0x174,2);g_base[p+0x2ac]=g_base[p+0x2ad]=1;weapon_done=false;
        WeaponTick(p);assert(!lowered&&g_base[p+0x2ad]);weapon_done=true;WeaponTick(p);
        assert(!lowered&&!g_base[p+0x2ad]&&!g_base[p+0x2ac]);WeaponTick(p);assert(lowered==1&&g_base[p+0x2ac]&&!g_base[p+0x2ae]);
    }
    // Restore only the weapon this scope lowered, after unrelated/native locks
    // release. Already-holstered, changed, dead or replaced actors stay owned
    // by their original lifecycle rather than a stale conversation record.
    for(unsigned mode=0;mode<8;++mode){
        StartWeapons(0);if(mode==0)g_base[p2+0x2ac]=1;
        WeaponTick(p2);assert(lowered==unsigned(mode!=0));EndWeapons();
        if(mode==1)g_base[p2+0x202]=1;
        if(mode==2)g_base[p2+0x848]=1;
        if(mode==3)W32(p2+0x4fc,5);
        if(mode==4){G<float>(p2+0x9c)=0;W32(p2+0x170,12);}
        if(mode==5)W32(p2+0x54,99);
        if(mode==6){std::memcpy(g_base+test_world+0x100,g_base+test_world,0x38);W32(0x51c4dc,test_world+0x100);}
        if(mode==7)TrialCoopInteractionReset();
        WeaponTick(p2);assert(!raised);
        if(mode==1||mode==2){g_base[p2+0x202]=g_base[p2+0x848]=0;WeaponTick(p2);assert(raised==1);}
    }
    // Waiting/revive recovery, abort and an invalid NPC all drop the shared
    // restriction. Another live locked interaction retains it until its end.
    for(unsigned mode=0;mode<5;++mode){
        StartWeapons(0);WeaponTick(p2);
        if(mode==0){Tick();Tick();playing=false;G<float>(p2+0x40)=1000;Tick();}
        if(mode==1){G<float>(p1+0x9c)=0;W32(p1+0x170,12);Tick();}
        if(mode==2)EndWeapons();
        if(mode==3)W32(npc+0x5c,1);
        if(mode==4){OtherConversation();auto* other=interactions::Find(g_base,other_behavior,true);assert(other);other->slot=1;g_base[other_behavior+0x129]=1;EndWeapons();}
        assert(TrialCoopInteractionWeaponsBlocked(g_base,p2)==(mode==4));WeaponTick(p2);assert(raised==unsigned(mode!=4));
    }
    for(bool throwing:{false,true})for(uint8_t prior_lock:{uint8_t(0),uint8_t(1)}){
        StartWeapons(0);g_base[p2+0x202]=prior_lock;W32(p2+0x4ac,0xa7);g_base[p2+0x2ac]=1;
        fail_weapon=throwing;nested_weapon=!throwing;auto c=Context();c.ecx=p2;
        const auto csr=_mm_getcsr();try{CoopWeaponDispatch(c,g_base);assert(!throwing);}catch(const std::runtime_error&){assert(throwing);}
        assert(g_base[p2+0x202]==prior_lock&&R32(p2+0x4ac)==0xa7&&!TrialCoopInteractionWeaponInputBlocked(p2)&&_mm_getcsr()==csr);
    }
    // The original kind-2 interaction exception remains available inside the
    // scoped native lock, while firearm/shield/switch action gates reject it.
    StartWeapons(0);auto c=Context();c.ecx=p2;{TrialCoopInteractionWeaponScope scope(c,g_base);auto gate=c;gate.esi=p2;gate.edi=2;
        assert(!NativeAttackGuard(gate,g_base)&&!NativeShieldAndAlternateGuard(gate,g_base)&&!NativeSwitchGuard(gate,g_base)&&NativeInteractionGuard(gate,g_base));}
}
int main(int argc,char** argv){
    if(argc==2){
        if(!std::strcmp(argv[1],"--waiting-admission"))WaitingAdmission();
        else if(!std::strcmp(argv[1],"--waiting-revival"))WaitingRevival();
        else if(!std::strcmp(argv[1],"--waiting-records"))WaitingRecords();
        else assert(false);
        std::puts("PASS selected original-guest waiting lifecycle regression");return 0;
    }
    assert(argc==1);CallBoundary();Ownership();RendezvousRecovery();Admission();SkipAndTurn();LifecycleAndSafety();
    WaitingAdmission();WaitingRevival();WaitingRecords();
    ScratchContracts();WeaponRestrictions();
    std::printf("PASS %u actual weapon-dispatch ticks; native lower/raise animations, both-player weapon gates, scoped control/fire restoration, manual-toggle denial, lifecycle and ownership recovery\n",weapon_checks);
    std::puts("PASS production CallGuest return/restore ABI and stale EAX isolation, original NPC range/facing and conversation lifecycle, either actor initiation, original per-actor lock/unlock, independent input/skip/turn target, partner-free movement with deferred script commitment, owner released to revive/retrieve a missing partner without repeated speech, current-update waiting after busy/condition/revival branches, stale record retirement and owned-lock/replacement guards, native single-player fallback, abort/down/revive/reset, context/stack/MXCSR guards, lazy scratch capture with pre-call arguments and nested/throwing callbacks; live dialogue visuals pending");
}
