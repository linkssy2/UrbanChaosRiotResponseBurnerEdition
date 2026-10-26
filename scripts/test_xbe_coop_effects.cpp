// Execute the retail particle target transitions, including their real saved
// operands, heat branch, particle count and release ordering. GPU rasterization
// and particle geometry/lifetime are deliberately outside this contract test.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
#include "kernel/kernel.h"
namespace kernel {static std::array<uint8_t,0x800000> memory{};}
namespace mem {
unsigned world_probes=0,rejected_probes=0;
uint32_t denied_page=0xffffffffu;
bool IsCommitted(uint32_t page){
    world_probes+=page==0x51c000;
    if(page==denied_page){++rejected_probes;return false;}
    return page<kernel::memory.size();
}
}
#include "../common/xbe_coop_effects.cpp"
namespace effects_test {
constexpr uint32_t owner0=0x610000,owner1=0x612000,world=0x620000,state=0x624000;
constexpr uint32_t layers=0x630000,layer0=0x631000,layer1=0x632000;
constexpr uint32_t graphics=0x640000,graphics_table=0x641000,distortion=0x642000;
constexpr uint32_t texture=0x643000,heat_target=0x644000,scene_target=0x645000,scene_depth=0x646000;
constexpr uint32_t sdk_device=0x700000,sdk_viewport=sdk_device+0xee0,stack=0x7f0000;
constexpr std::array<uint32_t,6> full{0,0,640,480,0,0x3f800000};
constexpr std::array<uint32_t,6> heat_full{0,0,32,32,0,0x3f800000};
unsigned restores=0;
bool throw_restore=false;
std::vector<std::array<uint32_t,2>> targets;
std::vector<uint32_t> releases;
uint8_t* Bytes(){return kernel::memory.data();}
uint32_t Word(uint32_t address){uint32_t value;std::memcpy(&value,Bytes()+address,4);return value;}
void Put(uint32_t address,uint32_t value){std::memcpy(Bytes()+address,&value,4);}
std::array<uint32_t,6> Viewport(){std::array<uint32_t,6> value;std::memcpy(value.data(),Bytes()+sdk_viewport,24);return value;}
void SetViewport(const std::array<uint32_t,6>& value){std::memcpy(Bytes()+sdk_viewport,value.data(),24);}
std::array<uint32_t,6> Half(unsigned slot){return {0,slot*240,640,240,0,0x3f800000};}
X86Context Setup(unsigned slot){
    kernel::memory.fill(0);trial_coop::Reset();trial_coop::SetAvailable(true);
    assert(trial_coop::Request());trial_coop::SetActive(true);
    for(unsigned i=0;i<2;++i){
        const uint32_t owner=i?owner1:owner0,layer=i?layer1:layer0;
        Put(owner,0x46fbe8);Put(owner+0x54,349+i);Put(owner+0x368,i);
        Put(layers+i*4,layer);Put(layer+4,owner);Put(layer+0x24,i);
    }
    Put(0x51c4dc,world);Put(world+4,state);Put(state+4,layers);Put(state+8,layers+8);
    Put(0x519f60,graphics);Put(graphics,graphics_table);Put(graphics_table+0x9c,0xf1000);
    Put(distortion+0x28,7);Put(distortion+0x34,texture);
    Put(0x3f4238,sdk_device);Put(sdk_device+0x1a04,scene_target);
    Put(sdk_device+0x1a08,scene_depth);Put(sdk_device+0x1a14,scene_target);
    SetViewport(Half(slot));restores=0;throw_restore=false;targets.clear();releases.clear();
    mem::world_probes=mem::rejected_probes=0;mem::denied_page=0xffffffffu;
    X86Context c{};c.eax=0x1234;c.ecx=0x2345;c.edx=0x3456;c.ebx=0x4567;
    c.esi=0x5678;c.edi=0x6789;c.ebp=0x789a;c.esp=stack;
    c.cf=1;c.zf=1;c.sf=1;c.of=1;c.pf=1;c.df=1;c.xflags=0x200000;
    c.fcw=0x37f;c.fsw=0x1234;c.mxcsr=0x1f80;c.top=2;
    for(unsigned i=0;i<8;++i){c.st[i]=double(i)+.125;c.mm[i]=0x1234567800000000ull+i;c.xmm[i].q[0]=i+1;c.xmm[i].q[1]=i+9;}
    return c;
}
}
uint32_t TrialCoopPlayer(unsigned slot){return slot==0?effects_test::owner0:slot==1?effects_test::owner1:0;}
[[noreturn]] void x86_trap(X86Context&,uint32_t,const char*){std::abort();}
void x86_icall(X86Context& c,uint8_t*,uint32_t target){
    using namespace effects_test;
    assert(target==0xf1000&&c.ecx==graphics);c.eax=distortion;c.esp+=4;
}
X86_FUNC(sub_003F1900){
    using namespace effects_test;
    assert(Word(ctx.esp+4)==texture&&Word(ctx.esp+8)==0);ctx.eax=heat_target;ctx.esp+=12;
}
X86_FUNC(sub_003E35D0){ctx.eax=effects_test::Word(effects_test::sdk_device+0x1a04);ctx.esp+=4;}
X86_FUNC(sub_003E35F0){ctx.eax=effects_test::Word(effects_test::sdk_device+0x1a08);ctx.esp+=4;}
X86_FUNC(sub_003E2FD0){
    using namespace effects_test;
    const uint32_t target=Word(ctx.esp+4),depth=Word(ctx.esp+8);
    assert(target==heat_target||target==scene_target);assert(depth==scene_depth);
    Put(sdk_device+0x1a04,target);Put(sdk_device+0x1a08,depth);targets.push_back({target,depth});
    // Retail 3E321C -> 3E7F70 -> 3E3610 resets to the full bound target.
    // These services complete; the actual particle caller instructions run.
    SetViewport(target==heat_target?heat_full:full);ctx.eax=0;ctx.esp+=12;
}
X86_FUNC(sub_003E7080){assert(ctx.ecx==0x40358&&ctx.edx==0x10101);ctx.esp+=4;}
X86_FUNC(sub_003E55B0){effects_test::releases.push_back(effects_test::Word(ctx.esp+4));ctx.esp+=8;}
// ORIGINAL_COOP_EFFECTS_TRANSITIONS
namespace kernel {
uint32_t CallGuest(X86Context& c,uint32_t target,const uint32_t* args,size_t count){
    using namespace effects_test;
    assert(target==0x3e3610&&count==1);++restores;
    std::memcpy(Bytes()+sdk_viewport,Bytes()+args[0],24);
    // The helper must isolate all guest registers, flags, FP state and host
    // MXCSR, even when the retained SDK service changes them or throws.
    c.eax=0xdead;c.ecx=0xbeef;c.edx=3;c.ebx=4;c.ebp=5;c.esi=6;c.edi=7;c.esp-=80;
    c.cf^=1;c.zf^=1;c.sf^=1;c.of^=1;c.pf^=1;c.df^=1;c.xflags^=0x200000;
    c.top=3;c.fcw=0x27f;c.fsw=0x4321;c.mxcsr=0x3f80;
    for(unsigned i=0;i<8;++i){c.st[i]=double(i)+.25;c.mm[i]=0x8765432100000000ull+i;c.xmm[i].q[0]=i+17;c.xmm[i].q[1]=i+23;}
    _mm_setcsr(0x3f80);if(throw_restore)throw 17;return 0;
}
}
namespace effects_test {
using Transition=void(*)(X86Context&,uint8_t*);
const Transition enter[]={ParticleEnter0,ParticleEnter1,ParticleEnter2,ParticleEnter3,ParticleEnter4};
const Transition leave[]={ParticleRestore0,ParticleRestore1,ParticleRestore2,ParticleRestore3,ParticleRestore4};
void ParticleTransitions(X86Context& c,unsigned family,bool heat){
    const uint32_t count_offset=family==3?0x20:(family==0||family==4?0x1c:0x18);
    Put(c.esp+count_offset,3);
    if(heat){enter[family](c,Bytes());assert(Viewport()==heat_full);}
    // Select the existing branch exactly as the original source kind query
    // did earlier in each complete renderer. No vertex or emitter work is run.
    if(family<2)c.ebx=(c.ebx&~0xffu)|unsigned(heat);
    else Bytes()[c.esp+(family==2?0x15:0x13)]=heat;
    leave[family](c,Bytes());
}
void CheckTransitions(){
    unsigned cases=0;
    for(unsigned family=0;family<5;++family)for(unsigned slot=0;slot<2;++slot)for(bool heat:{false,true}){
        auto original=Setup(slot);const unsigned csr=_mm_getcsr();_mm_setcsr(0x5f80);
        ParticleTransitions(original,family,heat);const unsigned expected_csr=_mm_getcsr();_mm_setcsr(csr);
        assert(Viewport()==(heat?full:Half(slot))&&!restores);
        const auto original_targets=targets;
        const auto original_releases=releases;
        const uint32_t original_count=Word(distortion+0x28);
        std::array<uint8_t,0x200> original_frame{};std::memcpy(original_frame.data(),Bytes()+stack-0x100,original_frame.size());
        auto fixed=Setup(slot);_mm_setcsr(0x5f80);
        {TrialCoopEffectsViewportScope scope(fixed,Bytes());ParticleTransitions(fixed,family,heat);}
        assert(_mm_getcsr()==expected_csr);_mm_setcsr(csr);
        assert(!std::memcmp(&fixed,&original,sizeof fixed));
        assert(!std::memcmp(original_frame.data(),Bytes()+stack-0x100,original_frame.size()));
        assert(Viewport()==Half(slot)&&restores==unsigned(heat));
        assert(targets==original_targets&&releases==original_releases&&Word(distortion+0x28)==original_count);
        assert(Word(sdk_device+0x1a04)==scene_target&&Word(sdk_device+0x1a08)==scene_depth);
        assert(original_count==7+unsigned(heat)*3);
        if(heat){
            assert((targets==std::vector<std::array<uint32_t,2>>{{heat_target,scene_depth},{scene_target,scene_depth}}));
            assert(releases.size()==3&&releases[0]==heat_target);
        }else assert(targets.empty()&&releases.empty());
        ++cases;
    }
    // Repeat different original particle families across both views rather
    // than accidentally carrying a previous camera's captured viewport.
    auto c=Setup(0);
    for(unsigned frame=0;frame<6;++frame)for(unsigned slot=0;slot<2;++slot){
        SetViewport(Half(slot));c.esp=stack;
        const bool heat=(frame+slot)%2==0;const unsigned before=restores;
        {TrialCoopEffectsViewportScope scope(c,Bytes());ParticleTransitions(c,(frame+slot)%5,heat);}
        assert(Viewport()==Half(slot)&&restores==before+unsigned(heat));++cases;
    }
    std::printf("co-op effects: %u retail particle transition cases; fullscreen negative witnesses and both-view restoration passed (no GPU)\n",cases);
}
void CheckGuards(){
    for(unsigned invalid=0;invalid<14;++invalid){
        auto c=Setup(1);
        switch(invalid){
            case 0:trial_coop::SetAvailable(false);break;
            case 1:trial_coop::Reset();break;
            case 2:trial_coop::SetActive(false);break;
            case 3:Put(owner0+0x5c,1);break;
            case 4:Put(owner1+0x54,0xffffffffu);break;
            case 5:Put(owner1,0x458ca8);break;
            case 6:Put(layer1+4,owner0);break;
            case 7:Put(layer1+0x24,0);break;
            case 8:Put(state+8,layers+12);break;
            case 9:Put(sdk_device+0x1a14,heat_target);break;
            case 10:SetViewport(full);break;
            case 11:{auto v=Half(1);v[4]=0x3e800000;SetViewport(v);break;}
            case 12:{auto v=Half(1);v[2]=0;SetViewport(v);break;}
            case 13:Put(0x3f4238,0xfffffff0);break;
        }
        auto original=c;ParticleTransitions(original,4,true);
        const auto expected=Viewport();const auto expected_targets=targets;
        const auto expected_releases=releases;
        // Replay the same original sequence from the exact initial state. Only
        // the guard setup, not a substitute rendering path, differs above.
        c=Setup(1);
        switch(invalid){
            case 0:trial_coop::SetAvailable(false);break;
            case 1:trial_coop::Reset();break;
            case 2:trial_coop::SetActive(false);break;
            case 3:Put(owner0+0x5c,1);break;
            case 4:Put(owner1+0x54,0xffffffffu);break;
            case 5:Put(owner1,0x458ca8);break;
            case 6:Put(layer1+4,owner0);break;
            case 7:Put(layer1+0x24,0);break;
            case 8:Put(state+8,layers+12);break;
            case 9:Put(sdk_device+0x1a14,heat_target);break;
            case 10:SetViewport(full);break;
            case 11:{auto v=Half(1);v[4]=0x3e800000;SetViewport(v);break;}
            case 12:{auto v=Half(1);v[2]=0;SetViewport(v);break;}
            case 13:Put(0x3f4238,0xfffffff0);break;
        }
        {TrialCoopEffectsViewportScope scope(c,Bytes());ParticleTransitions(c,4,true);}
        assert(!restores&&Viewport()==expected&&!std::memcmp(&c,&original,sizeof c));
        assert(targets==expected_targets&&releases==expected_releases);
    }
    // A changed target, depth, device, camera lifetime or deliberate subviewport
    // on exit must not be mistaken for the SDK's full-main-target reset.
    for(unsigned changed=0;changed<6;++changed){
        auto c=Setup(0);
        {
            TrialCoopEffectsViewportScope scope(c,Bytes());ParticleTransitions(c,0,true);
            switch(changed){
                case 0:Put(sdk_device+0x1a04,heat_target);break;
                case 1:Put(sdk_device+0x1a08,0);break;
                case 2:Put(0x3f4238,sdk_device+0x10);break;
                case 3:trial_coop::SetActive(false);break;
                case 4:{auto v=full;v[1]=1;SetViewport(v);break;}
                case 5:Put(owner1+0x5c,1);break;
            }
        }
        assert(!restores);
    }
}
void CheckExitWork(){
    // The global-world page probe counts qualification without timing noise.
    for(unsigned exit=0;exit<4;++exit){
        auto c=Setup(1);std::array<uint32_t,6> expected{};
        {
            TrialCoopEffectsViewportScope scope(c,Bytes());assert(mem::world_probes==1);
            if(exit==1){Put(sdk_device+0x1a04,heat_target);SetViewport(heat_full);}
            if(exit==2)ParticleTransitions(c,0,false);
            if(exit==3){auto viewport=full;viewport[1]=1;SetViewport(viewport);}
            expected=Viewport();
        }
        assert(mem::world_probes==1&&!restores&&Viewport()==expected);
    }
    for(bool changed_world:{false,true}){
        auto c=Setup(1);
        {
            TrialCoopEffectsViewportScope scope(c,Bytes());assert(mem::world_probes==1);
            ParticleTransitions(c,0,true);
            if(changed_world)Put(0x51c4dc,world+0x100);
        }
        assert(mem::world_probes==2&&restores==unsigned(!changed_world));
        assert(Viewport()==(changed_world?full:Half(1)));
    }
    // Teardown can invalidate either device page before world qualification.
    for(uint32_t page:{0x3f4000u,sdk_device})for(bool active:{false,true}){
        auto c=Setup(1);X86Context expected{};
        {
            TrialCoopEffectsViewportScope scope(c,Bytes());
            ParticleTransitions(c,0,true);expected=c;
            mem::denied_page=page;trial_coop::SetActive(active);
        }
        assert(mem::rejected_probes==1&&mem::world_probes==1&&!restores);
        assert(Viewport()==full&&!std::memcmp(&c,&expected,sizeof c));
    }
}
void CheckExceptionalExit(){
    auto c=Setup(1);const unsigned csr=_mm_getcsr();_mm_setcsr(0x5f80);
    X86Context expected{};
    try{
        TrialCoopEffectsViewportScope scope(c,Bytes());ParticleTransitions(c,2,true);
        expected=c;throw_restore=true;
    }catch(int code){assert(code==17);}
    assert(restores==1&&!std::memcmp(&c,&expected,sizeof c)&&_mm_getcsr()==0x5f80);
    _mm_setcsr(csr);
    c=Setup(0);
    try{
        TrialCoopEffectsViewportScope scope(c,Bytes());ParticleTransitions(c,3,true);
        expected=c;throw 19;
    }catch(int code){assert(code==19);}
    assert(!restores&&!std::memcmp(&c,&expected,sizeof c)&&Viewport()==full);
}
void CheckCompositeRestore(){
    // The original composite's final target transition is followed by authored
    // screen-space grid drawing. Its full viewport must stay intact inside the
    // function; only the exit back to ordinary geometry restores the split.
    for(unsigned slot=0;slot<2;++slot){
        auto original=Setup(slot);Put(stack+0x50,scene_target);Put(stack+0x58,scene_depth);
        Put(sdk_device+0x1a04,heat_target);SetViewport(heat_full);
        ParticleRestore5(original,Bytes());assert(Viewport()==full&&!restores);
        const auto original_targets=targets;
        auto fixed=Setup(slot);Put(stack+0x50,scene_target);Put(stack+0x58,scene_depth);
        {
            TrialCoopEffectsViewportScope scope(fixed,Bytes());
            Put(sdk_device+0x1a04,heat_target);SetViewport(heat_full);
            ParticleRestore5(fixed,Bytes());assert(Viewport()==full&&!restores);
        }
        assert(Viewport()==Half(slot)&&restores==1&&targets==original_targets);
        assert(!std::memcmp(&fixed,&original,sizeof fixed));
    }
}
}
int main(){effects_test::CheckTransitions();effects_test::CheckCompositeRestore();effects_test::CheckGuards();effects_test::CheckExitWork();effects_test::CheckExceptionalExit();}
