// Production setting/cache hook, actual XAPI query and original title selection.
// Stream reads/allocation are completing fixture services; no user files change.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <vector>
#include "kernel/kernel.h"
#include "xbe_language.h"

namespace kernel {
static std::array<uint8_t,0x800000> memory{};
uint8_t* g_base=memory.data();
}
using namespace kernel;
namespace mem {
static uint32_t denied=0xffffffff;
bool IsCommitted(uint32_t page){return page<memory.size()&&page!=denied;}
}
namespace app {
static std::recursive_mutex lock;
static unsigned saves;
static uint32_t saved_language;
static bool fail_save;
std::recursive_mutex& UiLock(){return lock;}
void Save(){++saves;if(fail_save)throw std::runtime_error("fixture write denied");saved_language=TrialLanguage;}
}
/* PRODUCTION_LANGUAGE */

static uint32_t VideoFlags(){return 0x90000;}
#undef LOG_FIRST
#define LOG_FIRST(...) ((void)0)
/* PRODUCTION_QUERY */

static uint32_t fixture_region=0x00400100,fixture_language=0xffffffff;
X86_FUNC(sub_00086EDE){
    uint32_t args[5];for(unsigned i=0;i<5;++i)args[i]=R32(ctx.esp+4+i*4);
    ctx.eax=QuerySettings(args);
    if(args[2]&&args[3]>=4){
        if(args[0]==0x103)W32(args[2],fixture_region);
        if(args[0]==7&&fixture_language!=0xffffffff)W32(args[2],fixture_language);
    }
    ctx.esp+=24;
}
[[noreturn]] void x86_trap(X86Context&,uint32_t,const char*){throw std::runtime_error("unexpected original trap");}
static std::vector<uint8_t> bank;
static size_t stream_position;
void x86_icall(X86Context& ctx,uint8_t* base,uint32_t target){
    assert(ctx.ecx==0x30000);
    if(target==0xf001){
        const uint32_t to=X86_MEM32(ctx.esp+4),n=X86_MEM32(ctx.esp+8);
        assert(stream_position+n<=bank.size()&&uint64_t(to)+n<memory.size());
        std::memcpy(base+to,bank.data()+stream_position,n);stream_position+=n;ctx.esp+=12;
    }else if(target==0xf002){
        stream_position+=X86_MEM32(ctx.esp+4);assert(stream_position<=bank.size());ctx.esp+=8;
    }else assert(false&&"unknown fixture stream method");
}
X86_FUNC(sub_002D589F){assert(X86_MEM32(ctx.esp+4)==80);ctx.eax=0x40000;ctx.esp+=4;}
X86_FUNC(sub_00017740){ctx.eax=0x12345678;ctx.esp+=4;}
/* ORIGINAL_FUNCTIONS */

