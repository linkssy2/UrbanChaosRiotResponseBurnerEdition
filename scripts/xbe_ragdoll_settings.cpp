// Retention policy from migration_physics_ragdoll_pool_guard_resume.c.
// The original constructors, activation, simulation and destruction still own
// each body. Only the number of pool slots and automatic retirement change.
#include <algorithm>
#include <initializer_list>
#include <limits>
#include <mutex>
#include <new>
#include <unordered_map>
#include <vector>
#include "kernel/kernel.h"
#include "xbe_input_memory.h"
#include "xbe_ragdoll_settings.h"
namespace trial_graphics {uint32_t RagdollMode();}
namespace {
using kernel::R32;using kernel::W32;
constexpr uint32_t pool=0x51da44,initialized=0x51da6c,next_id=0x51da70;
struct Slot {uint32_t id=UINT32_MAX,body=0;};
std::vector<Slot> extra;
struct ObjectSlot {uint32_t body;bool active;};
struct ObjectPool {
    uint32_t stride,capacity,storage,free_list;
    std::vector<ObjectSlot> slots;
};
std::unordered_map<uint32_t,ObjectPool> object_pools;
std::recursive_mutex object_pool_lock;
uint32_t Mode(){return std::min(trial_graphics::RagdollMode(),2u);}
size_t Count(){return 5+extra.size();}
Slot Read(size_t i){return i<5?Slot{R32(pool+uint32_t(i)*8),R32(pool+uint32_t(i)*8+4)}:extra[i-5];}
void Write(size_t i,Slot s){if(i<5){W32(pool+uint32_t(i)*8,s.id);W32(pool+uint32_t(i)*8+4,s.body);}else extra[i-5]=s;}
void Require(X86Context& ctx,bool valid,uint32_t entry){if(!valid)x86_trap(ctx,entry,"Invalid retained ragdoll ownership");}
bool Span(uint32_t p,uint64_t n){return n&&n<=UINT32_MAX&&TrialGuestReadable(p,uint32_t(n));}
uint32_t Call(X86Context& ctx,uint8_t* base,uint32_t fn,uint32_t object,
              std::initializer_list<uint32_t> args,uint32_t ret){
    // Extra clone construction can occur during gameplay, after startup asset
    // descriptors were released. Preserve the caller's entire CPU state, not
    // just the registers covered by the host CallGuest convenience function.
    struct Restore {X86Context& ctx;X86Context saved;unsigned mxcsr;
        ~Restore(){ctx=saved;_mm_setcsr(mxcsr);}} restore{ctx,ctx,_mm_getcsr()};
    ctx.ecx=object;
    for(auto p=args.end();p!=args.begin();){ctx.esp-=4;W32(ctx.esp,*--p);}
    ctx.esp-=4;W32(ctx.esp,ret);x86_icall(ctx,base,fn);return ctx.eax;
}
bool Grow(X86Context& ctx,uint8_t* base){
    const uint32_t source=Read(0).body;
    Require(ctx,Span(source,0xa0),0x2a6790);
    for(uint32_t offset:{0x70u,0x7cu}){
        const uint32_t n=R32(source+offset+4);
        Require(ctx,!n||Span(R32(source+offset),uint64_t(n)*4),0x2a6790);
    }
    if(extra.size()>=UINT32_MAX/8-5)return false;
    try{
        if(extra.size()==extra.capacity())extra.reserve(std::max(size_t(8),extra.size()*2));
    }catch(const std::bad_alloc&){return false;}
    const uint32_t object=Call(ctx,base,0x2d57fb,0,{0xa0},0x2a6ab0);
    if(!object)return false;
    const uint32_t world=Call(ctx,base,0x29e4f0,0,{source+0x70,source+0x7c},0x2a6ace);
    const uint32_t body=Call(ctx,base,0x2a6130,object,{world,source+0x70,source+0x7c},0x2a6ad6);
    Require(ctx,body&&Span(body,0xa0),0x2a6790);
    extra.push_back({UINT32_MAX,body});return true;
}
ObjectPool& RetainedObjects(X86Context& ctx,uint32_t owner){
    Require(ctx,Span(owner,28)&&R32(owner)==0x482e94,0x238430);
    const uint32_t stride=R32(owner+8),capacity=R32(owner+12);
    const uint32_t storage=R32(owner+24),free_list=R32(owner+16);
    Require(ctx,stride&&capacity&&Span(storage,uint64_t(stride)*capacity)&&
        Span(free_list,uint64_t(capacity)*4)&&R32(owner+20)<=capacity,0x238430);
    auto [it,inserted]=object_pools.try_emplace(owner,ObjectPool{stride,capacity,storage,free_list,{}});
    const auto& p=it->second;
    Require(ctx,p.stride==stride&&p.capacity==capacity&&p.storage==storage&&p.free_list==free_list,0x238430);
    return it->second;
}
ObjectSlot& AvailableObject(X86Context& ctx,uint8_t* base,uint32_t owner){
    auto& p=RetainedObjects(ctx,owner);
    for(auto& s:p.slots)if(!s.active)return s;
    // Original factories immediately write the returned object. Reserve host
    // bookkeeping first, then fail explicitly if guest allocation is exhausted.
    if(p.slots.size()==p.slots.capacity())p.slots.reserve(std::max(size_t(8),p.slots.size()*2));
    const uint32_t object=Call(ctx,base,0x2d83f8,0,{p.stride},0x2383e8);
    if(!object)x86_trap(ctx,0x238430,"Out of memory extending exhausted object pool");
    Require(ctx,(object&3)==0&&Span(object,p.stride),0x238430);
    p.slots.push_back({object,false});return p.slots.back();
}
}
float TrialRagdollSeconds(){const auto mode=Mode();return mode==2?std::numeric_limits<float>::infinity():mode==1?120.f:30.f;}
bool TrialRagdollKeepUnseen(float seconds){const auto mode=Mode();return mode==2||(mode==1&&seconds<=120.f);}
uint32_t TrialRagdollRetireCount(uint32_t original,uint32_t dead){const auto mode=Mode();return mode==2?0:mode==1?(dead>32?dead-32:0):original;}

