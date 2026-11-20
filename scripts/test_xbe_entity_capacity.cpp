// Compose emitted registration, lookup, retirement and pool operations with a checked heap.
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>
#include <vector>
#include "xbe_entity_registry.h"
#include "xbe_entity_pool.h"

alignas(16) static std::array<uint8_t,0x2000000> memory{};
static std::map<uint32_t,uint32_t> allocations;
static uint32_t bump=0x1000000;
static unsigned checks=0,guest_calls=0;
#define CHECK(x) do { ++checks; if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);std::exit(2);} }while(0)
namespace mem { bool IsCommitted(uint32_t address) { return address>=0x10000&&address<memory.size(); } }
struct GuestAccess { uint32_t address; };
static void* Map(uint32_t address,size_t size) {
    if(address>=0x10000&&uint64_t(address)+size<=memory.size()) return memory.data()+address;
    std::fprintf(stderr,"Invalid guest access: %08x (%zu bytes)\n",address,size);throw GuestAccess{address};
}
template<class T> static T* Ptr(uint32_t address) { return static_cast<T*>(Map(address,sizeof(T))); }
#undef X86_MEM8
#undef X86_MEM16
#undef X86_MEM32
#undef X86_MEM64
#undef X86_MEMF32
#undef X86_MEMF64
#undef X86_PTR
#define X86_MEM8(a) (*Ptr<uint8_t>(uint32_t(a)))
#define X86_MEM16(a) (*Ptr<x86_u16u>(uint32_t(a)))
#define X86_MEM32(a) (*Ptr<x86_u32u>(uint32_t(a)))
#define X86_MEM64(a) (*Ptr<x86_u64u>(uint32_t(a)))
#define X86_MEMF32(a) (*Ptr<x86_f32u>(uint32_t(a)))
#define X86_MEMF64(a) (*Ptr<x86_f64u>(uint32_t(a)))
#define X86_PTR(T,a) Ptr<T>(uint32_t(a))
static uint32_t Get(uint32_t address) { return X86_MEM32(address); }
static void Put(uint32_t address,uint32_t value) { X86_MEM32(address)=value; }
constexpr uint32_t W=0x600000,STATE=0x610000,R=STATE+0x3c,S=0x800000,OUTPUT=0x810000;
constexpr std::array<uint32_t,4> POOLS{0x620000,0x620100,0x620200,0x620300};
constexpr std::array<uint32_t,4> CAPACITY{1024,1024,1074,1024};
[[noreturn]] void x86_trap(X86Context&,uint32_t address,const char* message) {
    std::fprintf(stderr,"Unexpected trap %08x: %s\n",address,message);CHECK(false);std::abort();
}
void x86_resume(X86Context&,uint8_t*,const X86Resume&) { CHECK(false); }
void x86_icall(X86Context&,uint8_t*,uint32_t target) {
    std::fprintf(stderr,"Unexpected indirect call: %08x\n",target);CHECK(false);
}
X86_FUNC(sub_002D589F) {
    const uint32_t bytes=Get(ctx.esp+4),result=bump;
    CHECK(bytes>0&&uint64_t(bump)+bytes+15<memory.size());bump=(bump+bytes+15)&~15u;
    CHECK(allocations.emplace(result,bytes).second);std::memset(Map(result,bytes),0xa5,bytes);
    ctx.eax=result;ctx.esp+=4;
}
X86_FUNC(sub_002D57FB) { sub_002D589F(ctx,base); }
X86_FUNC(sub_002D631D) {
    const uint32_t address=Get(ctx.esp+4);auto found=allocations.find(address);
    CHECK(found!=allocations.end());std::memset(Map(address,found->second),0xcc,found->second);
    allocations.erase(found);ctx.esp+=4;
}
X86_FUNC(sub_002D5CCD) { sub_002D631D(ctx,base); }
namespace kernel {
uint32_t CallGuest(X86Context& ctx,uint32_t function,const uint32_t* args,size_t count) {
    ++guest_calls;CHECK(count==1);ctx.esp-=8;Put(ctx.esp,0x12345678);Put(ctx.esp+4,args[0]);
    if(function==0x2d589f)sub_002D589F(ctx,memory.data());
    else {CHECK(function==0x2d631d);sub_002D631D(ctx,memory.data());}
    const uint32_t result=ctx.eax;std::memset(&ctx,0xa7,sizeof(ctx));_mm_setcsr(0x5f80);return result;
}
}
// RETAIL_FUNCTIONS

