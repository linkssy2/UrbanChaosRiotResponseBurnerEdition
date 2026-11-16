// Complete original world owner and delta publication; downstream services are
// completing observations, not replacement physics/audio implementations.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <vector>
#include "kernel/kernel.h"
namespace kernel {static std::array<uint8_t,0x800000> memory{};}
static uint32_t missing_page=0xffffffffu;
namespace mem {bool IsCommitted(uint32_t a){return a>=0x10000&&a<kernel::memory.size()&&(a>>12)!=missing_page;}}
#include "../common/xbe_weapon_wheel_time.cpp"
constexpr uint32_t W=0x600000,L=0x610000,V=0x620000,G=0x630000,O=0x640000,T=0x650000,S=0x7e0000;
static bool wheel=true,throw_call=false;
static uint32_t wheel_world=W;
namespace trial_weapon_wheel {bool SlowTimeActive(uint32_t world){return wheel&&world==wheel_world;}}
static unsigned checks=0;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);std::abort();}}while(0)
struct Event {uint32_t call,scaled,unscaled;bool operator==(const Event&)const=default;};
static std::vector<Event> events;
uint8_t* B(){return kernel::memory.data();}
uint32_t Get(uint32_t at){uint32_t v;std::memcpy(&v,B()+at,4);return v;}
void Put(uint32_t at,uint32_t v){std::memcpy(B()+at,&v,4);}
uint32_t Bits(float value){uint32_t v;std::memcpy(&v,&value,4);return v;}
void EventAt(uint32_t tag,uint32_t first=0,uint32_t second=0){events.push_back({tag,first,second});}
[[noreturn]] void x86_trap(X86Context&,uint32_t,const char*){std::abort();}
X86_FUNC(sub_002D3670){if(throw_call)throw std::runtime_error("world callback");ctx.eax=17;ctx.esp+=4;}
X86_FUNC(sub_0001CC80){EventAt(0x1cc80,Get(W+0x14));ctx.esp+=4;}
X86_FUNC(sub_00024920){EventAt(0x24920,Get(W+0x14));ctx.esp+=4;}
X86_FUNC(sub_0002F5A0){EventAt(0x2f5a0,Get(W+0x14));ctx.esp+=4;}
X86_FUNC(sub_00219DF0){EventAt(0x219df0,Get(ctx.esp+4));ctx.esp+=4;}
X86_FUNC(sub_00044600){EventAt(0x44600,Get(ctx.esp+4));ctx.esp+=8;}
X86_FUNC(sub_0004F550){EventAt(0x4f550,Get(ctx.esp+4));ctx.esp+=8;}
X86_FUNC(sub_00048E80){EventAt(0x48e80,Get(ctx.esp+4));ctx.esp+=8;}
X86_FUNC(sub_00208930){EventAt(0x208930,Get(ctx.esp+4));ctx.esp+=8;}
X86_FUNC(sub_0006BA10){EventAt(0x6ba10,Get(ctx.esp+4));ctx.esp+=4;}
X86_FUNC(sub_00211B80){ctx.esp+=4;}
X86_FUNC(sub_002145D0){ctx.esp+=4;}
#define UNREACHED(name) X86_FUNC(name){CHECK(false);}
UNREACHED(sub_001F8FC0)
UNREACHED(sub_001F90A0)
UNREACHED(sub_0020A140)
UNREACHED(sub_00210CA0)
UNREACHED(sub_00216040)
UNREACHED(sub_00221A10)
UNREACHED(sub_002D5CCD)
#undef UNREACHED
void x86_icall(X86Context& ctx,uint8_t* base,uint32_t target){
    CHECK(base==B()&&target>=0xf0000&&target<0xf0100);
    const auto caller=Get(ctx.esp);
    switch(caller){
    case 0x55701:case 0x5571c:ctx.eax=O;ctx.esp+=4;return;
    case 0x55723:ctx.eax=0;ctx.esp+=4;return;
    case 0x2092e9:case 0x209308:
    case 0x5567a:case 0x55686:case 0x55692:case 0x5569e:case 0x556aa:
    case 0x556b6:case 0x556c2:case 0x556da:case 0x55709:
        EventAt(caller,Get(ctx.esp+4),Get(W+0x18));ctx.esp+=8;return;
    default:std::fprintf(stderr,"unexpected virtual caller %x\n",caller);CHECK(false);
    }
}
// ORIGINAL_WORLD_FRAME

