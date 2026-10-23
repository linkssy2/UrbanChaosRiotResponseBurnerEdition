// Original pool bodies and production retention hooks. Guest allocation,
// ragdoll construction/activation, world attachment and destruction are modeled
// boundaries; renderer-list insertion/removal is modeled too. This qualifies
// ownership/ABI and admission, not solver or live visual parity.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <unordered_map>
#include <vector>
#include "kernel/kernel.h"
namespace kernel {static std::array<uint8_t,0x1000000> memory{};uint8_t* g_base=memory.data();}
using namespace kernel;
namespace mem {bool IsCommitted(uint32_t page){return page<kernel::memory.size();}}
namespace trial_graphics {static uint32_t mode;uint32_t RagdollMode(){return mode;}}
#include "../common/xbe_ragdoll_settings.cpp"
static constexpr uint32_t kStack=0xf00000,kFs=0x600000,kLists=0x610000,kJoints=0x620000;
static constexpr uint32_t kManager=0x630000,kWorld=0x640000,kVtable=0x650000,kBody=0x700000;
static unsigned allocated,destroyed,attached,activated,deactivated;
static bool fail_alloc,setup_released,clobber_host_call;
static bool raw_object_pool,raw_object_query,expect_trap,fail_object_alloc;
static uint32_t object_heap=0x800000;
static std::unordered_map<uint32_t,uint32_t> object_allocations;
static unsigned object_frees;
static unsigned action_destroyed;
static constexpr uint32_t kRenderer=0x6a0000,kRenderList=0x6b0000;
static std::unordered_map<uint32_t,uint32_t> render_wrappers;
struct Activation {uint32_t body;std::array<uint32_t,6> args;};
static std::vector<Activation> activations;
X86_EXTERN_FUNC(sub_002A5030);X86_EXTERN_FUNC(sub_002A5070);X86_EXTERN_FUNC(sub_002A5D50);
X86_EXTERN_FUNC(sub_002A6790);X86_EXTERN_FUNC(sub_002A6850);X86_EXTERN_FUNC(sub_002A6A70);
X86_EXTERN_FUNC(sub_0029E4F0);
X86_EXTERN_FUNC(sub_002383C0);X86_EXTERN_FUNC(sub_00238430);X86_EXTERN_FUNC(original_00238430);
X86_EXTERN_FUNC(sub_00238440);X86_EXTERN_FUNC(sub_002384C0);X86_EXTERN_FUNC(sub_002384F0);
X86_EXTERN_FUNC(original_002384C0);
X86_EXTERN_FUNC(sub_00089EB0);X86_EXTERN_FUNC(sub_00089F90);X86_EXTERN_FUNC(sub_00089FF0);X86_EXTERN_FUNC(sub_0008A100);
X86_EXTERN_FUNC(sub_00089AB0);X86_EXTERN_FUNC(sub_00089E80);X86_EXTERN_FUNC(sub_00089840);
X86_EXTERN_FUNC(sub_002BA100);X86_EXTERN_FUNC(sub_002BA020);X86_EXTERN_FUNC(sub_002B9F40);
X86_EXTERN_FUNC(sub_002B9130);X86_EXTERN_FUNC(sub_002B8C00);X86_EXTERN_FUNC(sub_002B9080);X86_EXTERN_FUNC(sub_002B90B0);
X86_EXTERN_FUNC(sub_002B8650);X86_EXTERN_FUNC(sub_002B8B70);X86_EXTERN_FUNC(sub_002B8BA0);
[[noreturn]] void x86_trap(X86Context&,uint32_t,const char*){if(expect_trap)throw std::runtime_error("expected trap");std::abort();}
void x86_resume(X86Context&,uint8_t*,const X86Resume&){std::abort();}
X86_FUNC(sub_002D57FB){
    assert(X86_MEM32(ctx.esp+4)==0xa0);ctx.eax=fail_alloc?0:kBody+256*allocated++;ctx.esp+=4;
}
static bool LiveLists(uint32_t bodies,uint32_t joints){
    return (!setup_released&&bodies==kLists&&joints==kJoints)||
        (bodies==R32(0x51da48)+0x70&&joints==R32(0x51da48)+0x7c);
}
X86_FUNC(sub_002A6130){
    assert(X86_MEM32(ctx.esp+4)==kWorld&&LiveLists(X86_MEM32(ctx.esp+8),X86_MEM32(ctx.esp+12)));
    W32(ctx.ecx,kVtable);W32(ctx.ecx+0x70,ctx.ecx+0xb0);W32(ctx.ecx+0x74,2);
    W32(ctx.ecx+0x7c,ctx.ecx+0xc0);W32(ctx.ecx+0x80,1);W32(ctx.ecx+0x88,kWorld);
    W32(ctx.ecx+0xb0,ctx.ecx+0xd0);W32(ctx.ecx+0xb4,ctx.ecx+0xe0);
    ctx.eax=ctx.ecx;ctx.esp+=16;
}
X86_FUNC(sub_002A5F00){
    assert(ctx.ecx>=kBody&&ctx.ecx<kBody+allocated*256);
    Activation a{ctx.ecx,{}};for(unsigned i=0;i<6;++i)a.args[i]=X86_MEM32(ctx.esp+4+i*4);
    activations.push_back(a);if(a.args[0])++activated;else ++deactivated;ctx.esp+=28;
}
X86_FUNC(sub_002EEFF0){
    assert(ctx.ecx==kWorld&&X86_MEM32(ctx.esp+8)==0&&X86_MEM32(ctx.esp+12)==1);++attached;ctx.esp+=16;
}
X86_FUNC(sub_002D83F8){
    const uint32_t size=X86_MEM32(ctx.esp+4);ctx.esp+=4;
    if(fail_object_alloc){ctx.eax=0;return;}
    object_heap=(object_heap+31)&~15u;ctx.eax=object_heap;object_heap+=size;
    assert(object_heap<kStack-0x10000);assert(object_allocations.emplace(ctx.eax,size).second);
    // Original83B5D returns header+10; header+0C retains the free-list
    // backlink. An exhausted fixed pool reads precisely this array[-1].
    W32(ctx.eax-4,0x530180);std::memset(base+ctx.eax,0,size);
}
X86_FUNC(sub_002D73A4){
    const uint32_t object=X86_MEM32(ctx.esp+4);assert(object_allocations.erase(object)==1);++object_frees;ctx.esp+=4;
}
X86_FUNC(sub_00011030){assert(X86_MEM32(ctx.esp+4));ctx.esp+=4;}
X86_FUNC(sub_000958B0){
    const uint32_t object=X86_MEM32(ctx.esp+4);
    assert(ctx.ecx==kRenderList&&object&&X86_MEM32(ctx.esp+8)==0);
    assert(render_wrappers.emplace(object,R32(object+4)).second);ctx.esp+=12;
}
X86_FUNC(sub_00095090){
    assert(ctx.ecx==kRenderList&&render_wrappers.erase(X86_MEM32(ctx.esp+4))==1);ctx.esp+=8;
}
void x86_icall(X86Context& ctx,uint8_t* base,uint32_t fn){
    switch(fn){
    case 0x2d57fb:sub_002D57FB(ctx,base);break;
    case 0x29e4f0:sub_0029E4F0(ctx,base);break;
    case 0x2a6130:sub_002A6130(ctx,base);break;
    case 0x2a5f00:sub_002A5F00(ctx,base);break;
    case 0x2eeff0:sub_002EEFF0(ctx,base);break;
    case 0x2d83f8:sub_002D83F8(ctx,base);break;
    case 0x2d73a4:sub_002D73A4(ctx,base);break;
    case 0x238430:if(raw_object_pool)original_00238430(ctx,base);else sub_00238430(ctx,base);break;
    case 0x238440:sub_00238440(ctx,base);break;
    case 0x2384c0:if(raw_object_query)original_002384C0(ctx,base);else sub_002384C0(ctx,base);break;
    case 0x89e80:sub_00089E80(ctx,base);break;
    case 0x2b8650:sub_002B8650(ctx,base);break;
    case 0xf1000020:ctx.eax=0;ctx.esp+=4;break; // Source action needs a CombinerAction wrapper.
    case 0xf1000024:assert(X86_MEM32(ctx.esp+4)==1);++action_destroyed;ctx.esp+=8;break;
    case 0xf1000010:ctx.eax=0x12345678;ctx.esp+=8;break; // Registered controller token.
    case 0xf1000000:assert(X86_MEM32(ctx.esp+4)==1);++destroyed;ctx.esp+=8;break;
    default:std::abort();
    }
    if(clobber_host_call){
        ctx.ecx=ctx.edx=ctx.ebx=ctx.esi=ctx.edi=ctx.ebp=0xbad;
        ctx.xmm[6].u[2]=0xdeadbeef;ctx.cf=1;ctx.esp-=32;_mm_setcsr(0x7f80);
    }
}
// GENERATED_ORIGINAL

