// Real original query thunk, XAPI query, title choice and mode setter. The old
// kernel value is a negative witness; AV/lock services complete original calls.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include "kernel/kernel.h"

namespace kernel {
static std::array<uint8_t,0x800000> memory{};
uint8_t* g_base=memory.data();
}
using namespace kernel;
namespace trial_language { uint32_t XboxLanguage(){return 4;} }
static uint32_t VideoFlags(){return 0x90000;}
#undef LOG_FIRST
#define LOG_FIRST(...) ((void)0)
/* BEFORE_QUERY */
/* AFTER_QUERY */

using Query=uint32_t(*)(const uint32_t*);
static Query query=AfterQuery;
static uint32_t av_pack;
static bool fail_query,lock_held;
static unsigned queries,releases;
void x86_kernel(X86Context& ctx,uint8_t*,uint32_t ordinal){
    if(ordinal==24){
        uint32_t args[5];for(unsigned i=0;i<5;++i)args[i]=R32(ctx.esp+4+i*4);
        assert(args[0]==9&&args[1]&&args[2]&&args[3]==4&&args[4]==0);
        ++queries;ctx.eax=fail_query?kUnsuccessful:query(args);ctx.esp+=24;
    }else{
        assert(ordinal==294&&R32(ctx.esp+4)==0x441ca4&&lock_held);
        ++releases;ctx.eax=0;ctx.esp+=8;
    }
}
X86_FUNC(sub_00085D84){ctx.eax=av_pack;ctx.esp+=4;}
X86_FUNC(sub_00423AB5){ctx.eax=lock_held?1:0;ctx.esp+=4;}
/* ORIGINAL_FUNCTIONS */

static constexpr uint32_t block=0x10000,type=block+16,result=block+20,value=block+64;
using Snapshot=std::array<uint8_t,512>;
static Snapshot RunQuery(Query fn,uint32_t index,uint32_t length,unsigned outputs){
    std::memset(HostPtr(block),0xa5,512);
    const uint32_t args[]={index,(outputs&1)?type:0,(outputs&2)?value:0,
                           length,(outputs&4)?result:0};
    assert(fn(args)==kSuccess);
    if(outputs&1)assert(R32(type)==(index==0xffff?3u:4u));
    if(outputs&4)assert(R32(result)==(index==0xffff?256u:4u));
    for(uint32_t at=block;at<block+512;++at){
        const bool changed=((outputs&1)&&at>=type&&at<type+4)
            ||((outputs&4)&&at>=result&&at<result+4)
            ||((outputs&2)&&at>=value&&at<value+length);
        if(!changed)assert(memory[at]==0xa5);
    }
    Snapshot snapshot{};std::memcpy(snapshot.data(),HostPtr(block),snapshot.size());
    return snapshot;
}
static void CheckQuery(uint32_t index,uint32_t length,unsigned outputs){
    auto before=RunQuery(BeforeQuery,index,length,outputs);
    auto after=RunQuery(AfterQuery,index,length,outputs);
    const bool audio=(outputs&2)&&((index==9&&length>=4)||(index==0xffff&&length>=256));
    if(audio){
        const size_t at=value-block+(index==0xffff?0xf0:0);
        uint32_t old=0,now=0;std::memcpy(&old,before.data()+at,4);std::memcpy(&now,after.data()+at,4);
        assert(old==0x10001&&now==0);
        std::memset(before.data()+at,0,4);
    }
    // Byte comparison also protects language, video, region, type/length and
    // legacy unknown/short/null-query behavior outside the two intended fields.
    assert(before==after);
}