X86Context Setup(uint32_t delta=0x3d000000){
    kernel::memory.fill(0);events.clear();missing_page=0xffffffffu;
    trial_coop::Reset();wheel=true;wheel_world=W;throw_call=false;
    Put(0x51c4dc,W);Put(W,0x450c48);Put(W+4,L);Put(W+8,Bits(20));
    Put(W+0x10,0x80);Put(W+0x14,Bits(.04f));Put(W+0x18,Bits(.08f));
    Put(L+4,V);Put(L+8,V+4);Put(V,0x660000);
    Put(0x509a60,G);Put(G,0x450588);Put(0x4505a8,0x276e0);
    Put(0x5150b0,O);Put(O,T);for(unsigned i=0;i<0x100;i+=4)Put(T+i,0xf0000+i);
    for(unsigned offset:{0x908u,0x90cu,0x910u,0x914u,0x918u,0x91cu,0x928u,0x92cu,0x930u,0x934u,0x938u,0x93cu,0x940u,0x944u})Put(W+offset,O);
    Put(L+0x2054,O);Put(L+0x205c,0x661000);Put(0x661000,0x661000);
    Put(L+0x2068,0x662000);Put(0x662000,0x662000);
    Put(L+0x30,0x663000);Put(L+0x38,0x664000);Put(L+0x10,O);Put(L+0x2070,O);
    Put(0x4e7d38,Bits(.5f)); // A pre-existing original slow-motion effect is untouched.
    X86Context c{};c.ecx=W;c.esp=S;c.eax=0x12345678;c.ebx=0x23456789;c.esi=0x3456789a;c.edi=0x456789ab;c.ebp=0x56789abc;
    Put(S,0x5d214);Put(S+4,delta);Put(S+8,Bits(.0625f));return c;
}
void Unchanged(const X86Context& c,uint8_t* base=nullptr){
    const auto before=c;const std::vector<uint8_t> memory(kernel::memory.begin(),kernel::memory.end());
    const auto csr=_mm_getcsr();{TrialWeaponWheelTime scope(c,base?base:B());CHECK(!std::memcmp(&before,&c,sizeof c));}
    CHECK(memory==std::vector<uint8_t>(kernel::memory.begin(),kernel::memory.end()));CHECK(csr==_mm_getcsr());
}
int main(){
    const unsigned saved_csr=_mm_getcsr();
    // Full original frame with a scaled input is the behavioral reference;
    // the production scope must restore only its borrowed caller argument.
    for(unsigned mode:{0u,0x2000u,0x4000u,0x6000u})for(float dt:{1.f/30,1.f/60,1.f/240,.1f}){
        auto c=Setup(Bits(dt));const unsigned csr=0x1f80|mode;_mm_setcsr(csr);
        volatile float input=dt;float scaled=input*.2f;_mm_setcsr(csr);Put(S+4,Bits(scaled));
        OriginalWorldFrame(c,B());Put(S+4,Bits(dt));const auto reference=c;
        const auto expected_events=events;const auto expected_csr=_mm_getcsr();
        const std::vector<uint8_t> expected(kernel::memory.begin(),kernel::memory.end());
        c=Setup(Bits(dt));_mm_setcsr(csr);WheelWorldFrame(c,B());
        CHECK(!std::memcmp(&reference,&c,sizeof c));CHECK(expected_events==events);CHECK(expected_csr==_mm_getcsr());
        CHECK(expected==std::vector<uint8_t>(kernel::memory.begin(),kernel::memory.end()));
        CHECK(Get(W+0x14)==Bits(scaled)&&Get(W+0x18)==Bits(.0625f));
        CHECK(Get(S+4)==Bits(dt)&&Get(S+8)==Bits(.0625f)&&Get(0x4e7d38)==Bits(.5f));
        CHECK(events.size()>15&&events[0].scaled==Bits(.04f));
    }
    for(uint32_t bits:{0u,0x80000000u,0xbf800000u,0x7f800000u,0xff800000u,0x7fc12345u,0x7f812345u}){auto c=Setup(bits);Unchanged(c);}
    for(unsigned bad=0;bad<15;++bad){
        auto c=Setup();
        switch(bad){
        case 0:wheel=false;break;case 1:wheel_world=W+0x100;break;
        case 2:trial_coop::SetActive(true);break;case 3:trial_coop::requested=true;break;
        case 4:Put(S,0x5d215);break;case 5:c.ecx=W+4;break;
        case 6:Put(W,0x480570);break;case 7:Put(W+0x10,0x880);break;case 8:Put(W+0x10,0);break;
        case 9:Put(L+8,V+8);break;case 10:Put(L+4,0xfffffffcu);break;
        case 11:Put(0x4505a8,0x276f0);break;case 12:Put(0x509a60,0);break;
        case 13:missing_page=W>>12;break;case 14:missing_page=S>>12;break;
        }
        Unchanged(c);
    }
    // Nested entry, exceptional callback exit and tiny inputs cannot leak a
    // borrowed delta or host floating-point status into later original work.
    auto c=Setup();const auto argument=Get(S+4);const auto before=c;const auto csr=_mm_getcsr();
    {TrialWeaponWheelTime outer(c,B());const auto scaled=Get(S+4);CHECK(scaled!=argument);
        {TrialWeaponWheelTime inner(c,B());CHECK(Get(S+4)==scaled);}CHECK(Get(S+4)==scaled);
        CHECK(!std::memcmp(&before,&c,sizeof c));}
    CHECK(Get(S+4)==argument&&_mm_getcsr()==csr);
    c=Setup();throw_call=true;bool threw=false;
    try{WheelWorldFrame(c,B());}catch(const std::runtime_error&){threw=true;}
    CHECK(threw&&Get(S+4)==0x3d000000u);c=Setup();{TrialWeaponWheelTime again(c,B());CHECK(Get(S+4)!=0x3d000000u);}
    for(unsigned mode:{0u,0x8040u}){c=Setup(1);_mm_setcsr(0x1fa5|mode);const auto flags=_mm_getcsr();{TrialWeaponWheelTime small(c,B());CHECK(_mm_getcsr()==flags);}CHECK(Get(S+4)==1&&_mm_getcsr()==flags);}
    _mm_setcsr(saved_csr);std::printf("weapon wheel time: %u checks passed (original world frame, unscaled clock, pause, multiplayer, nesting, unwind)\n",checks);
}