using Function=void(*)(X86Context&,uint8_t*);
static uint32_t Run(Function f,std::initializer_list<uint32_t> args={}){
    X86Context c{};c.esp=kStack;c.fs_base=kFs;c.ebx=101;c.ebp=102;c.esi=103;c.edi=104;
    c.xmm[6].u[2]=0x1234;W32(kStack,0xfeefee);unsigned i=1;for(auto a:args)W32(kStack+i++*4,a);
    f(c,g_base);assert(c.esp==kStack+4&&c.ebx==101&&c.ebp==102&&c.esi==103&&c.edi==104);
    assert(c.fs_base==kFs&&R32(kFs)==0xffffffff&&c.xmm[6].u[2]==0x1234);return c.eax;
}
static uint32_t Acquire(){return Run(sub_002A6790,{1,2,3,4,5,6});}
static uint32_t Lookup(uint32_t id){return Run(sub_002A5070,{id});}
static void Setup(uint32_t mode){
    assert(extra.empty());memory.fill(0);trial_graphics::mode=mode;setup_released=false;
    fail_alloc=false;allocated=destroyed=attached=activated=deactivated=0;activations.clear();
    W32(kFs,0xffffffff);W32(kVtable,0xf1000000);
    W32(0x51d910,kManager);W32(kManager+0x10,kWorld);
    // GENERATED_CONSTANTS
    Run(sub_002A6A70,{kLists,kJoints});assert(allocated==5&&G<uint8_t>(0x51da6c)==1);
    // The initial descriptors are no longer owned after mission setup. Growth
    // must clone each body's retained asset lists, never these stale pointers.
    std::memset(g_base+kLists,0xdd,0x20000);setup_released=true;W32(0x51da70,0);
}
static bool OriginalUnseen(float seconds){
    X86Context ctx{};auto* base=g_base;ctx.esi=0x660000;G<float>(ctx.esi+0x10)=seconds;
    X86_LOAD_FLAGS();uint32_t ca=0,cb=0;
    // GENERATED_UNSEEN
    return true;
loc_0003CBE5:return false;
}
static uint32_t OriginalRetire(uint32_t original,uint32_t dead){
    X86Context ctx{};ctx.ebp=original;ctx.edx=dead;auto* base=g_base;
    X86_LOAD_FLAGS();uint32_t ca=0,cb=0;
    // GENERATED_RETIRE
    return ctx.ebp;
loc_0003CC85:return 0;
}
static bool OriginalAge(float seconds,bool missing){
    X86Context ctx{};auto* base=g_base;ctx.xmm[0].f[0]=seconds;ctx.eax=missing;ctx.ebx=0;
    X86_LOAD_FLAGS();uint32_t ca=0,cb=0;
    // GENERATED_AGE
loc_000E7960:return true;
loc_000E796F:return false;
}
static uint32_t ObjectCall(Function f,uint32_t owner,std::initializer_list<uint32_t> args={},unsigned pop=4){
    X86Context c{};c.esp=kStack;c.fs_base=kFs;c.ecx=owner;c.ebx=101;c.ebp=102;c.esi=103;c.edi=104;
    W32(kStack,0xfeefee);unsigned i=1;for(auto a:args)W32(kStack+i++*4,a);
    f(c,g_base);assert(c.esp==kStack+pop&&c.ebx==101&&c.ebp==102&&c.esi==103&&c.edi==104);
    assert(R32(kFs)==0xffffffff);return c.eax;
}
static constexpr uint32_t kObjectPool=0x690000,kObjectPool2=0x690100;
static void ObjectsSetup(uint32_t mode){
    assert(extra.empty()&&object_pools.empty());memory.fill(0);object_allocations.clear();object_frees=0;
    object_heap=0x800000;trial_graphics::mode=mode;raw_object_pool=raw_object_query=false;fail_object_alloc=false;
    W32(kFs,0xffffffff);W32(0x44bf94,0x3f800000);
    W32(0x482e98,0x238430);W32(0x482e9c,0x238440);W32(0x51dc94,kObjectPool);
    W32(0x5150b0,kManager);W32(kManager,kVtable);W32(kVtable+0x80,0xf1000010);
    // Exact3AAC90 profile: 80-byte ManualBlobShadow objects,15 fixed slots.
    ObjectCall(sub_002383C0,kObjectPool,{0x48f4f0,0x50,15},16);
}
static uint32_t Controller(){return ObjectCall(sub_002BA100,0,{0x700000,0x43020000,0x42c80000});}
static void ObjectPoolRegression(){
    ObjectsSetup(0);raw_object_pool=true;
    for(unsigned i=0;i<15;++i)assert(Controller());
    assert(R32(kObjectPool+20)==0&&ObjectCall(sub_002384C0,kObjectPool)==1);
    // Execute the original Body::ManualBlobShadow factory and constructors
    // with a modeled legitimate stale heap-header backlink. The sparse dump
    // captures the poisoned bin, not this pool's count or the first writer.
    assert(Controller()==0x530180&&R32(kObjectPool+20)==UINT32_MAX);
    assert(R32(0x530194)==0x43020000);
    ObjectCall(sub_002384F0,kObjectPool);assert(object_allocations.empty());
    for(uint32_t mode:{0u,1u,2u}){
        ObjectsSetup(mode);std::vector<uint32_t> created;
        const uint32_t storage=R32(kObjectPool+24),free_list=R32(kObjectPool+16);
        W32(0x530194,0x530190);
        for(unsigned i=0;i<15;++i)created.push_back(Controller());
        // Checked factories retain the original cap, without any allocation.
        assert(ObjectCall(sub_002384C0,kObjectPool)==1&&object_allocations.size()==2);
        created.push_back(Controller());assert(object_allocations.size()==3);
        assert(ObjectCall(sub_002384C0,kObjectPool)==1);
        for(unsigned i=16;i<48;++i)created.push_back(Controller());
        assert(object_pools.at(kObjectPool).slots.size()==33&&R32(kObjectPool+20)==0);
        assert(R32(kObjectPool+24)==storage&&R32(kObjectPool+16)==free_list);
        assert(R32(0x530194)==0x530190);
        for(auto p:created){assert((p&3)==0&&R32(p)==0x48f508&&R32(p+20)==0x43020000);}
        trial_graphics::mode=0;
        ObjectCall(sub_00238440,kObjectPool,{created[15]},8);
        assert(Controller()==created[15]); // Existing overflow remains owned after switching toDefault.
        for(auto p:created)ObjectCall(sub_00238440,kObjectPool,{p},8);
        assert(R32(kObjectPool+20)==15);
        assert(ObjectCall(sub_00238430,kObjectPool)==created[14]);
        ObjectCall(sub_00238440,kObjectPool,{created[14]},8);
        // An unrelated instance of the generic class keeps its exhaustion
        // result. Only the exact renderer-wrapper pool changes admission.
        ObjectCall(sub_002383C0,kObjectPool2,{0x48f4f0,0x50,1},16);trial_graphics::mode=0;
        auto p2=ObjectCall(sub_00238430,kObjectPool2);
        const auto before_query=object_allocations.size();
        assert(ObjectCall(sub_002384C0,kObjectPool2)==1&&object_allocations.size()==before_query);
        assert(!object_pools.count(kObjectPool2));
        // A newly encountered pool can exhaust after switching to Default
        // while retained objects from the prior setting still exist.
        auto p2extra=ObjectCall(sub_00238430,kObjectPool2);
        assert(p2extra!=p2&&ObjectCall(sub_002384C0,kObjectPool2)==1);
        ObjectCall(sub_00238440,kObjectPool2,{p2extra},8);
        ObjectCall(sub_00238440,kObjectPool2,{p2},8);
        ObjectCall(sub_002384F0,kObjectPool2);
        ObjectCall(sub_002384F0,kObjectPool);
        assert(object_allocations.empty()&&object_pools.empty());
        // Pool-object addressreuse and teardown with outstanding raw slots
        // follow the original storage-destructor contract, exactlyonce.
        ObjectCall(sub_002383C0,kObjectPool,{0x48f4f0,0x50,15},16);
        for(unsigned i=0;i<16;++i)ObjectCall(sub_00238430,kObjectPool);
        ObjectCall(sub_002384F0,kObjectPool);assert(object_allocations.empty()&&object_pools.empty());
    }
    ObjectsSetup(2);for(unsigned i=0;i<15;++i)Controller();
    fail_object_alloc=true;expect_trap=true;bool trapped=false;
    try{Controller();}catch(const std::runtime_error&){trapped=true;}
    expect_trap=false;W32(kFs,0xffffffff); // OOM is fatal in production, so no guest SEH continuation is expected.
    assert(trapped&&R32(kObjectPool+20)==0&&object_pools.at(kObjectPool).slots.empty());
    fail_object_alloc=false;ObjectCall(sub_002384F0,kObjectPool);assert(object_allocations.empty()&&object_pools.empty());
}
static void LayerSetup(){
    ObjectsSetup(2);ObjectCall(sub_002384F0,kObjectPool);
    // Original3AAC70 and3AAC50 profiles, including the nested helper pool.
    ObjectCall(sub_002383C0,kObjectPool,{0x48f3f0,0x20,225},16);
    ObjectCall(sub_002383C0,kObjectPool2,{0x48f3d8,0x14,60},16);
    W32(0x51dc90,kObjectPool);W32(0x51dc8c,kObjectPool2);
    W32(0x48f3b8,0x2b8650);W32(0x680000,0xf1000024);W32(0x680014,0xf1000020);
    action_destroyed=0;
}
static uint32_t Layer(uint32_t action=0){return ObjectCall(sub_002B9130,0,{action});}
static void LayerRegression(){
    LayerSetup();raw_object_pool=true;
    for(unsigned i=0;i<225;++i)assert(Layer());
    // Model the stale array[-1] backlink using the latest dump's poisoned
    // free-list address. Constructor2B8C2A explains its distinctive vtable;
    // it does not write+4, so this witness deliberately preserves that word.
    W32(R32(kObjectPool+16)-4,0x58d350);W32(0x58d350,0x530180);W32(0x58d354,0x12345678);
    assert(Layer()==0x58d350&&R32(kObjectPool+20)==UINT32_MAX);
    assert(R32(0x58d350)==0x48f41c&&R32(0x58d354)==0x12345678);
    ObjectCall(sub_002384F0,kObjectPool);ObjectCall(sub_002384F0,kObjectPool2);
    assert(object_allocations.empty());

    LayerSetup();std::vector<uint32_t> layers;
    const auto storage=R32(kObjectPool+24),free_list=R32(kObjectPool+16);
    W32(free_list-4,0x58d350);W32(0x58d350,0x530180);W32(0x58d354,0);
    auto make=[&](unsigned index){
        const uint32_t action=0x681000+index*16;W32(action,0x680000);
        const auto layer=Layer(action);assert(R32(layer)==0x48f41c);
        const auto helper=R32(layer+0x0c);assert(helper&&R32(helper)==0x48f3b8&&R32(helper+8)==action);
        layers.push_back(layer);
    };
    for(unsigned i=0;i<260;++i){
        if(i==223)trial_graphics::mode=0; // First Layer overflow occurs inDefault.
        make(i);
    }
    assert(R32(kObjectPool+24)==storage&&R32(kObjectPool+16)==free_list);
    assert(R32(0x58d350)==0x530180&&R32(0x58d354)==0);
    assert(object_pools.at(kObjectPool).slots.size()==35);
    assert(object_pools.at(kObjectPool2).slots.size()==200);
    assert(ObjectCall(sub_002384C0,kObjectPool)==1&&ObjectCall(sub_002384C0,kObjectPool2)==1);
    const auto allocated_before_reuse=object_allocations.size();
    // Use actual deleting destructors, including the helper's secondary-vtable
    // this-adjustment and return to its own pool, rather than a modeled free.
    for(unsigned i=0;i<40;++i){ObjectCall(sub_002B9080,layers.back(),{1},8);layers.pop_back();}
    for(unsigned i=260;i<300;++i)make(i);
    assert(object_allocations.size()==allocated_before_reuse);
    for(auto p:layers)ObjectCall(sub_002B9080,p,{1},8);
    assert(action_destroyed==300&&R32(kObjectPool+20)==225&&R32(kObjectPool2+20)==60);
    W32(0x51dc90,0);W32(0x51dc8c,0); // Teardown follows registered owners.
    ObjectCall(sub_002384F0,kObjectPool);ObjectCall(sub_002384F0,kObjectPool2);
    assert(object_allocations.empty()&&object_pools.empty());
}
static void OtherPoolProfiles(){
    // Constructor bytes for each original profile are pinned by preparation.
    struct Profile{uint32_t name,stride,capacity;};
    for(uint32_t mode:{0u,1u,2u})for(const auto p:{Profile{0x48f0d8,8,15},Profile{0x48e28c,0x80,15},
        Profile{0x48eff0,0x18,150},Profile{0x4830d4,0x140,100},Profile{0x455dd8,0x10,64}}){
        ObjectsSetup(mode);ObjectCall(sub_002384F0,kObjectPool);
        ObjectCall(sub_002383C0,kObjectPool,{p.name,p.stride,p.capacity},16);
        std::vector<uint32_t> objects;
        for(unsigned i=0;i<p.capacity;++i){
            assert(ObjectCall(sub_002384C0,kObjectPool)==0);
            objects.push_back(ObjectCall(sub_00238430,kObjectPool));
        }
        const auto count=object_allocations.size();
        assert(ObjectCall(sub_002384C0,kObjectPool)==1&&object_allocations.size()==count);
        objects.push_back(ObjectCall(sub_00238430,kObjectPool));
        assert(ObjectCall(sub_002384C0,kObjectPool)==1);
        for(auto object:objects)ObjectCall(sub_00238440,kObjectPool,{object},8);
        assert(R32(kObjectPool+20)==p.capacity);
        ObjectCall(sub_002384F0,kObjectPool);assert(object_allocations.empty()&&object_pools.empty());
    }
}
static void RendererSetup(uint32_t mode){
    assert(render_wrappers.empty());ObjectsSetup(mode);ObjectCall(sub_002384F0,kObjectPool);
    // Original3A79A0 profile; the four byte-pinned factories share this owner.
    ObjectCall(sub_002383C0,kObjectPool,{0x455dc0,16,256},16);W32(0x519f64,kObjectPool);
    W32(0x482ea0,0x2384c0);W32(kRenderer+0x48,kRenderList);
    for(uint32_t table:{0x455f20u,0x455ff0u,0x455eb8u,0x4560c0u})W32(table,0x89e80);
}
static void QueryAbi(uint32_t owner,bool expected){
    X86Context original{},candidate{};original.ecx=owner;original.esp=kStack;
    original.eax=0xabcdef00;original.edx=0x1234;original.cf=original.zf=1;
    original.xmm[6].u[2]=0x12345678;candidate=original;
    original_002384C0(original,g_base);sub_002384C0(candidate,g_base);
    assert(candidate.eax==uint32_t(expected));candidate.eax=original.eax;
    assert(!std::memcmp(&original,&candidate,sizeof(original)));
}
static void RendererAdmissionRegression(){
    struct Factory{Function call;uint32_t table;};
    const Factory factories[]={{sub_00089EB0,0x455f20},{sub_00089F90,0x455ff0},
        {sub_00089FF0,0x455eb8},{sub_0008A100,0x4560c0}};
    for(uint32_t mode:{0u,1u,2u}){
        RendererSetup(mode);raw_object_query=true;std::vector<uint32_t> created;
        auto make=[&](unsigned i){
            const auto& f=factories[i%4];const auto source=0x6c0000+i*4;
            const auto object=ObjectCall(f.call,kRenderer,{source},8);
            if(object){assert(R32(object)==f.table&&R32(object+4)==source);
                assert(render_wrappers.at(object)==source);}
            return object;
        };
        for(unsigned i=0;i<256;++i)created.push_back(make(i));
        assert(render_wrappers.size()==256&&R32(kObjectPool+20)==0);
        const auto allocated_before=object_allocations.size();
        // Original checked factories drop all four render kinds once full.
        for(unsigned i=256;i<260;++i)assert(!make(i));
        assert(object_allocations.size()==allocated_before&&render_wrappers.size()==256);
        raw_object_query=false;QueryAbi(kObjectPool,false);
        for(unsigned i=256;i<320;++i){const auto p=make(i);assert(p);created.push_back(p);}
        assert(render_wrappers.size()==320&&object_pools.at(kObjectPool).slots.size()==64);
        assert(R32(kObjectPool+20)==0);
        const auto allocated_after=object_allocations.size();
        // Exact original unlink + deleting destructor returns overflow slots.
        for(unsigned i=0;i<32;++i){ObjectCall(sub_00089AB0,kRenderer,{created.back()},8);created.pop_back();}
        for(unsigned i=320;i<352;++i){const auto p=make(i);assert(p);created.push_back(p);}
        assert(object_allocations.size()==allocated_after&&render_wrappers.size()==320);
        // Same layout but a different owner, malformed profile, or different
        // capacity must still report full without allocating anything.
        W32(0x519f64,kObjectPool2);QueryAbi(kObjectPool,true);W32(0x519f64,kObjectPool);
        for(auto field:{std::array<uint32_t,2>{0,0x482e94},std::array<uint32_t,2>{8,16},std::array<uint32_t,2>{12,256}}){
            W32(kObjectPool+field[0],field[1]+1);QueryAbi(kObjectPool,true);W32(kObjectPool+field[0],field[1]);
        }
        assert(object_allocations.size()==allocated_after);
        ObjectCall(sub_00089AB0,kRenderer,{0},8); // Original null cleanup remains safe.
        for(auto p:created)ObjectCall(sub_00089AB0,kRenderer,{p},8);
        assert(render_wrappers.empty()&&R32(kObjectPool+20)==256);QueryAbi(kObjectPool,false);
        ObjectCall(sub_002384F0,kObjectPool);assert(object_allocations.empty()&&object_pools.empty());
        // A new scene may rebuild the same pool object address.
        RendererSetup(mode);for(unsigned i=0;i<257;++i)assert(make(i));
        std::vector<uint32_t> remaining;for(const auto& p:render_wrappers)remaining.push_back(p.first);
        for(auto p:remaining)ObjectCall(sub_00089AB0,kRenderer,{p},8);
        ObjectCall(sub_002384F0,kObjectPool);assert(object_allocations.empty()&&object_pools.empty());
    }
}
int main(){
    // Unlike the runtime CallGuest convenience API, this bridge also restores
    // flags, XMM and host rounding state after a gameplay-time clone callback.
    X86Context c{};c.esp=kStack;c.ebx=123;c.xmm[6].u[2]=42;const auto saved=c;
    const unsigned mxcsr=_mm_getcsr();clobber_host_call=true;
    assert(Call(c,g_base,0x2d57fb,0,{0xa0},0x2a6ab0)==kBody);
    assert(!std::memcmp(&c,&saved,sizeof(c))&&_mm_getcsr()==mxcsr);clobber_host_call=false;
    for(uint32_t mode=0;mode<3;++mode){
        Setup(mode);
        for(uint32_t id=0;id<80;++id){assert(Acquire()==id&&Lookup(id));if(mode==2)assert(Lookup(0));}
        assert(Count()==(mode==0?5:mode==1?32:80));
        assert(allocated==Count()&&activated==80);
        for(const auto& a:activations)assert((a.args==std::array<uint32_t,6>{1,2,3,4,6,5}));
        if(mode!=2)assert(!Lookup(0));else assert(Lookup(0));
        Run(sub_002A5D50);assert(attached==2*Count());
        assert(!Lookup(UINT32_MAX));
        assert(TrialRagdollSeconds()==(mode==2?INFINITY:mode==1?120.f:30.f));
        assert(!OriginalAge(30,false));assert(OriginalAge(31,false)==(mode==0));
        assert(OriginalAge(121,false)==(mode!=2));assert(OriginalAge(0,true));
        assert(!OriginalUnseen(1));assert(OriginalUnseen(121)==(mode!=2));
        assert(OriginalUnseen(30)==(mode==0));
        assert(OriginalRetire(20,40)==(mode==0?20:mode==1?8:0));
        assert(OriginalRetire(10,8)==(mode==0?8:0));
        if(mode==2){
            fail_alloc=true;assert(Acquire()==UINT32_MAX&&Lookup(0));fail_alloc=false;
            const auto count=Count();Run(sub_002A6850,{20});assert(!Lookup(20)&&deactivated==1);
            assert(Acquire()==80&&Count()==count&&Lookup(80));
            assert((activations[80].args==std::array<uint32_t,6>{0,0,0,0,0,0}));
            // Applying a smaller limit does not invalidate existing extra IDs;
            // explicit release/update/destruction still see every owned slot.
            trial_graphics::mode=0;assert(Lookup(79));assert(Acquire()==81&&Lookup(79)&&Count()==80);
            Run(sub_002A6850,{79});assert(!Lookup(79));
        }
        const auto count=Count();Run(sub_002A5030);assert(destroyed==count&&extra.empty()&&!G<uint8_t>(0x51da6c));
        Run(sub_002A5030);assert(destroyed==count);
    }
    // Default still follows the original unsigned oldest-ID behaviour at wrap.
    Setup(0);W32(0x51da70,UINT32_MAX-1);assert(Acquire()==UINT32_MAX-1);assert(Acquire()==UINT32_MAX);
    assert(!Lookup(UINT32_MAX));assert(Acquire()==0&&Lookup(0));Run(sub_002A5030);
    ObjectPoolRegression();
    LayerRegression();OtherPoolProfiles();RendererAdmissionRegression();
    std::puts("Ragdoll retention passed: original shadow/Layer underflow witnesses, renderer admission beyond256 through all four original factories, original unlink/destructor reuse, other capacity queries, all modes, cleanup and OOM passed");
}
