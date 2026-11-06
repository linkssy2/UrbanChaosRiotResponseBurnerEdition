// Original ray-filter execution with a typed held-body registry boundary.
// Real clone membership/lifetime checks live in test_corpse_shield.cpp.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <vector>
#include "cpu/x86_ops.h"

namespace shield_query_test {
alignas(16) std::array<uint8_t,0x100000> ram{};
constexpr uint32_t shape=0x10000,vtable=0x11000,collidables=0x20000,
    output=0x30000,ray=0x40000,stack=0xe0000;
constexpr uint32_t holders[]={77,88};
bool active[]={true,true};
unsigned lookups=0,shape_kind=0,property29=1,property2a=1;
uint8_t* Bytes(){return ram.data();}
uint32_t Get(uint32_t p){uint32_t v;std::memcpy(&v,Bytes()+p,4);return v;}
void Set(uint32_t p,uint32_t v){std::memcpy(Bytes()+p,&v,4);}
uint32_t Collidable(unsigned i){return collidables+i*0x100;}
uint32_t Query(unsigned owner){return 0x20100000u|owner;}
void Seed(){
    ram.fill(0);active[0]=active[1]=true;lookups=0;shape_kind=0;property29=property2a=1;
    Set(shape,vtable);Set(vtable+8,0xf0001);Set(shape+8,0x3f);
    for(unsigned i=0;i<5;++i){
        Set(Collidable(i),shape);Set(Collidable(i)+0x10,uint32_t(-0x14));
        Set(Collidable(i)+0x1c,0x24800000u|(1000+i));
    }
}
X86Context Context(unsigned i,uint32_t filter){
    X86Context c{};c.eax=0x1111;c.ebx=0x2222;c.ecx=0x3333;c.edx=0x4444;
    c.esi=0x5555;c.edi=0x6666;c.ebp=0x7777;c.esp=stack;
    c.fcw=0x37f;c.mxcsr=0x1f80;c.cf=c.pf=c.zf=c.sf=c.of=1;
    Set(stack,0x2ed450);Set(stack+4,output);Set(stack+8,ray);Set(stack+12,Collidable(i));
    Set(ray+0x24,filter);Bytes()[output]=0xcc;return c;
}
}

bool TrialCorpseShieldIgnoreQuery(const uint8_t*,uint32_t collidable,uint32_t filter){
    using namespace shield_query_test;++lookups;
    // This fixture supplies independently known registry answers. It executes
    // the production hook and full original filter, not a second filter model.
    for(unsigned i=0;i<2;++i)
        if(active[i]&&collidable==Collidable(i)&&(filter&0xfffff)==holders[i])return true;
    return false;
}
X86_FUNC(sub_002A31C0){
    using namespace shield_query_test;
    const uint32_t key=Get(ctx.esp+8);assert(key==0x29||key==0x2a);
    ctx.eax=key==0x29?property29:property2a;ctx.esp+=4;
}
void x86_icall(X86Context& c,uint8_t*,uint32_t fn){
    using namespace shield_query_test;assert(fn==0xf0001&&c.ecx==shape);
    c.eax=shape_kind;c.esp+=4;
}
[[noreturn]] void x86_trap(X86Context&,uint32_t,const char*){std::abort();}
void x86_resume(X86Context&,uint8_t*,const X86Resume&){std::abort();}

// NATIVE_CORPSE_SHIELD_QUERY

namespace shield_query_test {
using Filter=void(*)(X86Context&,uint8_t*);
bool Run(Filter f,unsigned i,uint32_t filter){
    auto c=Context(i,filter);const auto before=c;f(c,Bytes());
    assert(c.esp==stack+16&&c.eax==output);
    assert(c.ebx==before.ebx&&c.esi==before.esi&&c.edi==before.edi&&c.ebp==before.ebp);
    assert(Get(stack)==0x2ed450&&Get(stack+4)==output&&Get(stack+8)==ray&&Get(stack+12)==Collidable(i));
    assert(Bytes()[output]<=1);return Bytes()[output]!=0;
}
void Compare(unsigned i,uint32_t filter,bool changed){
    const std::vector<uint8_t> initial(ram.begin(),ram.end());auto a=Context(i,filter);const auto input=a;
    NativeShieldQuery(a,Bytes());const std::vector<uint8_t> original(ram.begin(),ram.end());
    std::memcpy(Bytes(),initial.data(),ram.size());auto b=Context(i,filter);HeldShieldQuery(b,Bytes());
    assert(std::memcmp(&a,&b,sizeof a)==0);
    assert(b.esp==input.esp+16&&b.eax==output);
    if(changed){assert(original[output]==1&&ram[output]==0);ram[output]=1;}
    assert(std::memcmp(Bytes(),original.data(),ram.size())==0);
}
void FilterCases(){
    for(unsigned kind:{0u,12u,24u}){
        Seed();shape_kind=kind;
        Compare(0,Query(holders[0]),true);Compare(1,Query(holders[1]),true);
        Compare(0,Query(holders[1]),false);Compare(1,Query(holders[0]),false);
        Compare(2,Query(holders[0]),false);Compare(0,Query(0xfffff),false);
        active[0]=false;Compare(0,Query(holders[0]),false);
        active[0]=true;
        // Native mask/group rejections retain exact CPU and guest-memory parity.
        lookups=0;Compare(0,1000,false);assert(lookups==0);
        Compare(0,holders[0],false);assert(lookups==0);
    }
    Seed();Bytes()[Collidable(0)+0x18]=2;
    property29=0;lookups=0;Compare(0,Query(holders[0]),false);assert(lookups==0);
    property29=1;property2a=0;Compare(0,Query(holders[0]),false);assert(lookups==0);
    property2a=1;Compare(0,Query(holders[0]),true);assert(lookups==1);
}
struct Candidate{unsigned collidable;uint32_t actor;};
uint32_t FirstActor(Filter f,uint32_t filter,std::initializer_list<Candidate> ordered){
    // Typed nearest-hit traversal boundary, not a replacement Havok collector.
    // C57C0 stops at its first hit even when hit+2C has no actor to damage.
    for(const auto& hit:ordered)if(Run(f,hit.collidable,filter))return hit.actor;
    return 0;
}
void BashTargetCases(){
    constexpr uint32_t enemy=0xabc;
    for(unsigned slot=0;slot<2;++slot){
        Seed();const uint32_t filter=Query(holders[slot]);
        assert(FirstActor(NativeShieldQuery,filter,{{slot,0},{2,enemy}})==0);
        assert(FirstActor(HeldShieldQuery,filter,{{slot,0},{2,enemy}})==enemy);
        // An actual wall or another player's held body still stops the cast.
        assert(FirstActor(HeldShieldQuery,filter,{{3,0},{slot,0},{2,enemy}})==0);
        assert(FirstActor(HeldShieldQuery,filter,{{1-slot,0},{slot,0},{2,enemy}})==0);
        // Ordinary targets and a retired shield retain the original outcome.
        assert(FirstActor(HeldShieldQuery,filter,{{2,enemy}})==enemy);
        active[slot]=false;
        assert(FirstActor(HeldShieldQuery,filter,{{slot,0},{2,enemy}})==0);
    }
}
}
int main(){shield_query_test::FilterCases();shield_query_test::BashTargetCases();
    std::puts("PASS native ray-filter ABI/parity, own-body exclusion and typed bash target traversal for both owners");}
