// Retail pair filtering and overlap dispatch; only shape queries and final callbacks are supplied.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <utility>
#include <vector>
#include "kernel/kernel.h"
#include "cpu/x86_ops.h"
namespace kernel {static std::array<uint8_t,0x800000> memory{};uint8_t* g_base=memory.data();}
using namespace kernel;
static constexpr uint32_t bodies=0x600000,stride=0x10000,shape_vtable=0x640000,
    owner_vtable=0x641000,filter=0x642000,listeners=0x643000,listener=0x644000,
    listener_vtable=0x645000,pairs=0x646000,output=0x647000,stack=0x700000;
struct Body {
    unsigned type=1,shape_type=24,shape_user=0,layers=0x02F00000;
    bool shape=true;
    std::vector<std::pair<unsigned,unsigned>> properties;
};
static std::array<Body,2> specs;
static std::vector<std::array<unsigned,3>> events;
static bool baseline=false;
[[noreturn]] void x86_trap(X86Context&,uint32_t,const char*){std::abort();}
X86_FUNC(sub_001F7F00){assert(false);}
// RETAIL_FUNCTIONS

static uint32_t body(unsigned i){return bodies+i*stride;}
static unsigned index(uint32_t a){assert(a==body(0)||a==body(1));return a==body(1);}
void x86_icall(X86Context& ctx,uint8_t* base,uint32_t target){
    if(target==0xF1000008){ctx.eax=specs[index(ctx.ecx-0x100)].shape_type;ctx.esp+=4;return;}
    if(target==0x2A8B60){assert(ctx.ecx==filter);if(baseline)original_filter(ctx,base);else sub_002A8B60(ctx,base);return;}
    if(target==0x2FBB00||target==0x2FBB60){
        assert(ctx.ecx==listener);if(target==0x2FBB00)sub_002FBB00(ctx,base);else sub_002FBB60(ctx,base);return;
    }
    assert(target==0x300650||target==0x3005E0);
    const unsigned a=index(ctx.ecx-0x200),b=index(R32(ctx.esp+4));assert(specs[a].type==2);
    events.push_back({a,b,target==0x300650});ctx.esp+=8;
}
static void setup(const Body& a,const Body& b){
    std::memset(g_base+bodies,0,0x50000);specs={a,b};events.clear();baseline=false;
    for(unsigned i=0;i<2;++i){
        const auto& s=specs[i];const auto c=body(i),owner=c+0x200,props=c+0x1000;
        W32(c,s.shape?c+0x100:0);W32(c+0x10,0x200);G<uint8_t>(c+0x18)=s.type;
        G<uint8_t>(c+0x19)=uint8_t(-20);W32(c+0x1c,s.layers);
        W32(c+0x100,shape_vtable);W32(c+0x108,s.shape_user);W32(owner,owner_vtable);
        W32(owner+0x44,props);W32(owner+0x48,s.properties.size());
        for(unsigned n=0;n<s.properties.size();++n){W32(props+n*16,s.properties[n].first);W32(props+n*16+8,s.properties[n].second);}
    }
    W32(shape_vtable+8,0xF1000008);W32(owner_vtable+0x18,0x300650);W32(owner_vtable+0x20,0x3005E0);
    W32(filter,filter+0x100);W32(filter+0x100,0x2A8B60);
    W32(listeners+0x28,listener);W32(listeners+0x44,listener);W32(listeners+0x48,listener);
    W32(listener,listener_vtable);W32(listener_vtable+8,0x2FBB00);W32(listener_vtable+12,0x2FBB60);
}
static X86Context context(){
    X86Context ctx{};ctx.esp=stack;ctx.ebx=0x12345678;ctx.esi=0x23456789;ctx.edi=0x34567890;ctx.ebp=0x456789ab;return ctx;
}
static void preserved(const X86Context& ctx,unsigned pop){
    assert(ctx.esp==stack+pop&&ctx.ebx==0x12345678&&ctx.esi==0x23456789&&ctx.edi==0x34567890&&ctx.ebp==0x456789ab);
}
static bool accepted(bool original=false,bool reverse=false){
    auto ctx=context();ctx.ecx=filter;W32(stack+4,output);W32(stack+8,body(reverse));W32(stack+12,body(!reverse));
    if(original)original_filter(ctx,g_base);else sub_002A8B60(ctx,g_base);
    preserved(ctx,16);assert(ctx.eax==output&&G<uint8_t>(output)<=1);return G<uint8_t>(output)!=0;
}
static void dispatch(bool removing,bool reverse=false,bool original=false){
    baseline=original;auto ctx=context();ctx.ecx=listeners;W32(pairs,body(reverse)+0x14);W32(pairs+4,body(!reverse)+0x14);
    W32(stack+4,pairs);W32(stack+8,1);W32(stack+12,filter);
    if(removing)sub_003190C0(ctx,g_base);else sub_00319050(ctx,g_base);
    preserved(ctx,removing?12:16);
}
static Body character(){Body b;b.type=2;b.properties={{0x29,1},{0x2a,1}};return b;}
static Body ragdoll(){Body b;b.properties={{1,4}};return b;}
int main(){
    unsigned checks=0;
    for(bool reverse:{false,true}){
        setup(character(),ragdoll());assert(!accepted(true,reverse)&&accepted(false,reverse));++checks;
        dispatch(false,reverse,true);assert(events.empty());
        dispatch(false,reverse);assert((events==std::vector<std::array<unsigned,3>>{{0,1,1}}));
        dispatch(true,reverse);assert((events==std::vector<std::array<unsigned,3>>{{0,1,1},{0,1,0}}));++checks;
    }
    // Every non-ragdoll kind preserves the original gate across authored exclusions.
    for(unsigned kind=0;kind<23;++kind)for(unsigned mask=0;mask<16;++mask)for(bool reverse:{false,true}){
        auto a=character(),b=ragdoll();b.properties[0].second=kind;
        if(mask&1)b.properties.push_back({0x19,0});
        if(mask&2)b.properties.push_back({0x1a,0});
        if(mask&4)b.properties.push_back({0x1b,0});
        if(mask&8)b.properties.push_back({0x18,0xffff});
        setup(a,b);const bool old=accepted(true,reverse),now=accepted(false,reverse);
        if(kind==4)assert(!old&&now==!(mask&11));else assert(old==now);
        ++checks;
    }
    // Shape presence, entity restrictions, collision layers and shape masks still apply.
    for(unsigned which=0;which<5;++which)for(bool reverse:{false,true}){
        auto a=character(),b=ragdoll();a.shape_type=b.shape_type=0;a.layers=0x02F00001;b.layers=0x02F00002;
        if(which==0)b.shape=false;
        if(which==1)b.properties.push_back({0x18,1});
        if(which==2)b.layers=a.layers;
        if(which==3){a.layers=0x04000001;b.layers=0x08000002;}
        if(which==4){a.shape_user=1;b.shape_user=2;}
        setup(a,b);assert(!accepted(false,reverse));dispatch(false,reverse);assert(events.empty());++checks;
    }
    // Non-character phantoms and character-character pairs retain retail behavior.
    for(unsigned flag:{0u,2u})for(bool reverse:{false,true}){
        auto a=character();a.properties[0].second=flag;setup(a,ragdoll());assert(accepted(false,reverse)==accepted(true,reverse));++checks;
        setup(character(),a);assert(accepted(false,reverse)==accepted(true,reverse));++checks;
    }
    // Existing per-ragdoll adjacent-bone exclusions remain unchanged.
    for(bool same:{false,true})for(bool reverse:{false,true}){
        auto a=ragdoll(),b=ragdoll();a.properties={{1,4},{0x2d,7},{3,2},{4,1},{5,3}};
        b.properties={{1,4},{0x2d,same?7u:8u},{3,3},{4,1},{5,2}};
        setup(a,b);assert(accepted(false,reverse)==!same&&accepted(true,reverse)==!same);++checks;
    }
    // Removal must always dispatch, even if the contact became excluded afterward.
    auto excluded=ragdoll();excluded.properties.push_back({0x19,0});setup(character(),excluded);
    dispatch(true);assert((events==std::vector<std::array<unsigned,3>>{{0,1,0}}));++checks;
    std::printf("ragdoll contacts: %u retail filter/dispatch cases; character-body admission, both orders, exclusions, layers, self-contact and removal passed\n",checks);
}
