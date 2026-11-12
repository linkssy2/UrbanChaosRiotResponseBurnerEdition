
#undef NDEBUG
#include <cassert>
#include <cstdio>
#include <cstring>
#include "cpu/x86_ops.h"
static unsigned char memory[0x600000]{};
static bool eligible=true,placed=false;static unsigned attempts=0;
void put(unsigned a,unsigned v){std::memcpy(memory+a,&v,4);}
void x86_icall(X86Context& ctx,uint8_t*,uint32_t target){ctx.eax=eligible;ctx.esp+=target==1?4:12;}
X86_FUNC(sub_00056D20){++attempts;ctx.eax=placed;ctx.esp+=16;}
struct TrialBloodPoolScope {};
static uint64_t submissions=0;
static bool ground=false,admitted=false;
static unsigned submitted=0;
uint64_t TrialDecalSubmissions(){return submissions;}
X86_FUNC(sub_0003BD10){ctx.eax=X86_MEM32(ctx.esp+4);std::memset(base+ctx.eax,0,64);X86_MEMF32(ctx.eax+0x20)=1;ctx.esp+=8;}
X86_FUNC(sub_00055F40){ctx.eax=ground;ctx.esp+=20;}
X86_FUNC(sub_002088D0){
    ++submitted;assert(X86_MEM32(ctx.esp+48)==7);
    assert(X86_MEM32(X86_MEM32(ctx.esp+4))==0x450f10);
    if(admitted)++submissions;ctx.esp+=56;
}
// BLOOD_CREATION_BODIES
// BLOOD_PLACEMENT_BODY
void run(void(*f)(X86Context&,uint8_t*)){X86Context c{};c.ecx=0x10000;c.esp=0x40000;f(c,memory);}
int main(){
 put(0x10000+0xd4,0x11000);put(0x11000+0x118,0x12000);put(0x12000,0x13000);put(0x13000+0x1c,1);put(0x11000,0x14000);put(0x14000+0x14,2);
 run(Original);assert(memory[0x100f0]==1&&attempts==1);run(Original);assert(attempts==1);
 memory[0x100f0]=0;attempts=0;run(Candidate);assert(memory[0x100f0]==0&&attempts==1);
 placed=true;run(Candidate);assert(memory[0x100f0]==1&&attempts==2);run(Candidate);assert(attempts==2);
 memory[0x100f0]=0;eligible=false;run(Candidate);assert(!memory[0x100f0]&&attempts==2);
 puts("PASS retail creation prefix: failed placement retries, success emits once, ineligible actor emits nothing");
 for(unsigned mode=0;mode<3;++mode){
    ground=mode>0;admitted=mode==2;submitted=0;
    X86Context c{};c.esp=0x50000;c.ecx=0x22000;
    put(c.esp+4,0x20000);put(c.esp+8,0x21000);put(c.esp+12,0);
    put(0x44bf94,0x3f800000);PlacePool(c,memory);
    assert(c.esp==0x50010&&c.eax==unsigned(admitted)&&submitted==unsigned(ground));
 }
 puts("PASS complete pool placement: ground failure, renderer rejection, accepted submission, surface-clipping bypass flag and stack balance");
}