bool TrialRetainedObjectPool(X86Context& ctx,uint8_t* base,uint32_t entry){
    // A checked factory still observes the original capacity. Only an actual
    // unchecked allocation at zero needs overflow storage instead of array[-1].
    if(entry==0x2384c0)return false;
    std::lock_guard<std::recursive_mutex> lock(object_pool_lock);
    const uint32_t owner=ctx.ecx;
    auto it=object_pools.find(owner);
    if(it==object_pools.end()){
        if(entry==0x238440||entry==0x2384f0)return false;
        Require(ctx,entry==0x238430&&Span(owner,28)&&R32(owner)==0x482e94,entry);
        if(R32(owner+20))return false; // Normal allocation remains original.
    }
    Require(ctx,ctx.esp>=256&&Span(ctx.esp-256,284),entry);
    if(it!=object_pools.end())RetainedObjects(ctx,owner);
    if(entry==0x2384f0){
        if(it==object_pools.end())return false;
        // This owner frees raw backing storage, not contained objects. Match
        // its teardown without repeating the contained objects' destructors.
        auto slots=std::move(it->second.slots);object_pools.erase(it);
        for(const auto& s:slots)Call(ctx,base,0x2d73a4,0,{s.body},0x238502);
        return false;
    }
    if(entry==0x238440){
        if(it==object_pools.end())return false;
        const uint32_t object=R32(ctx.esp+4);
        for(auto& s:it->second.slots)if(s.body==object){
            Require(ctx,s.active,entry);s.active=false;ctx.esp+=8;return true;
        }
        return false; // Original storage retains its original range checks.
    }
    Require(ctx,entry==0x238430,entry);
    Require(ctx,Span(owner,28)&&R32(owner)==0x482e94&&R32(owner+20)<=R32(owner+12),entry);
    if(R32(owner+20))return false;
    try{
        auto& s=AvailableObject(ctx,base,owner);
        s.active=true;ctx.eax=s.body;
        ctx.esp+=4;return true;
    }catch(const std::bad_alloc&){
        x86_trap(ctx,entry,"Out of memory tracking object pool overflow");
    }
    return false;
}

