// Retail logical-player mapping and action-query execution; devices are supplied boundaries.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "cpu/x86_ops.h"
#include "kernel/kernel.h"
namespace kernel {static std::array<uint8_t,0x800000> memory{};uint8_t* g_base=memory.data();}
namespace mem {bool IsCommitted(uint32_t address){return address>=0x10000&&address<kernel::memory.size();}}
#include "xbe_coop_guest_input.h"
using namespace kernel;
static constexpr uint32_t owner=0x600000,manager=0x601000,states=0x602000,platform=0x603000,stack=0x700000;
static bool connected[4]={true,true,false,false};
static constexpr uint32_t world=0x604000,world_table=0x605000,pause_menu=0x606000,widget=0x607000;
static constexpr uint32_t input_query=0x12345000,widget_delete=0x12345010;
static unsigned query_slots[2]{},menu_creations=0;
static bool expect_owned_menu=false;
[[noreturn]] void x86_trap(X86Context&,uint32_t,const char*){std::abort();}
X86_EXTERN_FUNC(sub_000567F0);
void x86_icall(X86Context& ctx,uint8_t* base,uint32_t target){
    if(target==input_query){
        assert(ctx.ecx==world);const auto slot=R32(ctx.esp+4);assert(slot<2);++query_slots[slot];
        ctx.ecx=manager;sub_000567F0(ctx,base);return;
    }
    if(target==widget_delete){assert(ctx.ecx==widget);ctx.esp+=8;return;}
    std::abort();
}
uint32_t kernel::CallGuest(X86Context& c,uint32_t fn,const uint32_t* args,size_t n){
    for(size_t i=n;i;--i){c.esp-=4;W32(c.esp,args[i-1]);}c.esp-=4;W32(c.esp,0x11223344);
    x86_icall(c,g_base,fn);return c.eax;
}
X86_FUNC(sub_001F9010){x87_push(ctx,1.0/60);ctx.esp+=4;}
X86_FUNC(sub_00272A60){
    assert(R32(ctx.esp+4)==0x267620);++menu_creations;
    if(expect_owned_menu)assert(trial_coop::PauseOwner()>=0);
    ctx.eax=widget;ctx.esp+=4;
}
X86_FUNC(sub_00258440){ctx.esp+=4;}
X86_FUNC(sub_0022C2C0){
    const unsigned port=R32(ctx.esp+4);assert(port<4&&ctx.ecx==platform);
    ctx.eax=connected[port];ctx.esp+=8;
}
// RETAIL_INPUT_FUNCTIONS