static constexpr uint32_t object=0x10000,stack=0x20000;
static void Seed(){
    memory.fill(0);W32(0x5096e0,object);W32(object+0x28,0xffffffff);
    const uint32_t table[]={0x16c45,0x16c45,0x16c2a,0x16c33,0x16c45,0x16c3c};
    for(unsigned i=0;i<6;++i)W32(0x16c50+i*4,table[i]);
    fixture_region=0x00400100;fixture_language=0xffffffff;mem::denied=0xffffffff;
}
static X86Context Context(uint32_t caller=0x16eef){
    X86Context ctx{};ctx.ecx=object;ctx.esp=stack;ctx.eax=12;ctx.edx=13;
    ctx.ebx=14;ctx.esi=15;ctx.edi=16;ctx.ebp=17;
    W32(stack,caller);W32(stack-0x100,0x12345678);W32(stack+0x100,0x87654321);
    return ctx;
}
static void CheckAbi(const X86Context& ctx){
    assert(ctx.esp==stack+4&&ctx.ecx==object&&ctx.ebx==14&&ctx.esi==15&&ctx.edi==16&&ctx.ebp==17);
    assert(R32(stack-0x100)==0x12345678&&R32(stack+0x100)==0x87654321);
}
static void CheckKernel(uint32_t expected){
    uint32_t args[]={7,0x21000,0x22000,4,0x21004};
    assert(QuerySettings(args)==0&&R32(0x21000)==4&&R32(0x22000)==expected&&R32(0x21004)==4);
    args[0]=0xffff;args[3]=256;
    assert(QuerySettings(args)==0&&R32(0x21000)==3&&R32(0x220e8)==expected&&R32(0x21004)==256);
    assert(R32(0x22058)==0x00400100&&R32(0x2202c)==1&&R32(0x220ec)==0x90000);
    args[0]=0x103;args[3]=4;QuerySettings(args);assert(R32(0x22000)==0x00400100);
}
static void CheckBank(unsigned language){
    // Four real-format blocks: key56, variants12, text8, phonemes4. Execute
    // original17790's language seek, block read and pointer fixups unchanged.
    bank.assign(24+4*80,0);
    auto word=[&](size_t at,uint32_t v){std::memcpy(bank.data()+at,&v,4);};
    word(0,(32u<<16)|4);word(4,56);word(8,12);word(12,8);word(16,4);word(20,1);
    for(unsigned i=0;i<4;++i){
        const size_t start=24+i*80;std::memcpy(bank.data()+start,"speech",7);
        bank[start+49]=1;bank[start+50]=1;word(start+52,0);
        word(start+56,0);word(start+60,0);word(start+64,1);
        bank[start+68]=uint8_t('A'+i);bank[start+76]=uint8_t(10+i);
    }
    W32(0x30000,0x30100);W32(0x30104,0xf001);W32(0x30108,0xf002);
    auto ctx=Context();const uint32_t args[]={0x30000,language,0x31000,0x31004,0};
    for(unsigned i=0;i<5;++i)W32(stack+4+i*4,args[i]);
    stream_position=0;OriginalBank(ctx,g_base);
    assert(ctx.esp==stack+24&&R32(0x31000)==0x40000&&R32(0x31004)==1);
    assert(stream_position==24+(language+1)*80);
    assert(R32(0x40034)==0x40038&&R32(0x40038)==0x40044&&R32(0x4003c)==0x4004c);
    assert(memory[R32(0x40038)]=='A'+language&&memory[R32(0x4003c)]==10+language);
    ctx=Context();ctx.esi=0x32000;W32(ctx.esp+0x28,language);W32(ctx.esi+0x10,5);
    OriginalMovieMask(ctx,g_base);assert(R32(0x32060)==1u+(1u<<(language+1)));
}
int main(){
    const uint32_t ids[]={1,4,6,3};
    for(unsigned language=0;language<4;++language){
        Seed();TrialLanguage=language;trial_language::Initialize();
        assert(trial_language::Snapshot()==language&&trial_language::Active()==language&&!trial_language::PendingRestart());
        CheckKernel(ids[language]);
        // Negative witness: merely reporting the requested console language
        // still makes the original NTSC path select English.
        auto ctx=Context();OriginalLanguage(ctx,g_base);assert(R32(object+0x28)==0);CheckAbi(ctx);
        // The same original initializer on PAL establishes the authored mapping.
        W32(object+0x28,0xffffffff);fixture_region=0x00800300;
        ctx=Context();OriginalLanguage(ctx,g_base);assert(R32(object+0x28)==language);CheckAbi(ctx);
        // Our exact constructor boundary supplies that index without changing AV.
        W32(object+0x28,0xffffffff);fixture_region=0x00400100;
        ctx=Context();SelectedLanguage(ctx,g_base);assert(R32(object+0x28)==language);CheckAbi(ctx);
        CheckBank(language);
        const uint32_t next=(language+1)%4;std::string message;
        assert(trial_language::Apply(next,message)&&TrialLanguage==next&&app::saved_language==next);
        assert(trial_language::PendingRestart()&&trial_language::Active()==language);
        CheckKernel(ids[language]); // pending choice never changes a live query
        ctx=Context();SelectedLanguage(ctx,g_base);assert(R32(object+0x28)==language);
        assert(trial_language::Apply(language,message)&&!trial_language::PendingRestart());
    }
    // Retail's unsupported Xbox language IDs use English, including Spanish5.
    for(uint32_t id:{0u,2u,5u,7u,9u,10u}){
        Seed();fixture_region=0x00800300;fixture_language=id;
        auto ctx=Context();OriginalLanguage(ctx,g_base);assert(R32(object+0x28)==0);
    }
    for(uint32_t invalid:{4u,5u,0xffffffffu}){
        TrialLanguage=invalid;trial_language::Initialize();assert(TrialLanguage==0&&trial_language::Active()==0);
        std::string message;const unsigned saves=app::saves;
        assert(!trial_language::Apply(invalid,message)&&!message.empty()&&app::saves==saves&&TrialLanguage==0);
    }
    TrialLanguage=1;trial_language::Initialize();std::string message;app::fail_save=true;
    const auto saved=app::saved_language;
    assert(!trial_language::Apply(3,message)&&TrialLanguage==1&&trial_language::Active()==1&&app::saved_language==saved);
    assert(!trial_language::PendingRestart());app::fail_save=false;
    for(unsigned bad=0;bad<5;++bad){
        Seed();auto ctx=Context();
        if(bad==0)W32(stack,0x12345);
        if(bad==1)W32(0x5096e0,object+0x100);
        if(bad==2)W32(object+0x28,2);
        if(bad==3)mem::denied=object;
        const auto before=ctx;const uint32_t cached=R32(object+0x28);
        TrialLanguageStartup(ctx,bad==4?nullptr:g_base);
        assert(!std::memcmp(&ctx,&before,sizeof(ctx))&&R32(object+0x28)==cached);
    }
    std::puts("language: NTSC negative witness, original PAL mapping, startup cache guard, all4 text/phoneme blocks and movie masks, consistent frozen EEPROM and Apply rollback passed");
}
