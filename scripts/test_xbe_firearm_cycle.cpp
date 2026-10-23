// Actual retail input consumer, category selector, scan, and ammunition checks.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include "kernel/kernel.h"
namespace kernel {static std::array<uint8_t,0x800000> memory{};uint8_t* g_base=memory.data();}
namespace mem {bool IsCommitted(uint32_t a){return a>=0x10000&&a<kernel::memory.size();}}
#include "xbe_firearm_cycle.h"
using namespace kernel;
static constexpr uint32_t player=0x600000,inventory=player+0x498,items=0x610000,
    descriptors=0x620000,order=0x630000,reserves=0x640000,stack=0x700000;
static constexpr uint32_t ids[]={17,2,5,8}; // Checkpoint order is not item-ID order.
static int direction=0;
static unsigned pressed_category=0,publications=0,transitions=0;
static bool press=true,fail_publish=false;
int TrialFirearmDirection(){return direction;}
[[noreturn]] void x86_trap(X86Context&,uint32_t,const char*){std::abort();}
void x86_icall(X86Context& ctx,uint8_t*,uint32_t target){
    assert(target==0xF0001&&ctx.ecx==0x650000&&R32(ctx.esp+12)==1);
    const unsigned action=R32(ctx.esp+8);assert(action>=4&&action<=6);
    x87_push(ctx,press&&action==4+pressed_category?1.:0.);ctx.esp+=16;
}
// Descriptor lookup/property and downstream notification/state animation are
// supplied boundaries; ownership, selection and ammo eligibility run retail code.
X86_FUNC(sub_000442D0){const uint32_t id=R32(ctx.esp+4);assert(id<27);ctx.eax=descriptors+id*0x100;ctx.esp+=4;}
X86_FUNC(sub_00044160){ctx.eax=R32(ctx.ecx+0x24);ctx.esp+=4;}
X86_FUNC(sub_00044210){ctx.eax=R32(ctx.ecx+0x20);ctx.esp+=4;}
X86_FUNC(sub_000421B0){assert(ctx.ecx==inventory);++publications;if(fail_publish)throw std::runtime_error("guest unwind");ctx.esp+=4;}
X86_FUNC(sub_000BFE00){assert(ctx.ecx==player&&R32(ctx.esp+4)==8&&R32(ctx.esp+8)==0);++transitions;ctx.esp+=12;}
X86_FUNC(sub_000BC580){assert(ctx.ecx==player);ctx.eax=1;ctx.esp+=4;}
X86_FUNC(sub_000BE210){assert(ctx.ecx==player);ctx.eax=descriptors+R32(player+0x4fc)*0x100;ctx.esp+=4;}
X86_FUNC(sub_000D23E0){assert(ctx.ecx==player);++transitions;ctx.esp+=4;}
X86_FUNC(sub_000BC560){assert(ctx.ecx==player);ctx.esp+=4;}
// RETAIL_FUNCTIONS