static void Seed(){
    memory.fill(0);trial_coop::guest_input_mapping={};trial_coop::SetActive(false);
    trial_coop::ResetPauseOwner();trial_coop::pause_world=0;
    connected[0]=connected[1]=true;connected[2]=connected[3]=false;
    W32(0x5096e0,owner);W32(manager+0x20,states);
    const uint8_t record[]={0,1,2,3,0,255,255,255,1,0,0,0,1};
    std::memcpy(g_base+owner+0xc,record,sizeof(record));
    G<float>(states)=.25f;G<float>(states+0xd0)=-.75f;
    // RETAIL_INPUT_CONSTANTS
    W32(0x51c4dc,world);W32(world,world_table);W32(world_table+8,input_query);
    W32(pause_menu,0x487818);W32(pause_menu+4,0x267620);g_base[pause_menu+0xc]=1;
    W32(widget,world_table+0x80);W32(world_table+0x80,widget_delete);
    W32(0x509a70,0x450574);W32(0x509a74,1);
    query_slots[0]=query_slots[1]=menu_creations=0;expect_owned_menu=false;
}
static void Map(bool candidate){
    X86Context ctx{};ctx.esp=stack;ctx.edi=platform;ctx.ebx=0x12345678;ctx.esi=0xabcdef01;
    W32(stack,0x5d17d);
    if(candidate)CandidateMapping(ctx,g_base);else OriginalMapping(ctx,g_base);
    assert(ctx.esp==stack+4&&ctx.edi==platform&&ctx.ebx==0x12345678&&ctx.esi==0xabcdef01);
}
static float Query(unsigned player){
    X86Context ctx{};ctx.esp=stack;ctx.ecx=manager;W32(stack,0xbefb9);
    W32(stack+4,player);W32(stack+8,0x12);W32(stack+12,3);
    sub_000567F0(ctx,g_base);assert(ctx.esp==stack+16);return float(X87_ST(0));
}
static void Button(unsigned player,unsigned button,bool down){
    X86Context ctx{};ctx.ecx=manager;ctx.esp=stack;W32(stack+4,states+player*0xd0);W32(stack+8,button);
    sub_00055CD0(ctx,g_base);assert(ctx.eax);G<float>(ctx.eax+4)=down?1.f:0.f;
}
static void Frontend(bool candidate=true){
    X86Context c{};c.ecx=0x509a70;c.esp=stack;c.ebx=123;c.esi=456;c.edi=789;
    const unsigned csr=_mm_getcsr();if(candidate)CandidateFrontend(c,g_base);else OriginalFrontend(c,g_base);
    assert(c.esp==stack+4&&c.ebx==123&&c.esi==456&&c.edi==789&&(_mm_getcsr()&~63u)==(csr&~63u));
}
static bool Consume(unsigned action=3,bool candidate=true){
    X86Context c{};c.ecx=0x509a70;c.esp=stack;W32(stack+4,action);
    if(candidate)CandidateConsume(c,g_base);else OriginalConsume(c,g_base);
    assert(c.esp==stack+8);return (c.eax&255)!=0;
}
static void Menu(bool enter,bool candidate=true){
    X86Context c{};c.ecx=pause_menu;c.esp=stack;
    if(enter){if(candidate)CandidateEnter(c,g_base);else OriginalEnter(c,g_base);}
    else{if(candidate)CandidateExit(c,g_base);else OriginalExit(c,g_base);}
    assert(c.esp==stack+4);
}
static void PauseTests(){
    // Inactive co-op must preserve the original menu edge, pause state and guest bytes.
    Seed();Map(false);Frontend(false);Button(0,0x1d,true);Frontend(false);
    assert(Consume(3,false));Menu(true,false);Menu(false,false);
    static const auto original_sp_pause=memory;
    Seed();Map(true);Frontend();Button(0,0x1d,true);Frontend();
    assert(Consume());Menu(true);Menu(false);assert(memory==original_sp_pause);
    // The native frontend uses rising strength edges, not held/last-device input.
    Seed();Map(true);Frontend();Button(0,0x1d,true);Frontend();assert(Consume());
    assert(!Consume());Menu(true);assert(R32(world+0x10)&0x800);assert(trial_coop::PauseOwner()==-1);
    Menu(false);assert(!(R32(world+0x10)&0x800));
    // Original negative witness: P2 Start is never polled by the original frontend.
    Seed();trial_coop::SetActive(true);Map(true);Frontend(false);Button(1,0x1d,true);Frontend(false);
    assert(!Consume(3,false)&&!query_slots[1]);
    for(unsigned owner_slot=0;owner_slot<2;++owner_slot){
        Seed();trial_coop::SetActive(true);Map(true);Frontend();
        Button(owner_slot,0x1d,true);Frontend();assert(Consume());assert(trial_coop::PauseOwner()==-1);
        expect_owned_menu=true;Menu(true);assert(trial_coop::PauseOwner()==int(owner_slot)&&menu_creations==1);
        assert(R32(world+0x10)&0x800);Menu(true);assert(menu_creations==1); // no re-entry steal
        Button(owner_slot,0x1d,false);Button(owner_slot^1,0x1d,true);
        Frontend();assert(!Consume());assert(trial_coop::PauseOwner()==int(owner_slot));
        // All native frontend channels now use only the accepted owner's slot.
        Button(owner_slot^1,0x16,true);Frontend();assert(!Consume(0));
        Button(owner_slot,0x16,true);Frontend();assert(Consume(0));
        Menu(false);assert(trial_coop::PauseOwner()==-1&&!(R32(world+0x10)&0x800));
    }
    Seed();trial_coop::SetActive(true);Map(true);Frontend();
    Button(1,0x1d,true);Frontend(); // Unconsumed action cannot transfer ownership.
    expect_owned_menu=true;Menu(true);assert(trial_coop::PauseOwner()==0);Menu(false);
    Seed();trial_coop::SetActive(true);Map(true);Frontend();
    Button(0,0x1d,true);Button(1,0x1d,true);Frontend();assert(Consume());Menu(true);
    assert(trial_coop::PauseOwner()==0); // simultaneous native press deterministically P1
    Menu(false);Frontend();assert(!Consume()); // held Start is blocked by host on real handoff
    trial_coop::pause_state.Sample(false,false);trial_coop::pause_state.Sample(false,true);
    trial_coop::pause_state.Consume(true);trial_coop::pause_state.Enter();
    W32(0x51c4dc,world+0x100);trial_coop::UpdateGuestInputMapping(g_base);assert(trial_coop::PauseOwner()==-1);
    trial_coop::pause_state.Enter();trial_coop::SetActive(false);Map(true);assert(trial_coop::pause_state.owner==-1);
    // The native pause state controls the whole simulation, not a local overlay.
    Seed();trial_coop::SetActive(true);Map(true);W32(pause_menu+4,0x267b40);Menu(false);
    assert(trial_coop::PauseOwner()==-1);
}
int main(){
    PauseTests();
    for(int port=-2;port<6;++port){
        assert(trial_coop::PhysicalPort(port,false)==(port>=0&&port<4?port:-1));
        assert(trial_coop::PhysicalPort(port,true)==(port==1?0:-1));
    }
    Seed();Map(false);static auto original_mapping=memory;
    assert(Query(0)==.25f&&Query(1)==0); // Original locked profile excludes player 2.
    static auto original_queries=memory;
    Seed();Map(true);assert(memory==original_mapping); // Compare identical retail execution phases, including stack writes.
    assert(Query(0)==.25f&&Query(1)==0);assert(memory==original_queries);
    std::array<uint8_t,13> before{};std::memcpy(before.data(),g_base+owner+0xc,before.size());
    trial_coop::SetActive(true);Map(true);assert(Query(0)==.25f&&Query(1)==-.75f);
    G<float>(states)=.5f;assert(Query(0)==.5f&&Query(1)==-.75f);
    connected[1]=false;Map(true);assert(Query(1)==0&&Query(0)==.5f);
    connected[1]=true;Map(true);assert(Query(1)==-.75f);
    trial_coop::SetActive(false);Map(true);
    assert(!std::memcmp(before.data(),g_base+owner+0xc,before.size())&&Query(1)==0);
    // Other logical slots are never claimed, and original remapping changes win at exit.
    Seed();trial_coop::SetActive(true);Map(true);g_base[owner+0xc]=3;
    g_base[owner+0xe]=2;g_base[owner+0x12]=2;g_base[owner+0x16]=1;
    trial_coop::SetActive(false);trial_coop::UpdateGuestInputMapping(g_base);
    assert(g_base[owner+0xc]==3&&g_base[owner+0xe]==2&&g_base[owner+0x12]==2&&g_base[owner+0x16]==1);
    // A replaced/deleted session must not restore bytes into the prior owner.
    Seed();trial_coop::SetActive(true);Map(true);std::memcpy(before.data(),g_base+owner+0xc,before.size());
    W32(0x5096e0,owner+0x100);trial_coop::SetActive(false);trial_coop::UpdateGuestInputMapping(g_base);
    assert(!std::memcmp(before.data(),g_base+owner+0xc,before.size())&&!trial_coop::guest_input_mapping.owner);
    W32(0x5096e0,0xfffffff0);trial_coop::SetActive(true);trial_coop::UpdateGuestInputMapping(g_base);
    assert(!trial_coop::guest_input_mapping.owner);
    trial_coop::Reset();
    std::puts("Co-op input: retail slot admission, distinct axes, disconnect/reconnect, inactive parity, restore and bounds passed.");
}
