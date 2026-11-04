#undef NDEBUG
#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "cpu/x86_ops.h"
#include "xbe_daniels_spawn.h"
alignas(16) static std::array<uint8_t,0x800000> memory{},before{};
constexpr uint32_t A=0x600000,STATE=0x601000,MODEL=0x602000,VT=0x603000,RESOURCE=0x604000,SP=0x780000;
static uint32_t blocked=0xffffffff,received_x;
static unsigned creations;
uint8_t* B(){return memory.data();}
uint32_t Word(uint32_t p){uint32_t v;std::memcpy(&v,B()+p,4);return v;}
void Put(uint32_t p,uint32_t v){std::memcpy(B()+p,&v,4);}
namespace mem {bool IsCommitted(uint32_t p){return p<memory.size()&&p!=blocked;}}
// Resource setup and collision construction are boundaries; execute the original initializer through its transform handoff.
X86_FUNC(sub_000643E0){ctx.eax=STATE;ctx.esp+=4;}
X86_FUNC(sub_0021FC20){ctx.esp+=4;}
X86_FUNC(sub_0020A200){ctx.eax=3;ctx.esp+=4;}
X86_FUNC(sub_002B15D0){
    assert(Word(ctx.esp+4)==A+0x10&&Word(ctx.esp+8)==A);
    received_x=Word(Word(ctx.esp+4)+0x30);++creations;
    ctx.eax=MODEL;ctx.esp+=4;
}
X86_FUNC(sub_001FAB30){ctx.esp+=8;}
void x86_icall(X86Context& c,uint8_t*,uint32_t fn){
    if(fn==0x214e70){c.eax=c.ecx+0x10;c.esp+=4;return;}
    if(fn==0xf001){assert(c.ecx==MODEL);c.eax=123;c.esp+=4;return;}
    std::abort();
}
[[noreturn]]void x86_trap(X86Context&,uint32_t,const char*){std::abort();}
void x86_resume(X86Context&,uint8_t*,const X86Resume&){std::abort();}
// NATIVE_BODIES
static X86Context Setup(){
    memory.fill(0);blocked=0xffffffff;creations=0;
    Put(A,0x47bf50);Put(A+0x40,0x45900b6d);Put(A+0x44,0x42f4bef0);Put(A+0x48,0xc4204a85);
    Put(0x47bf50+0x2c,0x214e70);Put(MODEL,VT);Put(VT+0x20,0xf001);
    Put(SP,0xbeef);Put(SP+4,4);Put(SP+8,RESOURCE);Put(RESOURCE,123);
    X86Context c{};c.ecx=A;c.esp=SP;c.esi=111;c.edi=222;c.ebx=333;c.ebp=444;return c;
}
int main(){
    for(bool fixed:{false,true}){
        auto c=Setup();(fixed?FixedSpawn:OriginalSpawn)(c,B());
        assert(creations==1&&received_x==(fixed?0x458e7000u:0x45900b6du));
        assert(Word(A+0x118)==MODEL&&Word(A+0x44)==0x42f4bef0&&Word(A+0x48)==0xc4204a85);
        assert(c.esp==SP+20&&c.esi==111&&c.edi==222&&c.ebx==333&&c.ebp==444);
    }
    auto c=Setup();const auto cpu=c;const auto csr=_mm_getcsr();before=memory;
    trial_daniels_spawn::Correct(B(),c.ecx);
    const uint32_t fixed_x=0x458e7000;std::memcpy(before.data()+A+0x40,&fixed_x,4);
    assert(memory==before&&std::memcmp(&c,&cpu,sizeof(c))==0&&_mm_getcsr()==csr);
    trial_daniels_spawn::Correct(B(),c.ecx);assert(memory==before); // No repeated movement.
    for(uint32_t field:{0u,0x40u,0x44u,0x48u}){
        Setup();Put(A+field,Word(A+field)^1);before=memory;
        trial_daniels_spawn::Correct(B(),A);assert(memory==before);
    }
    Setup();before=memory;
    for(uint32_t p:{0u,0xffffu,0xfffffff0u,0x800000u}){trial_daniels_spawn::Correct(B(),p);assert(memory==before);}
    blocked=A;trial_daniels_spawn::Correct(B(),A);assert(memory==before);
    Setup();blocked=A+0x1000;before=memory;
    trial_daniels_spawn::Correct(B(),A+0xff0);assert(memory==before);
    std::puts("Daniels: original initializer forwards the corrected spawn before body creation; exact identity, idempotence, RAM/CPU/MXCSR and invalid spans passed. Live collision/rescue remains to test.");
}