static constexpr uint32_t stack=0x70000,mode=0x441c94;
static X86Context Seed(){
    memory.fill(0xa5);queries=releases=0;
    X86Context ctx{};ctx.esp=stack;ctx.ebp=0x10111213;ctx.ebx=0x20212223;
    ctx.esi=0x30313233;ctx.edi=1;ctx.ecx=0x40414243;
    W32(stack,0xabcdef01);W32(stack+12,1);
    return ctx;
}
static void CheckAbi(const X86Context& ctx,uint32_t expected_sp){
    assert(ctx.esp==expected_sp&&ctx.ebp==0x10111213&&ctx.ebx==0x20212223
           &&ctx.esi==0x30313233&&ctx.edi==1);
    assert(R32(stack-128)==0xa5a5a5a5&&R32(stack+128)==0xa5a5a5a5);
    assert(R32(mode-4)==0xa5a5a5a5&&R32(mode+4)==0xa5a5a5a5);
    assert(R32(stack)==0xabcdef01);
    assert(queries==1);
}
static void CheckThunk(Query fn,uint32_t expected_flags){
    query=fn;fail_query=false;
    auto ctx=Seed();
    W32(stack+4,9);W32(stack+8,type);W32(stack+12,value);
    W32(stack+16,4);W32(stack+20,0);
    sub_00086EDE(ctx,g_base);
    CheckAbi(ctx,stack+24);
    assert(ctx.eax==kSuccess&&R32(type)==4&&R32(value)==expected_flags);
    assert(R32(value-4)==0xa5a5a5a5&&R32(value+4)==0xa5a5a5a5);
}
static void CheckOriginal(Query fn,uint32_t pack,bool fail,bool locked,uint32_t expected_flags,uint32_t expected_mode){
    query=fn;av_pack=pack;fail_query=fail;lock_held=locked;
    auto ctx=Seed();sub_00385A9D(ctx,g_base);
    CheckAbi(ctx,stack+4);assert(ctx.eax==expected_flags&&releases==0&&R32(mode)==0xa5a5a5a5);
    ctx=Seed();OriginalTitleMode(ctx,g_base);
    CheckAbi(ctx,stack);assert(R32(stack+12)==expected_mode&&R32(mode)==expected_mode);
    assert(releases==(locked?1u:0u));
}
int main(){
    for(unsigned outputs=0;outputs<8;++outputs){
        for(uint32_t length:{0u,1u,2u,3u,4u,8u,255u,256u,260u}){
            CheckQuery(9,length,outputs);CheckQuery(0xffff,length,outputs);
            CheckQuery(0xdeadbeef,length,outputs);
        }
        for(uint32_t index:{7u,8u,0x103u,0x104u,0x12u})CheckQuery(index,4,outputs);
    }
    // Explicitly preserve the video and region values and agreement of both
    // language-query forms, including a query extracted after language setup.
    RunQuery(AfterQuery,7,4,7);const uint32_t language=R32(value);
    RunQuery(AfterQuery,8,4,7);assert(R32(value)==0x90000);
    RunQuery(AfterQuery,0xffff,256,7);
    assert(R32(value+0xe8)==language&&R32(value+0xec)==0x90000&&R32(value+0xf0)==0);
    assert(R32(value+0x58)==0x00400100&&R32(value+0x2c)==1);
    RunQuery(AfterQuery,0xdeadbeef,4,7);assert(R32(value)==0);
    CheckThunk(BeforeQuery,0x10001);CheckThunk(AfterQuery,0);
    for(bool locked:{false,true}){
        // Negative witness: the exact original caller chooses/stores mode 1
        // with the old reported flags and mode 0 after the stereo correction.
        CheckOriginal(BeforeQuery,0,false,locked,0x10001,1);
        CheckOriginal(AfterQuery,0,false,locked,0,0);
        CheckOriginal(BeforeQuery,6,false,locked,1,1);
        CheckOriginal(AfterQuery,6,false,locked,0,0);
        // Original AV-pack mono override and failed-query fallback remain.
        for(Query fn:{BeforeQuery,AfterQuery}){
            CheckOriginal(fn,3,false,locked,1,1);
            CheckOriginal(fn,0,true,locked,0,0);
            CheckOriginal(fn,6,true,locked,0,0);
            CheckOriginal(fn,3,true,locked,1,1);
        }
    }
    std::puts("PASS: original audio query/title negative witness, ABI, EEPROM and query sizes");
}
