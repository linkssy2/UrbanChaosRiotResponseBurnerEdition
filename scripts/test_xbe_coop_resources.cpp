// Executes retail archive iteration/name records, particle lookup and arms dispatch.
// Resource decoding remains a supplied boundary and needs live qualification.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include "kernel/kernel.h"
namespace kernel {static std::array<uint8_t,0x800000> memory{};uint8_t* g_base=memory.data();}
namespace mem {bool IsCommitted(uint32_t p){return p>=0x10000&&p<kernel::memory.size();}}
#ifdef UC_BURNER_RELEASE
// Readiness and parsed-particle admission must survive removal of every sink.
#define fprintf ReleaseDiagnosticFprintfMustNotCompile
#define fflush ReleaseDiagnosticFflushMustNotCompile
#endif
#include "../common/xbe_coop_resources.cpp"
#ifdef UC_BURNER_RELEASE
#undef fprintf
#undef fflush
#endif
static unsigned turret_asset_resets=0;
void TrialCoopTurretAssetsReset(){++turret_asset_resets;}
using namespace kernel;
namespace {
constexpr uint32_t archive=0x600000,provider=0x601000,actor=0x602000,stack=0x7f0000;
const char* file="test-coop-resources-fixture.lst";
const char* injection_file="test-coop-injection-fixture.lst";
constexpr unsigned resource_count=std::size(roots)+std::size(effects);
constexpr unsigned injection_index=std::size(roots)-1;
constexpr uint32_t registry=0x51cb78,sentinel=0x630000;
unsigned index=0,calls=0,queries=0,seen_arms=0,archive_count=resource_count;
unsigned parsed_effects=all_effects;
bool injecting=false;
int missing=-1;
const char* ResourceName(unsigned i){return i<std::size(roots)?roots[i]:effects[i-std::size(roots)];}
void SupplyParticleRegistry(uint8_t* b){
    // The stream decoder is a supplied boundary. Its results must still pass
    // the original tree lookup, including the absent-entry branch.
    Write(b,registry,0x4830cc);Write(b,registry+8,sentinel);
    std::memset(b+sentinel,0,0x200);b[sentinel+0x191]=1;
    uint32_t first=sentinel,previous=0;
    // Alphabetic order: death_distortion then spawn01.
    for(int i=1;i>=0;--i){
        if(!(parsed_effects&(1u<<i))||int(std::size(roots)+i)==missing||
           std::size(roots)+i>=archive_count)continue;
        const uint32_t node=sentinel+0x200+uint32_t(i)*0x200;
        std::memset(b+node,0,0x200);
        Write(b,node,sentinel);Write(b,node+8,sentinel);
        Write(b,node+0x10,0x610000+uint32_t(std::size(roots)+i)*0x100);
        if(previous)Write(b,previous+8,node);else first=node;
        previous=node;
    }
    Write(b,sentinel+4,first);
}
}
[[noreturn]] void x86_trap(X86Context&,uint32_t,const char*){std::abort();}
extern "C" void sub_0023DE20(X86Context&,uint8_t*);
extern "C" void sub_0008DAC0(X86Context&,uint8_t*);
void x86_icall(X86Context& c,uint8_t* b,uint32_t fn){
    switch(fn){
    case 0x23de20:
        assert(c.ecx==registry);++queries;sub_0023DE20(c,b);return;
    case 0x54b70:{
        const auto* name=reinterpret_cast<char*>(b+Read(b,c.esp+4));
        injecting=!std::strcmp(name,"coopinjection");
        assert(injecting||!std::strcmp(name,"coopcharacters"));++calls;
        // Supply only the LST loader's archive/provider boundary. The caller
        // and its register-preserving return are the production CallGuest.
        c.esp-=16;Write(b,c.esp,0x54bdc);Write(b,c.esp+4,provider);
        Write(b,c.esp+8,0);Write(b,c.esp+12,0);c.ecx=archive;
        sub_0008DAC0(c,b);if(!injecting)SupplyParticleRegistry(b);
        c.eax=0xdead;_mm_setcsr(0x1fa0);return;
    }
    case 1:index=injecting?injection_index:0;c.esp+=4;break;
    case 2:c.eax=injecting?1:archive_count;c.esp+=4;break;
    case 3:assert(injecting?index==injection_index:index<archive_count);c.eax=0x610000+index*0x100;c.esp+=4;break;
    case 4:{
        const uint32_t record=Read(b,c.esp+4);
        assert(Read(b,record)==0x610000+index*0x100&&Read(b,record+4)==0);
        // Extension callback returns true even when the particle parse failed.
        c.eax=index>=std::size(roots)?1:(int(index)==missing||(!injecting&&index==injection_index)?0:0x620000+index*16);
        ++index;c.esp+=8;break;
    }
    default:std::abort();
    }
}
X86_FUNC(sub_00064EA0){ctx.esp+=16;}
X86_FUNC(sub_003878F0){
    ctx.eax=uint32_t(std::strcmp(reinterpret_cast<char*>(base+Read(base,ctx.esp+4)),
                                reinterpret_cast<char*>(base+Read(base,ctx.esp+8))));
    ctx.esp+=4;
}
X86_FUNC(sub_000C87B0){
    assert(Read(base,ctx.esp+4)==0x51be00&&Read(base,ctx.esp+8)==0);
    seen_arms=Read(base,0x51be00);ctx.esp+=12;
}
// RETAIL_RESOURCE_FUNCTIONS
namespace kernel {
std::string ResolvePath(const std::string& path,uint32_t root){
    assert(root==0);
    if(path=="D:\\coopinjection.lst")return injection_file;
    assert(path=="D:\\coopcharacters.lst");return file;
}
// PRODUCTION_CALL_GUEST
}
int main(){
    assert(!std::filesystem::exists(file)&&!std::filesystem::exists(injection_file));
    struct Cleanup{~Cleanup(){std::filesystem::remove(file);std::filesystem::remove(injection_file);}}cleanup;
    auto* b=g_base;Write(b,archive,archive+0x100);Write(b,archive+0x104,2);
    Write(b,archive+0x108,1);Write(b,archive+0x10c,3);
    Write(b,provider,provider+0x100);Write(b,provider+0x10c,4);
    Write(b,actor,0x46fbe8);Write(b,0x51be00,0x450e08);
    for(unsigned i=0;i<resource_count;++i)std::strcpy(reinterpret_cast<char*>(b+0x610000+i*0x100),ResourceName(i));
    // A readable stale EAX must never masquerade as a successful lookup.
    X86Context c{};c.esp=stack;c.ecx=actor;c.eax=actor+1;const auto saved=c;
    trial_coop::Reset();TrialCoopPreload(c,b);assert(!calls&&!trial_coop::ResourcesReady());
    assert(turret_asset_resets==1);
    assert(trial_coop::Request(trial_coop::Character::Avenger,trial_coop::Character::TZero));
    TrialCoopPreload(c,b);assert(!calls&&!trial_coop::ResourcesReady());
    {std::ofstream out(file,std::ios::binary);out<<"short";}
    TrialCoopPreload(c,b);assert(!calls);
    {std::ofstream out(file,std::ios::binary);const std::array<char,0xc8> header{};out.write(header.data(),header.size());}
    TrialCoopPreload(c,b);assert(!calls&&!trial_coop::ResourcesReady());
    {std::ofstream out(injection_file,std::ios::binary);const std::array<char,0xc8> header{};out.write(header.data(),header.size());}
    // Negative witness: the old archive had all six bodies but no lifecycle FX.
    archive_count=6;TrialCoopPreload(c,b);assert(calls==2&&!queries&&!trial_coop::ResourcesReady());
    // The prior character/effect pack still omitted the third-person weapon meshes.
    archive_count=9;TrialCoopPreload(c,b);assert(!queries&&!trial_coop::ResourcesReady());
    archive_count=resource_count;
    for(unsigned i=0;i<resource_count;++i){
        missing=int(i);TrialCoopPreload(c,b);assert(!trial_coop::ResourcesReady());
    }
    missing=-1;
    for(unsigned mask=0;mask<all_effects;++mask){
        parsed_effects=mask;TrialCoopPreload(c,b);assert(!trial_coop::ResourcesReady());
    }
    const unsigned csr=_mm_getcsr();parsed_effects=all_effects;TrialCoopPreload(c,b);
    assert(queries&&trial_coop::ResourcesReady()&&!std::memcmp(&c,&saved,sizeof c)&&_mm_getcsr()==csr);
    for(const uint32_t stale:{0u,actor+1,0xdeadbeefu}){
        c=saved;c.eax=stale;const auto before=c;
        parsed_effects=0;TrialCoopPreload(c,b);assert(!trial_coop::ResourcesReady());
        parsed_effects=all_effects;TrialCoopPreload(c,b);assert(trial_coop::ResourcesReady());
        assert(!std::memcmp(&c,&before,sizeof c));
    }
    c=saved;
    sub_00172370(c,b);assert(seen_arms==0x45bf50&&Read(b,0x51be00)==0x450e08&&c.esp==stack+4);
    c=saved;Write(b,actor+0x368,1);sub_00172370(c,b);assert(seen_arms==0x450e08&&Read(b,0x51be00)==0x450e08);
    c=saved;Write(b,actor+0x368,0);
    {TrialCoopArmsScope outer(c,b);assert(Read(b,0x51be00)==0x45bf50);
        {TrialCoopArmsScope inner(c,b);}assert(Read(b,0x51be00)==0x45bf50);}
    assert(Read(b,0x51be00)==0x450e08);
    for(unsigned value=1;value<=4;++value){
        trial_coop::Reset();assert(trial_coop::Request(trial_coop::Character::TZero,static_cast<trial_coop::Character>(value)));
        trial_coop::ResourcesReady(true);Write(b,actor+0x368,1);c=saved;sub_00172370(c,b);assert(seen_arms==0x45bf50);
    }
    trial_coop::ResourcesReady(false);c=saved;sub_00172370(c,b);assert(seen_arms==0x450e08);
    trial_coop::Reset();c=saved;sub_00172370(c,b);assert(seen_arms==0x450e08);
    puts("Co-op resources: production CallGuest return/restore ABI, stale readable/null EAX isolation, retail archive and particle lookup, old-archive rejection, every missing root/parse, context restore, character arms and nested restore passed.");
}
