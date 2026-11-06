// Execute pinned shield, BasicProp and flashlight loader bodies; only allocation, resource
// parsing and render services are mocked. Asset tests cover the real AMF bytes.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "kernel/kernel.h"
namespace kernel {static std::array<uint8_t,0x800000> memory{};}
namespace mem {bool IsCommitted(uint32_t page){return page<kernel::memory.size();}}
#include "../common/xbe_corpse_shield.cpp"

extern const char* const native_model_names[4];
namespace corpse_shield_test {
const char* Original(unsigned kind){return kind?corpse_shield::original[1]:native_model_names[0];}
constexpr uint32_t world=0x600000,name_cell=0x601000,name_text=0x601010;
constexpr uint32_t owners[]={0x602000,0x603000},bodies[]={0x604000,0x605000};
constexpr uint32_t children[]={0x606000,0x607000};
constexpr uint32_t wrappers[]={0x608000,0x608100,0x608200,0x608300,0x608400,0x608500};
constexpr uint32_t renders[]={0x609000,0x60a000,0x60b000,0x60c000,0x60d000,0x60e000};
constexpr uint32_t resource[]={0x610000,0x611000,0x612000,0x613000},authored[]={0x620000,0x622000,0x624000,0x626000};
constexpr uint32_t metadata[]={0x630000,0x638000,0x660000,0x670000};
constexpr uint32_t proxy=0x640000,proxy_vtable=0x641000;
constexpr uint32_t allocation=0x650000,call_stack=0x680000,observer_name=0x690000;
constexpr uint32_t props[]={0x6a0000,0x6a1000},descriptors[]={0x6b0000,0x6b0100};
constexpr uint32_t prop_entries[]={0x6b1000,0x6b1020},placement=0x6b2000;
constexpr uint32_t level=0x6c0000,provider=0x6c2100,prop_slots=0x6c3000;
constexpr unsigned source_indices[]={585,578};
constexpr uint32_t stack=0x710000,fs=0x720000;
unsigned loads=0,allocations=0,frees=0,stacks=0,stack_frees=0,resource_mode=0;
unsigned active_slot=0,active_kind=0,registrations=0,model_calls=0;
uint32_t pose_render=0;
unsigned pose_compositions=0,pose_animation_updates=0;
bool pose_require_motion=false,physics_after_result=false;
std::vector<unsigned> lifecycle;
bool enabled=false,fail_alloc=false,fail_stack=false,throw_load=false,throw_model=false,fail_model=false;
std::string consumed;std::filesystem::path directory;
unsigned ModelIndex(){return active_kind==3?4+active_slot:active_slot*2+active_kind;}
uint8_t* Bytes(){return kernel::memory.data();}
uint32_t Get(uint32_t p){uint32_t value;std::memcpy(&value,Bytes()+p,4);return value;}
void Set(uint32_t p,uint32_t value){std::memcpy(Bytes()+p,&value,4);}
void Text(uint32_t p,const std::string& value){std::memcpy(Bytes()+p,value.c_str(),value.size()+1);}
void Reset(){
    kernel::memory.fill(0);TrialCorpseShieldReset();corpse_shield::held={};enabled=true;
    loads=allocations=frees=stacks=stack_frees=resource_mode=registrations=model_calls=0;
    active_slot=active_kind=0;fail_alloc=fail_stack=throw_load=throw_model=fail_model=false;consumed.clear();
    Set(0x51c4dc,world);Set(fs,0xffffffffu);Set(name_cell,name_text);
    for(unsigned slot=0;slot<2;++slot){
        Set(owners[slot],0x46fbe8);Set(owners[slot]+0x54,100+slot);Set(owners[slot]+0x368,slot);
        Set(bodies[slot],0x45d040);Set(bodies[slot]+0x54,200+slot);Set(bodies[slot]+0x58,100+slot);
        Set(children[slot],0x462658);Set(children[slot]+0x54,300+slot);Set(children[slot]+0x58,200+slot);
    }
    for(unsigned kind=0;kind<4;++kind){
        Set(resource[kind],0x457008);Set(resource[kind]+0xc,corpse_shield::counts[kind]);
        if(kind<2)Text(resource[kind]+0x10,corpse_shield::tags[kind]);
        if(kind==3)Text(resource[kind]+0x10,"CorpseShieldPickup");
        Set(resource[kind]+0x3c,authored[kind]);Set(resource[kind]+0x40,metadata[kind]);
    }
    for(unsigned i=0;i<4;++i){Set(renders[i],0x457880);Set(renders[i]+0x68,resource[i%2]);}
    for(unsigned i=4;i<6;++i){Set(renders[i],0x457880);Set(renders[i]+0x68,resource[3]);}
    Set(0x457880+0x2c,0xf0001);Set(0x457880+0x10,0xf0002);
    Set(proxy,proxy_vtable);Set(proxy_vtable+0x54,0xf0003);Set(proxy_vtable+0x24,0xf0004);Set(proxy_vtable+0x1c,0xf0005);
}
X86Context Context(unsigned slot,unsigned kind){
    active_slot=slot;active_kind=kind;Text(name_text,Original(kind));
    X86Context c{};c.ecx=kind?children[slot]:owners[slot];c.esp=stack;c.fs_base=fs;
    c.ebp=0x123456;c.esi=0x234567;c.edi=0x345678;c.ebx=0x678901;c.eax=0x456789;c.edx=0x567890;
    c.fcw=0x37f;c.fsw=0x101;c.top=1;c.mxcsr=0x1f80;c.xflags=0x200000;
    c.cf=c.zf=c.sf=c.of=c.pf=c.df=1;
    for(unsigned i=0;i<8;++i){c.st[i]=i+.25;c.mm[i]=0x1234567800000000ull+i;c.xmm[i].q[0]=i+3;c.xmm[i].q[1]=i+15;}
    Set(stack,kind?0xfe184:0xc8a77);Set(stack+4,name_cell);Set(stack+8,0);return c;
}
void ChangedContext(X86Context& c){
    c.eax=1;c.ecx=2;c.edx=3;c.ebx=4;c.esi=5;c.edi=6;c.ebp=7;c.esp-=64;
    c.cf^=1;c.zf^=1;c.sf^=1;c.of^=1;c.pf^=1;c.df^=1;c.xflags^=0x200000;
    c.top=3;c.fcw=0x27f;c.fsw=0x333;c.mxcsr=0x3f80;
    for(unsigned i=0;i<8;++i){c.st[i]=i+.75;c.mm[i]=0x8765432100000000ull+i;c.xmm[i].q[0]=i+22;c.xmm[i].q[1]=i+42;}
    _mm_setcsr(0x3f80);
}
void Balanced(){assert(allocations==frees&&stacks==stack_frees);}
}
// Native solver contracts are executed separately in test_corpse_shield_physics.
// This fixture checks the renderer bridge with independently supplied poses.
bool TrialCorpseShieldPhysicsValid(const uint8_t*,const TrialCorpseShieldPhysics& s){return s.ragdoll!=0;}
bool TrialCorpseShieldPhysicsCreate(const X86Context&,uint8_t*,TrialCorpseShieldPhysics&,uint32_t,const TrialCorpseShieldMatrix&,uint32_t){return false;}
bool TrialCorpseShieldPhysicsBefore(const X86Context&,uint8_t*,TrialCorpseShieldPhysics&,const TrialCorpseShieldMatrix&,float){return false;}
bool TrialCorpseShieldPhysicsAfter(const X86Context&,uint8_t*,TrialCorpseShieldPhysics&,const TrialCorpseShieldMatrix&,float){return corpse_shield_test::physics_after_result;}
void TrialCorpseShieldPhysicsDestroy(const X86Context&,uint8_t*,TrialCorpseShieldPhysics& s){if(s.ragdoll)corpse_shield_test::lifecycle.push_back(1);s={};}
namespace trial_cheats {bool CorpseShieldEnabled(){return corpse_shield_test::enabled;}}
uint32_t TrialCoopPlayer(unsigned slot){return slot<2?corpse_shield_test::owners[slot]:0;}
uint32_t TrialCoopBodyForPlayer(const uint8_t*,uint32_t owner){
    for(unsigned slot=0;slot<2;++slot)if(owner==corpse_shield_test::owners[slot])return corpse_shield_test::bodies[slot];return 0;
}
namespace mem {
uint32_t AllocSystem(uint32_t size,uint32_t){using namespace corpse_shield_test;assert(size==256);if(fail_alloc)return 0;++allocations;std::memset(Bytes()+allocation,0xcd,size);return allocation;}
void FreeSystem(uint32_t p){using namespace corpse_shield_test;assert(p==allocation);++frees;std::memset(Bytes()+p,0xdd,256);}
uint32_t AllocStack(uint32_t size){using namespace corpse_shield_test;assert(size==0x10000);if(fail_stack)return 0;++stacks;return call_stack;}
void FreeStack(uint32_t p){using namespace corpse_shield_test;assert(p==call_stack);++stack_frees;}
}
[[noreturn]] void x86_trap(X86Context&,uint32_t,const char*){std::abort();}
void x86_resume(X86Context&,uint8_t*,const X86Resume&){assert(false);}
X86_FUNC(sub_0020BC40);
void x86_icall(X86Context& c,uint8_t*,uint32_t target){
    using namespace corpse_shield_test;
    if(target==0x20bc40){sub_0020BC40(c,Bytes());}
    else if(target==0xf0010){assert(c.ecx==pose_render+8);c.eax=proxy;c.esp+=4;}
    else if(target==0xf0001){assert(c.ecx==renders[ModelIndex()]);c.eax=proxy;c.esp+=4;}
    else if(target==0xf0002){assert(c.ecx==renders[ModelIndex()]);c.esp+=8;}
    else{assert(c.ecx==proxy&&target>=0xf0003&&target<=0xf0005);c.esp+=target==0xf0003?12:8;}
}
X86_FUNC(sub_0028A190){ctx.eax=0;ctx.esp+=4;}
X86_FUNC(sub_0021B1A0){ctx.esp+=4;}
X86_FUNC(sub_002D5CCD){ctx.esp+=4;}
X86_FUNC(sub_002D57FB){ctx.eax=corpse_shield_test::wrappers[corpse_shield_test::ModelIndex()];ctx.esp+=4;}
X86_FUNC(sub_0020A200){ctx.eax=0;ctx.esp+=4;}
X86_FUNC(sub_0021D0E0){
    using namespace corpse_shield_test;const unsigned index=ModelIndex();assert(ctx.ecx==wrappers[index]);
    consumed=reinterpret_cast<char*>(Bytes()+Get(ctx.esp+4));++model_calls;
    if(throw_model)throw std::runtime_error("model boundary");
    if(active_kind==3){assert(Get(ctx.esp+8)==placement);Set(renders[index]+0x68,consumed==corpse_shield::alias[3]?resource[3]:0x614000);}
    Set(wrappers[index],fail_model?0:renders[index]);ctx.eax=wrappers[index];ctx.esp+=12;
}
X86_FUNC(sub_0021AFB0){ctx.eax=UINT32_MAX;ctx.esp+=8;}
X86_FUNC(sub_000FE240){ctx.esp+=4;}
X86_FUNC(sub_0021AEC0){ctx.eax=0;ctx.esp+=4;}
X86_FUNC(sub_0021AD80){ctx.esp+=8;}
X86_FUNC(sub_0021ADC0){ctx.esp+=8;}
X86_FUNC(sub_001FB3B0){++corpse_shield_test::registrations;ctx.esp+=8;}
X86_FUNC(sub_0021AC70){ctx.esp+=4;}
X86_FUNC(sub_0021ACE0){ctx.esp+=4;}
X86_FUNC(sub_0021CB90){ctx.esp+=8;}
X86_FUNC(sub_001F1B80){ctx.esp+=4;}
X86_FUNC(sub_001F1700){++corpse_shield_test::pose_animation_updates;ctx.esp+=8;}
namespace corpse_shield_test {
void Compose(X86Context& ctx){
    assert(ctx.ecx==pose_render&&pose_animation_updates>0);
    const uint32_t locals=Get(pose_render+0x6c),authored=Get(Get(pose_render+0x68)+0x3c);
    if(pose_require_motion)assert(std::memcmp(Bytes()+locals+83*64,Bytes()+authored+83*64,64));
    ++pose_compositions;
}
}
X86_FUNC(sub_000B8B60){corpse_shield_test::Compose(ctx);ctx.esp+=12;}
X86_FUNC(sub_000B8E40){corpse_shield_test::Compose(ctx);ctx.esp+=16;}
X86_FUNC(sub_000B9450){ctx.esp+=4;}
X86_FUNC(sub_001FAB30){ctx.esp+=8;}
X86_FUNC(sub_0005AAB0){ctx.eax=corpse_shield_test::world;ctx.esp+=4;}
X86_FUNC(sub_0005B140){ctx.eax=corpse_shield_test::world;ctx.esp+=4;}
X86_FUNC(sub_00016AB0){ctx.eax=0;ctx.esp+=4;}
X86_FUNC(sub_00047A80){ctx.esp+=4;}
X86_FUNC(sub_001FB260){ctx.esp+=12;}
X86_FUNC(sub_000BCCB0){ctx.esp+=4;}
// NATIVE_CORPSE_SHIELD_LOADERS
namespace kernel {
std::string ResolvePath(const std::string& path,uint32_t){
    using namespace corpse_shield_test;assert(path=="D:\\corpseshield.lst");return (directory/"corpseshield.lst").string();
}
uint32_t CallGuest(X86Context& c,uint32_t address,const uint32_t* args,size_t count){
    using namespace corpse_shield_test;
    if(address==0x21b1a0){assert(!count);lifecycle.push_back(2);return 0;}
    if(address==0x2d5ccd){assert(count==1);lifecycle.push_back(3);return 0;}
    if(address==0xbe050||address==0xfe170){
        assert(count==2&&c.esp==call_stack-16&&corpse_shield::restoring);
        bool found=false;
        for(unsigned slot=0;slot<2;++slot)for(unsigned kind=0;kind<2;++kind)if(c.ecx==(kind?children[slot]:owners[slot])){active_slot=slot;active_kind=kind;found=true;}
        assert(found);c.esp=call_stack-0x100;Set(c.esp,0x123456);Set(c.esp+4,args[0]);Set(c.esp+8,args[1]);
        if(address==0xbe050)NativeShield(c,Bytes());else NativeThirdShield(c,Bytes());return c.eax;
    }
    assert(address==0x54b70&&count==1&&c.esp==call_stack-16&&c.df==0);++loads;
    assert(args[0]==allocation+16&&!std::memcmp(Bytes()+args[0],"corpseshield",sizeof("corpseshield")));Set(fs,0x660123);
    if(throw_load){ChangedContext(c);throw std::runtime_error("load boundary");}
    for(unsigned kind=0;kind<4&&resource_mode!=11;++kind){
        Text(observer_name,resource_mode==3?"wrong\\model.amf":corpse_shield::alias[kind]);
        X86Context event=c;event.esp=call_stack-0x100;Set(event.esp+0x10,observer_name);
        event.eax=resource_mode==1||resource_mode==4&&kind==1?0:resource[kind];
        if(resource_mode==6&&kind==3)event.eax=0;
        if(resource_mode==2)Set(resource[kind],0x123456);
        if(resource_mode==5)Set(resource[kind]+0x3c,0);
        if(kind==3){
            if(resource_mode==7)Text(resource[kind]+0x10,"CorpseShieldFP");
            if(resource_mode==8)Set(resource[kind]+0xc,1);
            if(resource_mode==9)Set(resource[kind],0x123456);
            if(resource_mode==10)Set(resource[kind]+0x3c,0);
        }
        TrialCorpseShieldResourceLoaded(event,Bytes());
    }
    ChangedContext(c);return 0;
}
}
namespace corpse_shield_test {
void Run(X86Context& c,unsigned kind){if(kind)sub_0020BC40(c,Bytes());else NativeShield(c,Bytes());}
bool Bound(uint32_t owner){const auto& r=corpse_shield::ready[Get(owner+0x368)*2];return enabled&&r.owner==owner&&corpse_shield::Current(Bytes(),r,0);}
void LoaderCases(){
    for(unsigned slot=0;slot<2;++slot)for(unsigned kind=0;kind<2;++kind)for(unsigned flags:{0u,0x12u}){
        Reset();auto c=Context(slot,kind);const auto initial=c;Set(c.ecx+0x60,flags);_mm_setcsr(0x5f80);Run(c,kind);
        assert(consumed==corpse_shield::alias[kind]&&uint8_t(c.eax));assert(loads==1&&model_calls==1);
        assert(c.esp==stack+12&&c.esi==initial.esi&&c.edi==initial.edi&&c.ebp==initial.ebp&&c.ebx==initial.ebx);
        assert(c.fcw==initial.fcw&&c.fsw==initial.fsw&&c.top==initial.top&&c.mxcsr==initial.mxcsr);
        assert(!std::memcmp(c.st,initial.st,sizeof c.st)&&!std::memcmp(c.xmm,initial.xmm,sizeof c.xmm));
        assert(_mm_getcsr()==0x5f80&&Get(fs)==0xffffffffu);
        assert(Get(name_cell)==name_text&&std::string(reinterpret_cast<char*>(Bytes()+name_text))==Original(kind));
        const auto& r=corpse_shield::ready[slot*2+kind];assert(corpse_shield::Current(Bytes(),r,kind));
        assert(Bound(owners[slot])==!kind);Balanced();
        if(!kind){
            // Weapon reload replaces +74; the resources remain cached.
            c=Context(slot,kind);Run(c,kind);assert(model_calls==2&&registrations==0&&loads==1);
            assert(Bound(owners[slot]));
        }
    }
    for(unsigned variant=1;variant<3;++variant){Reset();auto c=Context(0,0);Text(name_text,native_model_names[variant]);
        Run(c,0);assert(consumed==corpse_shield::alias[0]&&Bound(owners[0]));Balanced();}
    // The real CC6BF call passes the flashlight path; its independent +660
    // allocation must survive unchanged, without aliasing or losing shield readiness.
    Reset();auto shield=Context(0,0);Run(shield,0);const auto proof=corpse_shield::ready[0];
    auto light=Context(0,0);Text(name_text,native_model_names[3]);Set(stack,0xcc6c4);
    active_kind=1;NativeFlashlight(light,Bytes());assert(consumed==native_model_names[3]&&loads==1&&registrations==1);
    assert(Get(owners[0]+0x660)==wrappers[1]&&Get(owners[0]+0x74)==proof.wrapper);
    assert(Bound(owners[0]));Balanced();
    // Switching to any other first-person equipment retires only FP readiness.
    auto other=Context(0,0);Text(name_text,"other\\weapon.amf");Run(other,0);
    assert(consumed=="other\\weapon.amf"&&!Bound(owners[0]));Balanced();
    Reset();for(unsigned slot=0;slot<2;++slot)for(unsigned kind=0;kind<2;++kind){auto c=Context(slot,kind);Run(c,kind);}
    assert(loads==1&&registrations==0);Balanced();
    for(unsigned slot=0;slot<2;++slot)assert(Bound(owners[slot]));
    for(unsigned slot=0;slot<2;++slot){
        const unsigned index=slot*2;const auto saved=corpse_shield::ready[index];
        Set(owners[slot]+0x54,999);assert(!Bound(owners[slot]));Set(owners[slot]+0x54,saved.id);
        Set(owners[slot]+0x74,wrappers[index]+8);assert(!Bound(owners[slot]));Set(owners[slot]+0x74,wrappers[index]);
        Set(renders[index]+0x68,0);assert(!Bound(owners[slot]));Set(renders[index]+0x68,resource[0]);
        Set(owners[slot]+0x5c,1);assert(!Bound(owners[slot]));Set(owners[slot]+0x5c,0);
        Set(0x51c4dc,world+0x100);assert(!Bound(owners[slot]));Set(0x51c4dc,world);
    }
    enabled=false;assert(!Bound(owners[0])&&!Bound(owners[1]));
    TrialCorpseShieldReset();assert(corpse_shield::resources[0]==0&&corpse_shield::resources[1]==0);
    for(unsigned slot=0;slot<2;++slot)for(unsigned kind=0;kind<2;++kind){
        Set((kind?children[slot]:owners[slot])+0x74,0);
        auto c=Context(slot,kind);Run(c,kind);assert(consumed==Original(kind));
        assert(!corpse_shield::Current(Bytes(),corpse_shield::ready[slot*2+kind],kind));
    }
    assert(loads==1);Balanced();
}
void RestoreCases(){
    for(unsigned slot=0;slot<2;++slot)for(unsigned variant=0;variant<3;++variant){
        Reset();auto c=Context(slot,0);Text(name_text,native_model_names[variant]);Run(c,0);
        assert(Bound(owners[slot]));const auto saved=c;_mm_setcsr(0x5f80);
        corpse_shield::RestoreShield(c,Bytes(),slot);
        assert(consumed==(variant?corpse_shield::upgrades[variant-1]:corpse_shield::original[0]));
        assert(!corpse_shield::ready[slot*2].owner&&!corpse_shield::restoring&&loads==1);
        assert(!std::memcmp(&saved,&c,sizeof c)&&_mm_getcsr()==0x5f80);Balanced();
    }
    Reset();auto a=Context(0,0);Run(a,0);auto b=Context(0,1);Run(b,1);
    corpse_shield::RestoreShield(b,Bytes(),0);assert(consumed==corpse_shield::original[1]&&model_calls==4&&loads==1);Balanced();
    for(bool native_failure:{false,true}){
        Reset();auto c=Context(0,0);Run(c,0);auto& h=corpse_shield::held[0];
        h.owner=owners[0];h.id=100;h.wrapper=0x6e0000;h.render=0x6e1000;h.physics.ragdoll=1;h.posed=true;
        Set(h.wrapper,h.render);Set(h.render,0x457880);Set(h.render+0x70,0x6e2000);
        const float invalid=std::nanf("");std::memcpy(Bytes()+0x6e2000+8*64,&invalid,4);
        physics_after_result=!native_failure;lifecycle.clear();
        TrialCorpseShieldAfterPhysics(c,Bytes(),1.f/60.f);
        assert(!h.wrapper&&!h.physics.ragdoll&&!corpse_shield::ready[0].owner);
        assert(consumed==corpse_shield::original[0]&&lifecycle==std::vector<unsigned>({1,2,3}));Balanced();
    }

}
void FailureCases(){
    for(unsigned kind=0;kind<2;++kind)for(unsigned mode=0;mode<12;++mode){
        Reset();auto c=Context(0,kind);
        switch(mode){
        case 0:enabled=false;break;
        case 1:Set(owners[0],0x45c770);break;
        case 2:Set(owners[0]+0x54,UINT32_MAX);break;
        case 3:Text(name_text,"other\\model.amf");break;
        case 4:fail_alloc=true;break;
        case 5:fail_stack=true;break;
        case 6:case 7:case 8:case 9:case 10:resource_mode=mode-5;break;
        case 11:std::filesystem::rename(directory/"corpseshield.lst",directory/"absent.lst");break;
        }
        Run(c,kind);assert(consumed==(mode==3?"other\\model.amf":Original(kind)));
        assert(!Bound(owners[0]));
        assert(!corpse_shield::Current(Bytes(),corpse_shield::ready[kind],kind));Balanced();
        if(mode==11)std::filesystem::rename(directory/"absent.lst",directory/"corpseshield.lst");
    }
    for(unsigned kind=0;kind<2;++kind)for(bool during_load:{true,false}){
        Reset();auto c=Context(0,kind);throw_load=during_load;throw_model=!during_load;_mm_setcsr(0x5f80);
        try{Run(c,kind);assert(false);}catch(const std::runtime_error&){}
        assert(!Bound(owners[0])&&!corpse_shield::loading);
        assert(_mm_getcsr()==0x5f80&&Get(fs)==stack-(kind?12:36));Balanced();
    }
    for(unsigned kind=0;kind<2;++kind){Reset();auto c=Context(0,kind);Set(stack,kind?0xfe184:0xbe075);
        fail_model=true;sub_0020BC40(c,Bytes());
        assert(!uint8_t(c.eax)&&!Bound(owners[0]));Balanced();}
    Reset();auto unknown=Context(0,0);Set(stack,0x12345);sub_0020BC40(unknown,Bytes());
    assert(consumed==Original(0)&&loads==0&&!Bound(owners[0]));Balanced();
    // A matching path on unrelated ThirdPersonGun children cannot enable it.
    for(unsigned mode=0;mode<3;++mode){Reset();auto c=Context(0,1);
        if(mode==0)Set(stack,0x12345);if(mode==1)Set(children[0]+0x58,999);if(mode==2)Set(children[0],0x4753a0);
        Run(c,1);assert(consumed==corpse_shield::original[1]&&loads==0);Balanced();}
}
X86Context PickupContext(unsigned slot){
    auto c=Context(slot,0);active_kind=3;c.ecx=props[slot];Set(stack,0x1e27d5);Set(stack+4,descriptors[slot]);
    Text(name_text,corpse_shield::pickup_original);Set(0x47e278+0x28,0x20bc40);
    Set(world+4,level);Set(level+0x2054,provider);Set(provider+4,prop_slots);Set(provider+0x10,1024);
    for(unsigned i=0;i<2;++i){
        Set(props[i],0x47e278);Set(props[i]+0x50,source_indices[i]);Set(props[i]+0x54,1200+i);
        Set(prop_slots+source_indices[i]*4,prop_entries[i]);Set(prop_entries[i]+0xc,descriptors[i]);
        Set(descriptors[i]+0x4c,name_text);Set(descriptors[i]+0x54,placement);
    }
    return c;
}
void PickupCases(){
    for(bool on:{false,true})for(unsigned slot=0;slot<2;++slot){
        Reset();auto c=PickupContext(slot);enabled=on;const auto initial=c;
        std::array<uint8_t,0x58> descriptor;std::memcpy(descriptor.data(),Bytes()+descriptors[slot],descriptor.size());
        _mm_setcsr(0x5f80);NativePickup(c,Bytes());
        assert(consumed==(on?corpse_shield::alias[3]:corpse_shield::pickup_original));
        assert(loads==unsigned(on)&&model_calls==1&&uint8_t(c.eax));
        assert(c.esp==stack+8&&c.esi==initial.esi&&c.edi==initial.edi&&c.ebp==initial.ebp&&c.ebx==initial.ebx);
        assert(c.fcw==initial.fcw&&c.fsw==initial.fsw&&c.top==initial.top&&c.mxcsr==initial.mxcsr);
        assert(!std::memcmp(c.st,initial.st,sizeof c.st)&&!std::memcmp(c.xmm,initial.xmm,sizeof c.xmm));
        assert(_mm_getcsr()==0x5f80&&Get(fs)==0xffffffffu);
        assert(!std::memcmp(descriptor.data(),Bytes()+descriptors[slot],descriptor.size()));
        assert(Get(props[slot]+0x50)==source_indices[slot]&&Get(props[slot]+0x54)==1200+slot);
        assert(Get(props[slot]+0x74)==wrappers[4+slot]&&Get(renders[4+slot]+0x68)==(on?resource[3]:0x614000));
        for(const auto& r:corpse_shield::ready)assert(!r.owner);
        for(const auto& h:corpse_shield::held)assert(!h.owner&&!h.wrapper&&!h.physics.ragdoll);
        Balanced();
    }
    // Loading both scenery models must not replace either owner's held state.
    Reset();for(unsigned slot=0;slot<2;++slot)for(unsigned kind=0;kind<2;++kind){auto c=Context(slot,kind);Run(c,kind);}
    for(unsigned slot=0;slot<2;++slot){auto& h=corpse_shield::held[slot];h.owner=owners[slot];h.id=100+slot;h.posed=true;}
    std::array<uint8_t,sizeof corpse_shield::held> held_before;
    std::array<uint8_t,sizeof corpse_shield::ready> ready_before;
    std::memcpy(held_before.data(),corpse_shield::held.data(),held_before.size());
    std::memcpy(ready_before.data(),corpse_shield::ready.data(),ready_before.size());
    for(unsigned slot=0;slot<2;++slot){auto c=PickupContext(slot);NativePickup(c,Bytes());
        assert(consumed==corpse_shield::alias[3]&&Get(props[slot]+0x74)==wrappers[4+slot]);
        TrialCorpseShieldPose(Bytes(),renders[4+slot]);
        assert(!std::memcmp(held_before.data(),corpse_shield::held.data(),sizeof held_before));
        assert(!std::memcmp(ready_before.data(),corpse_shield::ready.data(),sizeof ready_before));
    }
    assert(loads==1&&model_calls==6&&registrations==0);Balanced();
    for(unsigned mode=0;mode<22;++mode){
        Reset();auto c=PickupContext(0);bool direct=false;
        switch(mode){
        case 0:enabled=false;break;
        case 1:Set(props[0],0x47e000);Set(0x47e000+0x28,0x20bc40);break;
        case 2:Set(props[0]+0x54,UINT32_MAX);break;
        case 3:Set(props[0]+0x50,577);break;
        case 4:Text(name_text,"other\\model.amf");break;
        case 5:Set(world+4,0);break;
        case 6:Set(level+0x2054,0);break;
        case 7:Set(prop_slots+source_indices[0]*4,0);break;
        case 8:Set(prop_entries[0]+0xc,0);break;
        case 9:Set(props[0]+0x5c,1);break;
        case 10:Set(provider+0x10,source_indices[0]);break;
        case 11:direct=true;Set(stack,0xbe075);Set(stack+4,name_cell);Set(stack+8,placement);break;
        case 12:case 13:case 14:case 15:case 16:resource_mode=mode-6;break;
        case 17:fail_alloc=true;break;
        case 18:fail_stack=true;break;
        case 19:direct=true;Text(name_text+256,corpse_shield::pickup_original);Set(name_cell,name_text+256);
            Set(stack,0x20a1f1);Set(stack+4,name_cell);Set(stack+8,placement);break;
        case 20:Set(provider+0x10,16385);break;
        case 21:Set(provider+4,0xfffffff0);break;
        }
        if(direct)sub_0020BC40(c,Bytes());else NativePickup(c,Bytes());
        assert(consumed==(mode==4?"other\\model.amf":corpse_shield::pickup_original));
        for(const auto& r:corpse_shield::ready)assert(!r.owner);
        for(const auto& h:corpse_shield::held)assert(!h.owner&&!h.wrapper&&!h.physics.ragdoll);
        Balanced();
    }
    // A prior recipe lacks only the static alias: held shields still work.
    for(bool held_first:{false,true}){
        Reset();resource_mode=6;
        if(held_first){auto shield=Context(0,0);Run(shield,0);assert(Bound(owners[0]));}
        auto pickup=PickupContext(0);NativePickup(pickup,Bytes());assert(consumed==corpse_shield::pickup_original);
        auto shield=Context(0,0);Run(shield,0);assert(consumed==corpse_shield::alias[0]&&Bound(owners[0]));Balanced();
    }
    // A repeated archive load may publish no observations; it must not erase
    // still-valid held resources while probing an absent pickup alias.
    Reset();resource_mode=6;auto held=Context(0,0);Run(held,0);resource_mode=11;
    auto pickup=PickupContext(0);NativePickup(pickup,Bytes());assert(consumed==corpse_shield::pickup_original&&loads==2);
    for(unsigned kind=0;kind<3;++kind)assert(corpse_shield::resources[kind]==resource[kind]);
    held=Context(0,0);Run(held,0);assert(consumed==corpse_shield::alias[0]&&Bound(owners[0])&&loads==2);Balanced();
    for(bool during_load:{true,false}){
        Reset();auto c=PickupContext(0);throw_load=during_load;throw_model=!during_load;_mm_setcsr(0x5f80);
        try{NativePickup(c,Bytes());assert(false);}catch(const std::runtime_error&){}
        assert(!corpse_shield::loading&&_mm_getcsr()==0x5f80);
        for(const auto& r:corpse_shield::ready)assert(!r.owner);
        Balanced();
    }
}
}
namespace corpse_shield_test {
void QueryCases(){
    Reset();
    constexpr uint32_t clone[]={0x6d0000,0x6d0200},limbs[]={0x6d1000,0x6d4000};
    constexpr uint32_t group[]={0x123,0x456};
    for(unsigned slot=0;slot<2;++slot){
        auto c=Context(slot,0);Run(c,0);Set(owners[slot]+0x94,group[slot]);
        auto& h=corpse_shield::held[slot];h.owner=owners[slot];h.id=100+slot;h.posed=true;h.physics.ragdoll=clone[slot];
        Set(clone[slot]+0x70,clone[slot]+0x100);Set(clone[slot]+0x74,12);
        for(unsigned i=0;i<12;++i){const uint32_t body=limbs[slot]+i*0x200;
            Set(clone[slot]+0x100+i*4,body);Set(body+0x24,uint32_t(-0x14));
            Set(body+0x30,0x24800000|uint32_t(0x800+slot*12+i));
        }
    }
    const std::vector<uint8_t> untouched(Bytes(),Bytes()+kernel::memory.size());
    for(unsigned slot=0;slot<2;++slot)for(unsigned i=0;i<12;++i){
        const uint32_t collidable=limbs[slot]+i*0x200+0x14;
        // Both holders can bash/shoot through their own shield. Incoming NPC
        // and partner queries retain the original blocking decision.
        for(uint32_t mask:{0u,0x24800000u,0xfff00000u}){
            assert(TrialCorpseShieldIgnoreQuery(Bytes(),collidable,mask|group[slot]));
            assert(!TrialCorpseShieldIgnoreQuery(Bytes(),collidable,mask|group[1-slot]));
            assert(!TrialCorpseShieldIgnoreQuery(Bytes(),collidable,mask|0x789));
        }
    }
    assert(!std::memcmp(untouched.data(),Bytes(),untouched.size()));
    const uint32_t collidable=limbs[0]+0x14;
    assert(!TrialCorpseShieldIgnoreQuery(nullptr,collidable,group[0]));
    for(uint32_t invalid:{0u,0xfffffff0u,0x800000u,collidable+4})assert(!TrialCorpseShieldIgnoreQuery(Bytes(),invalid,group[0]));
    // An ordinary corpse with identical filter bits and property18 is not the held clone.
    constexpr uint32_t ordinary=0x6d7000;Set(ordinary+0x24,uint32_t(-0x14));Set(ordinary+0x30,Get(limbs[0]+0x30));
    Set(ordinary+0x44,ordinary+0x100);Set(ordinary+0x48,1);Set(ordinary+0x100,0x18);Set(ordinary+0x108,group[0]);
    assert(!TrialCorpseShieldIgnoreQuery(Bytes(),ordinary+0x14,group[0]));
    // Matching a low 16-bit suffix alone must not mistake an unrelated query
    // group for the holder; original cast filters carry the full low 20 bits.
    assert(!TrialCorpseShieldIgnoreQuery(Bytes(),collidable,0x10000|group[0]));
    for(unsigned bad=0;bad<14;++bad){
        auto& h=corpse_shield::held[0];const auto saved=h;
        uint32_t address=0,value=0;
        switch(bad){
        case 0:enabled=false;break;
        case 1:h.posed=false;break;
        case 2:++h.id;break;
        case 3:h.owner=owners[1];break;
        case 4:h.physics.ragdoll=0;break;
        case 5:address=owners[0]+0x54;value=999;break;
        case 6:address=0x51c4dc;value=world+0x100;break;
        case 7:address=owners[0]+0x74;value=0;break;
        case 8:address=clone[0]+0x70;value=0;break;
        case 9:address=clone[0]+0x74;value=11;break;
        case 10:address=clone[0]+0x100;value=ordinary;break;
        case 11:address=collidable+0x10;value=0;break;
        case 12:address=owners[0]+0x94;value=0x789;break;
        case 13:address=owners[0]+0x5c;value=1;break;
        }
        const uint32_t old=address?Get(address):0;if(address)Set(address,value);
        assert(!TrialCorpseShieldIgnoreQuery(Bytes(),collidable,group[0]));
        assert(!TrialCorpseShieldIgnoreQuery(Bytes(),collidable,group[1]));
        if(address)Set(address,old);h=saved;enabled=true;
        assert(TrialCorpseShieldIgnoreQuery(Bytes(),collidable,group[0]));
    }
    TrialCorpseShieldReset();assert(!TrialCorpseShieldIgnoreQuery(Bytes(),collidable,group[0]));
    Balanced();
}
}
#include "test_xbe_corpse_shield_motion.h"
int main(){
    using namespace corpse_shield_test;
    directory=std::filesystem::temp_directory_path()/"urban-chaos-corpse-shield-fixture";
    std::filesystem::create_directories(directory);
    {std::ofstream stream(directory/"corpseshield.lst",std::ios::binary);const std::array<char,256> bytes{};stream.write(bytes.data(),bytes.size());}
    LoaderCases();RestoreCases();FailureCases();PickupCases();QueryCases();BridgeCases();
    std::filesystem::remove(directory/"corpseshield.lst");std::filesystem::remove(directory);
    std::puts("Corpse shield native loader, static pickup, ownership, fallback and native pose bridge checks passed");
}