static uint32_t Object(unsigned index) { return 0x900000+index*0x80; }
static X86Context Context(uint32_t owner=W) {
    X86Context ctx{};ctx.ecx=owner;ctx.ebx=0x11223344;ctx.ebp=0x55667788;
    ctx.esi=0x99aabbcc;ctx.edi=0xddeeff00;ctx.esp=S;ctx.fs_base=0x700000;
    Put(S-0x200,0x76543210);Put(S,0xf0000);Put(S+0x20,0x89abcdef);return ctx;
}
static void Preserved(const X86Context& ctx,uint32_t stack_bytes) {
    CHECK(ctx.esp==S+stack_bytes&&ctx.ebx==0x11223344&&ctx.ebp==0x55667788);
    CHECK(ctx.esi==0x99aabbcc&&ctx.edi==0xddeeff00);
    CHECK(Get(S-0x200)==0x76543210&&Get(S)==0xf0000&&Get(S+0x20)==0x89abcdef);
}
static void Setup() {
    CHECK(allocations.empty()&&trial_entity_pool::owners.empty()&&trial_entity_registry::extra.empty());
    memory.fill(0);bump=0x1000000;guest_calls=0;Put(0x51c4dc,W);Put(W+4,STATE);
    for(unsigned i=0;i<1024;++i)Put(R+i*8,UINT32_MAX);
    Put(STATE+0x204c,UINT32_MAX);
    for(unsigned i=0;i<4;++i) {
        Put(STATE+0x2c+i*4,POOLS[i]);auto ctx=Context(POOLS[i]);Put(S+4,CAPACITY[i]);Put(S+8,8);
        if(i<2)sub_00221AE0(ctx,memory.data());else sub_00221960(ctx,memory.data());
        Preserved(ctx,i<2?12:8);CHECK(Get(POOLS[i]+4)==0&&Get(POOLS[i]+8)!=0);
    }
    CHECK(allocations.size()==6);
}
static uint32_t Register(unsigned index,uint32_t parent=UINT32_MAX,bool after=false,unsigned group=0) {
    const uint32_t object=Object(index);Put(object+0x54,UINT32_MAX);Put(object+0x5c,8);
    for(unsigned word=0;word<16;++word)Put(object+0x10+word*4,0x3f000000+index*0x100+word);
    Put(STATE+0x2044,unsigned(after));Put(STATE+0x2048,group);Put(STATE+0x204c,parent);
    auto ctx=Context();Put(S+4,object);sub_00208150(ctx,memory.data());Preserved(ctx,8);
    CHECK(ctx.eax!=UINT32_MAX&&Get(object+0x58)==parent&&(Get(object+0x5c)&8)==0);
    // The owning constructor publishes the registration return value at +0x54.
    Put(object+0x54,ctx.eax);return ctx.eax;
}
static void CheckLookup(uint32_t id,uint32_t object) {
    auto ctx=Context();Put(S+4,id);sub_00207D70(ctx,memory.data());Preserved(ctx,8);CHECK(ctx.eax==object);
    ctx=Context();Put(S+4,id);sub_001F8FC0(ctx,memory.data());Preserved(ctx,4);CHECK(ctx.eax==object);
    CHECK(trial_entity_registry::LookupWorld(memory.data(),id)==object);
}
static std::vector<uint32_t> Nodes(unsigned index) {
    std::vector<uint32_t> result;std::set<uint32_t> seen;
    auto chain=[&](uint32_t node) {
        uint32_t previous=0;
        while(node) {
            CHECK(result.size()<10000&&seen.insert(node).second);CHECK(Get(node+8)==previous);
            result.push_back(node);previous=node;node=Get(node+4);
        }
    };
    if(index<2) {
        uint32_t previous=0;unsigned buckets=0;
        for(uint32_t bucket=Get(POOLS[index]+0x20);bucket;bucket=Get(bucket+4)) {
            CHECK(++buckets<=8&&Get(bucket+8)==previous&&Get(bucket)!=0);
            chain(Get(bucket));previous=bucket;
        }
    } else {chain(Get(POOLS[index]+0xc));CHECK(result.size()==Get(POOLS[index]+4));}
    return result;
}
using Order=std::array<std::vector<uint32_t>,8>;
static void After(Order& order,uint32_t parent,uint32_t child) {
    for(auto& bucket:order) {
        const auto at=std::find(bucket.begin(),bucket.end(),parent);
        if(at!=bucket.end()) {bucket.insert(at+1,child);return;}
    }
    CHECK(false);
}
static void Remove(Order& order,uint32_t id) {
    unsigned removed=0;
    for(auto& bucket:order) {
        const auto at=std::find(bucket.begin(),bucket.end(),id);
        if(at!=bucket.end()){bucket.erase(at);++removed;}
    }
    CHECK(removed==1);
}
static std::map<uint32_t,uint32_t> CheckRecords(unsigned index,const Order& order,const std::map<uint32_t,uint32_t>& objects) {
    std::vector<uint32_t> expected,actual;std::map<uint32_t,uint32_t> records;
    for(const auto& bucket:order)expected.insert(expected.end(),bucket.begin(),bucket.end());
    for(uint32_t node:Nodes(index)) {
        const uint32_t record=Get(node),id=Get(record);actual.push_back(id);
        CHECK(allocations.count(record)&&allocations.at(record)==8&&records.emplace(id,record).second);
        CHECK(objects.count(id)&&Get(record+4)==objects.at(id));
    }
    CHECK(actual==expected);return records;
}
static void MatrixFromOverflow(uint32_t id,uint32_t object) {
    std::memset(Map(OUTPUT,64),0xcc,64);auto ctx=Context();
    Put(S+4,0);Put(S+8,id);Put(S+12,UINT32_MAX);Put(S+16,OUTPUT);
    sub_00207DA0(ctx,memory.data());Preserved(ctx,20);CHECK(uint8_t(ctx.eax)==1);
    CHECK(std::memcmp(Map(OUTPUT,64),Map(object+0x10,64),64)==0);
}
static void Retire(uint32_t id) {
    auto ctx=Context();Put(S+4,id);sub_00207FF0(ctx,memory.data());Preserved(ctx,8);
}
static void CombinedLifecycle() {
    Setup();std::vector<uint32_t> ids;std::map<uint32_t,uint32_t> objects;Order update,render;
    for(unsigned i=0;i<3072;++i) {
        const uint32_t id=Register(i,UINT32_MAX,false,i%8);CHECK(objects.emplace(id,Object(i)).second);ids.push_back(id);
        update[i%8].insert(update[i%8].begin(),id);render[i%8].insert(render[i%8].begin(),id);
        CHECK(Get(POOLS[2]+4)==i+1);
    }
    for(unsigned i=0;i<ids.size();++i) {CheckLookup(ids[i],Object(i));CHECK(trial_entity_registry::HasExtra(R,ids[i])==(i>=1024));}
    CheckRecords(0,update,objects);CheckRecords(1,render,objects);CHECK(Nodes(2).size()==ids.size());
    const uint32_t parent=ids[2048];MatrixFromOverflow(parent,Object(2048));
    const uint32_t absolute=Register(3072,parent,false,3);CHECK(objects.emplace(absolute,Object(3072)).second);ids.push_back(absolute);
    update[3].insert(update[3].begin(),absolute);render[3].insert(render[3].begin(),absolute);
    CheckRecords(0,update,objects);CheckRecords(1,render,objects);
    const uint32_t relative=Register(3073,parent,true);CHECK(objects.emplace(relative,Object(3073)).second);ids.push_back(relative);
    After(update,parent,relative);After(render,parent,relative);
    CheckRecords(0,update,objects);CheckRecords(1,render,objects);
    // A disabled overflow parent selects the alternate registration path in 208150.
    auto ctx=Context();Put(S+4,parent);sub_00207F70(ctx,memory.data());Preserved(ctx,8);
    CHECK(Get(Object(2048)+0x5c)&8);Remove(render,parent);CheckRecords(1,render,objects);
    const uint32_t hidden_parent_child=Register(3074,parent,true);
    CHECK(objects.emplace(hidden_parent_child,Object(3074)).second);ids.push_back(hidden_parent_child);
    After(update,parent,hidden_parent_child);render[1].insert(render[1].begin(),hidden_parent_child);
    CheckRecords(0,update,objects);CheckRecords(1,render,objects);
    ctx=Context();Put(S+4,parent);sub_00207F00(ctx,memory.data());Preserved(ctx,8);
    CHECK((Get(Object(2048)+0x5c)&8)==0);render[1].insert(render[1].begin(),parent);
    const auto update_records=CheckRecords(0,update,objects),render_records=CheckRecords(1,render,objects);
    CHECK(Get(POOLS[2]+4)==ids.size()&&Get(POOLS[3]+4)==0);
    for(unsigned i=3072;i<ids.size();++i)CheckLookup(ids[i],Object(i));
    for(unsigned i=0;i<ids.size();++i) {
        Retire(ids[i]);CheckLookup(ids[i],0);
        CHECK(Get(POOLS[2]+4)==ids.size()-i-1&&Get(POOLS[3]+4)==i+1);
        CHECK(!allocations.count(update_records.at(ids[i]))&&!allocations.count(render_records.at(ids[i])));
        if(i==0) {Retire(ids[i]);CHECK(Get(POOLS[2]+4)==ids.size()-1&&Get(POOLS[3]+4)==1);}
    }
    CHECK(Nodes(0).empty()&&Nodes(1).empty()&&Nodes(2).empty());
    std::set<uint32_t> pending;
    for(uint32_t node:Nodes(3))CHECK(pending.insert(Get(node)).second);
    CHECK(pending.size()==objects.size());for(auto [id,object]:objects)CHECK(pending.count(object)==1);
    CHECK(trial_entity_registry::extra.empty());
    for(uint32_t pool:POOLS)CHECK(trial_entity_pool::owners.at(pool).slabs.size()>=2);
    while(Get(POOLS[3]+4)) {
        ctx=Context(POOLS[3]);Put(S+4,Get(Get(POOLS[3]+0xc)));sub_00221A10(ctx,memory.data());Preserved(ctx,8);
    }
    CHECK(Nodes(3).empty());
    for(unsigned i=0;i<4;++i) {
        ctx=Context(POOLS[i]);Put(S+4,0);
        if(i<2)sub_00221A80(ctx,memory.data());else if(i==2)sub_00208CB0(ctx,memory.data());else sub_00221930(ctx,memory.data());
        Preserved(ctx,i==3?8:4);
    }
    trial_entity_registry::Reset(R);
    CHECK(allocations.empty()&&trial_entity_pool::owners.empty()&&trial_entity_registry::extra.empty());
    CHECK(guest_calls>0);
}
int main() {
    const auto csr=_mm_getcsr();CombinedLifecycle();_mm_setcsr(csr);
    std::printf("PASS %u checks: 3075 registered entities; four grown pools; exact update/render records; overflow-parent absolute/relative registration; render disable/enable; matrix lookup; retirement/pending cleanup; pool slab teardown\n",checks);
}