bool TrialRagdollPool(X86Context& ctx,uint8_t* base,uint32_t entry){
    if(entry==0x2a6a70){
        // Original initialization includes FS/SEH handling. Let the current
        // translator execute it; mission destruction must precede rebuilding.
        Require(ctx,extra.empty(),entry);return false;
    }
    if(extra.empty()&&(entry!=0x2a6790||Mode()==0))return false;
    Require(ctx,Span(pool,48)&&ctx.esp>=256&&Span(ctx.esp-256,284),entry);
    if(entry==0x2a6790){
        Require(ctx,base[initialized]!=0,entry);
        const unsigned mode=Mode();const size_t limit=mode==0?5:mode==1?32:Count()+1;
        size_t selected=0;bool found=false;
        for(size_t i=0;i<std::min(Count(),limit);++i){
            const Slot s=Read(i);
            if(s.id==UINT32_MAX){selected=i;found=true;break;}
            if(s.id<Read(selected).id)selected=i;
        }
        if(!found&&Count()<limit){
            if(!Grow(ctx,base)){ctx.eax=UINT32_MAX;ctx.esp+=4;return true;}
            selected=Count()-1;
        }
        Slot s=Read(selected);Require(ctx,Span(s.body,0xa0),entry);
        s.id=R32(next_id);Write(selected,s);W32(next_id,s.id+1);
        // Original 2A6790 forwards six cdecl arguments into thiscall2A5F00.
        const uint32_t a=ctx.esp;
        Call(ctx,base,0x2a5f00,s.body,{R32(a+4),R32(a+8),R32(a+12),R32(a+16),R32(a+24),R32(a+20)},0x2a683a);
        ctx.eax=s.id;ctx.esp+=4;return true;
    }
    if(entry==0x2a5070){
        const uint32_t id=R32(ctx.esp+4);ctx.eax=0;
        if(id!=UINT32_MAX)for(size_t i=0;i<Count();++i)if(Read(i).id==id){ctx.eax=Read(i).body;break;}
        ctx.esp+=4;return true;
    }
    if(entry==0x2a6850){
        const uint32_t id=R32(ctx.esp+4);
        if(id!=UINT32_MAX)for(size_t i=0;i<Count();++i){Slot s=Read(i);if(s.id!=id)continue;
            Require(ctx,Span(s.body,0xa0),entry);s.id=UINT32_MAX;Write(i,s);
            Call(ctx,base,0x2a5f00,s.body,{0,0,0,0,0,0},0x2a6889);
        }
        ctx.esp+=4;return true;
    }
    if(entry==0x2a5d50){
        for(size_t i=0;i<Count();++i){const Slot s=Read(i);if(s.id==UINT32_MAX)continue;
            Require(ctx,Span(s.body,0xa0),entry);
            uint32_t n=R32(s.body+0x74);
            for(uint32_t j=0;int32_t(j)<int32_t(n);++j){
                const uint32_t bodies=R32(s.body+0x70);
                Require(ctx,Span(bodies,uint64_t(n)*4),entry);
                Call(ctx,base,0x2eeff0,R32(s.body+0x88),{R32(bodies+j*4),0,1},0x2a5d86);
                n=R32(s.body+0x74);
            }
        }
        ctx.esp+=4;return true;
    }
    Require(ctx,entry==0x2a5030,entry);
    if(base[initialized]){
        for(size_t i=0;i<Count();++i){Slot s=Read(i);if(s.body){
            Require(ctx,Span(s.body,4)&&Span(R32(s.body),4),entry);
            Call(ctx,base,R32(R32(s.body)),s.body,{1},0x2a5050);
        }s.body=0;Write(i,s);}
        base[initialized]=0;
    }
    std::vector<Slot>().swap(extra);ctx.esp+=4;return true;
}
