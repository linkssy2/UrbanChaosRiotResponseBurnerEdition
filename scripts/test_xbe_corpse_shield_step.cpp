// Execute the pinned shared-world update; only solver and existing-entity callbacks are boundaries.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
#include "cpu/x86_ops.h"

namespace {
alignas(16) std::array<uint8_t,0x800000> memory{};
constexpr uint32_t manager=0x600000,world=0x601000,list=0x602000,actor=0x603000,stack=0x700000;
std::vector<unsigned> events;
float expected_dt;
uint32_t bits(float f){uint32_t v;std::memcpy(&v,&f,4);return v;}
void put(uint32_t p,uint32_t v){std::memcpy(memory.data()+p,&v,4);}
uint32_t get(uint32_t p){uint32_t v;std::memcpy(&v,memory.data()+p,4);return v;}
}
void TrialCorpseShieldBeforePhysics(X86Context& ctx,uint8_t* base,float dt){
    assert(base==memory.data() && ctx.eax==manager && dt==expected_dt);
    assert(events.empty());events.push_back(1);
}
void TrialCorpseShieldAfterPhysics(X86Context& ctx,uint8_t* base,float dt){
    assert(base==memory.data() && dt==expected_dt && ctx.ebp==bits(dt));
    assert(events==std::vector<unsigned>({1,2}));events.push_back(3);
}
X86_FUNC(sub_002ED370){
    assert(ctx.ecx==world && get(ctx.esp+4)==bits(expected_dt));
    events.push_back(2);ctx.esp+=8;
}
X86_FUNC(sub_0029F570){
    assert(ctx.ecx==actor && get(ctx.esp+4)==actor+0x20 && get(ctx.esp+8)==bits(expected_dt));
    events.push_back(4);ctx.esp+=12;
}

// NATIVE_SHIELD_WORLD_STEP

int main(){
    for(float hz:{30.f,60.f,120.f,240.f,360.f})for(bool existing_entity:{false,true}){
        expected_dt=1.f/hz;memory.fill(0);events.clear();
        put(0x51d910,manager);put(manager+0x10,world);put(manager+0x18,bits(2.f));
        put(0x44bf94,bits(1.f));put(manager+4,list);put(manager+8,existing_entity?1:0);
        put(list,actor);memory[actor+0x10]=1;
        put(stack,0xf00000);put(stack+4,bits(expected_dt));
        X86Context original{};original.esp=stack;original.ebp=0x1234;original.edi=0x5678;original.esi=0x9876;
        auto candidate=original;const std::vector<uint8_t> before(memory.begin(),memory.end());
        OriginalStep(original,memory.data());const std::vector<uint8_t> expected_memory(memory.begin(),memory.end());
        assert(events==(existing_entity?std::vector<unsigned>{2,4}:std::vector<unsigned>{2}));
        std::memcpy(memory.data(),before.data(),memory.size());events.clear();ShieldStep(candidate,memory.data());
        assert(events==(existing_entity?std::vector<unsigned>{1,2,3,4}:std::vector<unsigned>{1,2,3}));
        assert(!std::memcmp(memory.data(),expected_memory.data(),memory.size()) && !std::memcmp(&candidate,&original,sizeof(original)));
    }
    std::puts("PASS: one shield pre/post update around the original world step; native CPU/memory parity at 30-360 FPS.");
}
