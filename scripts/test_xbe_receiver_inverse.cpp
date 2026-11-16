#define NOMINMAX
#include <windows.h>
#include "cpu/x86_ops.h"
#include "xbe_receiver_inverse.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace mem {
uint8_t* mapped_base=nullptr;
std::array<uint8_t,0x100000> committed{};
uint8_t* Base(){return mapped_base;}
bool IsCommitted(uint32_t address){return committed[address>>12]!=0;}
}
#undef X86_FUNC
#define X86_FUNC(name) extern "C" __attribute__((noinline)) void name(X86Context& __restrict ctx,uint8_t* base)
// CURRENT_GENERATED_INVERSE

namespace {
using Matrix=std::array<uint32_t,16>;
xbe_receiver_inverse::Cache cache;
uint32_t random_state=0x749ed31u;
uint64_t checks=0,hits=0,original_calls=0,high_stack=0;
unsigned stub_calls=0;
uint32_t Random(){random_state^=random_state<<13;random_state^=random_state>>17;random_state^=random_state<<5;return random_state;}
void Require(bool value,const char* why){if(!value){std::fprintf(stderr,"FAIL check=%llu %s\n",(unsigned long long)checks,why);std::abort();}}
void Fill(void* p,size_t bytes){for(size_t i=0;i<bytes;++i)static_cast<uint8_t*>(p)[i]=uint8_t(Random());}
void Put(uint32_t address,uint32_t value){std::memcpy(mem::mapped_base+address,&value,4);}
uint32_t Bits(float value){uint32_t word;std::memcpy(&word,&value,4);return word;}
void Commit(uint32_t address,uint32_t bytes){
    Require(VirtualAlloc(mem::mapped_base+address,bytes,MEM_COMMIT,PAGE_READWRITE)!=nullptr,"commit test pages");
    for(uint32_t page=address;page<address+bytes;page+=0x1000)mem::committed[page>>12]=1;
}
Matrix Make(unsigned shape){
    Matrix m{};for(unsigned i=0;i<4;++i)m[i*5]=Bits(1.f);
    if(shape==0)return m;
    if(shape==1){for(unsigned i=0;i<12;++i)if(i%5!=0)m[i]=0x80000000u;return m;}
    if(shape==2){m.fill(0);return m;}
    if(shape==3){for(auto& x:m)x=Bits(float(int(Random()%2001)-1000)/32.f);return m;}
    if(shape==4){for(auto& x:m)x=Random();return m;}
    if(shape==5){m[Random()%16]=0x7fc12345u;return m;}
    if(shape==6){m[Random()%16]=0x7f812345u;return m;}
    if(shape==7){m[Random()%16]=(Random()&1)?0x7f800000u:0xff800000u;return m;}
    if(shape==8){for(auto& x:m)x=(Random()&1)?1u:0x80000001u;return m;}
    if(shape==9){for(auto& x:m)x=(Random()&1)?0x7f7fffffu:0xff7fffffu;return m;}
    if(shape==10){for(unsigned i=0;i<4;++i)m[i+4]=m[i];return m;}
    for(unsigned i=0;i<3;++i)m[i*5]=Bits(.125f+float(Random()%64)/8.f);
    m[1]=Bits(float(int(Random()%31)-15)/128.f);m[4]=Bits(float(int(Random()%31)-15)/128.f);
    for(unsigned i=12;i<15;++i)m[i]=Bits(float(int(Random()%20001)-10000)/8.f);
    return m;
}
X86_FUNC(CountedOriginal){++original_calls;Original(ctx,base);}
bool Compare(uint32_t sp,const Matrix& input,uint32_t csr,bool entry=false){
    X86Context initial;Fill(&initial,sizeof initial);initial.esp=sp;initial.ecx=sp+0x94;
    const uint32_t start=(sp&~0xfffu)-0x1000;constexpr unsigned bytes=0x3000;
    Fill(mem::mapped_base+start,bytes);Put(sp,0xb3404);
    const uint32_t constant=Random();Put(0x44c174,constant);
    std::memcpy(mem::mapped_base+initial.ecx,input.data(),64);
    std::vector<uint8_t> before(mem::mapped_base+start,mem::mapped_base+start+bytes);
    auto expected=initial;_mm_setcsr(csr);Original(expected,mem::mapped_base);const auto expected_csr=_mm_getcsr();
    std::vector<uint8_t> wanted(mem::mapped_base+start,mem::mapped_base+start+bytes);
    std::memcpy(mem::mapped_base+start,before.data(),bytes);auto actual=initial;
    const auto calls=original_calls;_mm_setcsr(csr);
    if(entry)TrialReceiverInverse(actual,mem::mapped_base,CountedOriginal);
    else cache.Call(actual,mem::mapped_base,CountedOriginal);
    const auto actual_csr=_mm_getcsr();
    Require(!std::memcmp(&actual,&expected,sizeof actual),"full guest context");
    Require(actual_csr==expected_csr,"exact host MXCSR");
    Require(!std::memcmp(wanted.data(),mem::mapped_base+start,bytes),"all stack pages including gaps");
    Require(!std::memcmp(mem::mapped_base+0x44c174,&constant,4),"fixed read unchanged");
    const bool hit=calls==original_calls;hits+=hit;high_stack+=sp>0x08000000u;++checks;return hit;
}
X86_FUNC(Stub){++stub_calls;ctx.eax=0x13579bdf;ctx.esp+=4;}
void Guards(){
    for(unsigned kind=0;kind<10;++kind){
        X86Context ctx{};ctx.esp=0xd007eff0;ctx.ecx=ctx.esp+0x94;Put(ctx.esp,0xb3404);
        uint8_t* base=mem::mapped_base;uint32_t page=0;
        if(kind==0)page=(((ctx.esp-4)&~15u)-192)>>12;
        if(kind==1)page=(ctx.ecx+63)>>12;
        if(kind==2)page=0x44c174>>12;
        DWORD previous=0;
        if(kind<3){mem::committed[page]=0;Require(VirtualProtect(mem::mapped_base+(size_t(page)<<12),0x1000,PAGE_NOACCESS,&previous)!=FALSE,"protect unmapped guard page");}
        if(kind==3)base=nullptr;
        if(kind==4)base+=0x1000;
        if(kind==5){ctx.esp=0xfffffff0;ctx.ecx=0x84;}
        if(kind==6)ctx.esp=0x10;
        if(kind==7)ctx.ecx+=4;
        if(kind==8)Put(ctx.esp,0xb3405);
        const uint32_t csr=kind==9?0:0x1f80;
        auto expected=ctx;expected.eax=0x13579bdf;expected.esp+=4;
        const auto calls=stub_calls;_mm_setcsr(csr);cache.Call(ctx,base,Stub);const auto actual_csr=_mm_getcsr();_mm_setcsr(0x1f80);
        Require(stub_calls==calls+1&&!std::memcmp(&ctx,&expected,sizeof ctx)&&actual_csr==csr,"guard exact original-only context and FP effects");
        if(kind<3){DWORD ignored;Require(VirtualProtect(mem::mapped_base+(size_t(page)<<12),0x1000,previous,&ignored)!=FALSE,"restore guard page");mem::committed[page]=1;}++checks;
    }
    for(unsigned masks=0;masks<63;++masks)for(unsigned rounding=0;rounding<4;++rounding){
        X86Context ctx{};ctx.esp=0xd007e080;ctx.ecx=ctx.esp+0x94;Put(ctx.esp,0xb3404);
        const auto calls=stub_calls;const uint32_t csr=(masks<<7)|(rounding<<13);
        _mm_setcsr(csr);cache.Call(ctx,mem::mapped_base,Stub);const auto after=_mm_getcsr();_mm_setcsr(0x1f80);
        Require(stub_calls==calls+1&&after==csr,"every unmasked exception mode bypasses without FP effects");++checks;
    }
}
}
int main(){
    const auto saved=_mm_getcsr();
    mem::mapped_base=static_cast<uint8_t*>(VirtualAlloc(nullptr,size_t(1ull<<32),MEM_RESERVE,PAGE_NOACCESS));
    Require(mem::mapped_base!=nullptr,"reserve guest VA");
    Commit(0x20000,0x4000);Commit(0xd007d000,0x4000);Commit(0x44c000,0x1000);
    for(unsigned mode=0;mode<16;++mode)for(unsigned trial=0;trial<256;++trial){
        const uint32_t csr=0x1f80|((mode&3)<<13)|((mode&4)?0x8000:0)|((mode&8)?0x40:0)|(trial&63);
        const auto matrix=Make(trial%16);cache={};
        Compare(0x21080+trial%16,matrix,csr);
        Compare(0xd007e080+(trial+7)%16,matrix,csr);
        Compare(0xd007eff0+(trial+13)%16,matrix,csr^0x20);
    }
    for(unsigned word=0;word<16;++word){
        cache={};auto m=Make(0);Require(!Compare(0x21080,m,0x1f80),"cold inverse calls original");
        Require(Compare(0xd007e080,m,0x1f80),"warm inverse replays across stack addresses");
        m[word]^=1;Require(!Compare(0x21080,m,0x1f80),"every matrix word participates in key");
    }
    Require(!Compare(0x21080,Make(0),0x1f80,true),"production entry cold call");
    Require(Compare(0xd007e080,Make(0),0x1f80,true),"production entry warm call");
    Guards();Require(hits>1000&&original_calls>1000&&high_stack>1000,"cache and high-stack paths executed");
    std::printf("PARITY checks=%llu high_stack=%llu hits=%llu original_calls=%llu guard_calls=%u cache_bytes=%zu\n",(unsigned long long)checks,(unsigned long long)high_stack,(unsigned long long)hits,(unsigned long long)original_calls,stub_calls,sizeof(cache));
    Require(VirtualFree(mem::mapped_base,0,MEM_RELEASE)!=FALSE,"release guest VA");_mm_setcsr(saved);return 0;
}