static uint32_t item(unsigned id){return items+id*0x5c;}
static void reset(unsigned current=0,unsigned count=4,unsigned category=0){
    memory.fill(0);direction=0;pressed_category=category;press=true;fail_publish=false;
    publications=transitions=0;assert(!trial_firearm_cycle::inventory);
    W32(inventory,items);W32(inventory+0x64,ids[current]);W32(inventory+0x68,ids[current]);W32(inventory+0x6c,ids[current]);
    for(unsigned c=0;c<3;++c)W32(inventory+0x70+c*4,0xffffffff);
    W32(inventory+0x70+category*4,ids[current]);
    W32(inventory+0x80+category*16,count?order:0);W32(inventory+0x84+category*16,count?order+count*4:0);
    for(unsigned i=0;i<4;++i){
        const unsigned id=ids[i];W32(order+i*4,id);G<uint8_t>(item(id))=1;W32(item(id)+4,id);
        W32(item(id)+0xc,3);W32(item(id)+0x10,reserves+id*4);
        W32(descriptors+id*0x100+8,category);W32(descriptors+id*0x100+0x20,30);W32(descriptors+id*0x100+0x24,1);
    }
    W32(0x51c4dc,0x650000);W32(0x650000,0x650100);W32(0x650108,0xF0001);
    W32(inventory+0xf8,99);G<float>(inventory+0xfc)=123;
}
static X86Context context(uint32_t receiver,uint32_t caller=0xC4E03,unsigned category=0){
    X86Context ctx{};ctx.ecx=receiver;ctx.esp=stack;ctx.ebx=0x11111111;ctx.ebp=0x22222222;
    ctx.esi=0x33333333;ctx.edi=0x44444444;W32(stack,caller);W32(stack+4,category);return ctx;
}
static void preserved(const X86Context& ctx,uint32_t end){
    assert(ctx.esp==end&&ctx.ebx==0x11111111&&ctx.ebp==0x22222222&&ctx.esi==0x33333333&&ctx.edi==0x44444444);
    assert(!trial_firearm_cycle::inventory);
}
static unsigned cycle(int dir,bool consumer=true,uint32_t caller=0xC4E03,unsigned category=0){
    direction=dir;auto ctx=context(consumer?player:inventory,caller,category);
    if(consumer)sub_000C4CB0(ctx,g_base);else sub_00042840(ctx,g_base);
    preserved(ctx,stack+(consumer?4:8));return R32(inventory+0x68);
}
int main(){
    unsigned cases=0;
    for(int dir:{-1,1})for(unsigned current=0;current<4;++current){
        reset(current);assert(cycle(dir)==ids[(current+4+dir)%4]);
        assert(R32(inventory+0xf8)==0&&G<float>(inventory+0xfc)==0);++cases;
    }
    // Original eligibility independently admits loaded, reserve, and extra ammo.
    for(unsigned kind=0;kind<3;++kind){
        reset();G<uint8_t>(item(ids[3]))=0;W32(item(ids[2])+0xc,0);
        if(kind==0)W32(item(ids[2])+0xc,1);
        if(kind==1)W32(reserves+ids[2]*4,1);
        if(kind==2)W32(item(ids[2])+0x18,1);
        assert(cycle(-1)==ids[2]);++cases;
    }
    reset();G<uint8_t>(item(ids[3]))=0;W32(item(ids[2])+0xc,0);assert(cycle(-1)==ids[1]);++cases;
    for(unsigned count:{0u,1u})for(int dir:{-1,1}){
        reset(0,count);assert(cycle(dir,false)==ids[0]);assert(!trial_firearm_cycle::inventory);++cases;
    }
    for(int dir:{-1,1}){
        reset();for(unsigned id:ids){G<uint8_t>(item(id))=0;W32(item(id)+0xc,0);}
        assert(cycle(dir,false)==ids[0]&&publications==0);++cases;
    }
    // Entering firearms first recalls their last selected slot in both directions.
    for(int dir:{-1,1}){
        reset(0);W32(inventory+0x68,ids[1]);assert(cycle(dir,false)==ids[0]);++cases;
    }
    // Remembered empty slot recurses into the same retail scan, in reverse.
    reset();W32(inventory+0x68,ids[1]);W32(item(ids[0])+0xc,0);
    assert(cycle(-1,false)==ids[3]);++cases;
    // The other explicit caller qualifies too; lowered same-category input retains retail gating.
    reset();assert(cycle(-1,false,0xC4DCD)==ids[3]);++cases;
    reset();G<uint8_t>(player+0x2ac)=1;assert(cycle(-1)==ids[0]);++cases;
    // Controller/nonlocal player, other categories and unrelated callers retain forward order.
    reset();W32(player+0x368,1);assert(cycle(-1,false)==ids[1]);++cases;
    for(unsigned category:{1u,2u}){reset(0,4,category);assert(cycle(-1,false,0xC4E03,category)==ids[1]);++cases;}
    reset();assert(cycle(-1,false,0x12345678)==ids[1]);++cases;
    // Whole automatic selection is bit-identical with Previous latched but no explicit wrapper.
    reset();direction=1;auto auto_ctx=context(inventory);sub_00042870(auto_ctx,g_base);preserved(auto_ctx,stack+4);
    static auto expected=memory;const auto expected_ctx=auto_ctx;
    reset();direction=-1;auto_ctx=context(inventory);sub_00042870(auto_ctx,g_base);preserved(auto_ctx,stack+4);
    assert(memory==expected&&!std::memcmp(&auto_ctx,&expected_ctx,sizeof auto_ctx));++cases;
    // A guest exception cannot leave the reverse scope active for later automatic selection.
    reset();direction=-1;fail_publish=true;auto ctx=context(inventory);
    try{sub_00042840(ctx,g_base);assert(false);}catch(const std::runtime_error&){}
    assert(!trial_firearm_cycle::inventory);++cases;
    // Pending selection after a checkpoint keeps its authored non-numeric ordering.
    reset(3);assert(cycle(-1)==ids[2]);W32(inventory+0x64,ids[2]);assert(cycle(-1)==ids[1]);++cases;
    std::printf("firearm cycle: %u retail input/scan cases; reverse/wrap/checkpoint order, ownership/ammo, zero/one, recursion, controller/category/automatic parity and unwind passed\n",cases);
}
