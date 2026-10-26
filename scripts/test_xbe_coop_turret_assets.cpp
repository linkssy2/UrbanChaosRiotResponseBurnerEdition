// Actual 20BC40 model replacement and 8DB10 resource-result instructions run.
// AMF parsing, filesystem services and guest allocation are explicit boundaries;
// the companion asset test verifies real input bytes and unchanged cabin draws.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include "kernel/kernel.h"
namespace kernel {static std::array<uint8_t,0x800000> memory{};}
namespace mem {bool IsCommitted(uint32_t page){return page<kernel::memory.size();}}
#include "../common/xbe_coop_turret_assets.cpp"
namespace turret_asset_test {
constexpr uint32_t vehicle=0x600000,world=0x602000,name_cell=0x604000,name_text=0x604010;
constexpr uint32_t wrapper=0x606000,render=0x608000,resource=0x60a000,platform=0x60c000,vtable=0x60d000;
constexpr uint32_t allocation=0x650000,call_stack=0x680000,observer_name=0x60e000,stack=0x710000,fs=0x720000;
unsigned loads=0,allocations=0,frees=0,stacks=0,stack_frees=0,resources_mode=0;
bool fail_alloc=false,fail_stack=false,throw_load=false,throw_model=false,fail_model=false;
std::string consumed;std::filesystem::path directory;
uint8_t* Bytes(){return kernel::memory.data();}
uint32_t Get(uint32_t p){uint32_t value;std::memcpy(&value,Bytes()+p,4);return value;}
void Set(uint32_t p,uint32_t value){std::memcpy(Bytes()+p,&value,4);}
void Text(uint32_t p,const std::string& value){std::memcpy(Bytes()+p,value.c_str(),value.size()+1);}
X86Context Seed(unsigned level,bool reset=true){
    kernel::memory.fill(0);trial_coop::Reset();trial_coop::SetAvailable(true);assert(trial_coop::Request());
    if(reset){TrialCoopTurretAssetsReset();loads=0;}
    allocations=frees=stacks=stack_frees=0;resources_mode=0;fail_alloc=fail_stack=throw_load=throw_model=fail_model=false;consumed.clear();
    Set(vehicle,0x4753a0);Set(vehicle+0x54,381);Set(0x51c4dc,world);
    Set(name_cell,name_text);Text(name_text,original[level]);Set(platform,vtable);Set(vtable+12,0xf0001);
    Set(resource,0x457008);Set(render,0x457880);
    X86Context c{};c.ecx=vehicle;c.esp=stack;c.fs_base=fs;c.ebp=0x123456;c.esi=0x234567;c.edi=0x345678;
    c.eax=0x456789;c.edx=0x567890;c.ebx=0x678901;c.fcw=0x37f;c.fsw=0x101;c.top=1;c.mxcsr=0x1f80;
    c.cf=c.zf=c.sf=c.of=c.pf=c.df=1;c.xflags=0x200000;
    for(unsigned i=0;i<8;++i){c.st[i]=i+.25;c.mm[i]=0x1234567800000000ull+i;c.xmm[i].q[0]=i+3;c.xmm[i].q[1]=i+15;}
    Set(stack,0x199825);Set(stack+4,name_cell);Set(stack+8,0);Set(fs,0xffffffffu);return c;
}
void ChangedContext(X86Context& c){
    c.eax=1;c.ecx=2;c.edx=3;c.ebx=4;c.esi=5;c.edi=6;c.ebp=7;c.esp-=64;
    c.cf^=1;c.zf^=1;c.sf^=1;c.of^=1;c.pf^=1;c.df^=1;c.xflags^=0x200000;
    c.top=3;c.fcw=0x27f;c.fsw=0x333;c.mxcsr=0x3f80;
    for(unsigned i=0;i<8;++i){c.st[i]=i+.75;c.mm[i]=0x8765432100000000ull+i;c.xmm[i].q[0]=i+22;c.xmm[i].q[1]=i+42;}
    _mm_setcsr(0x3f80);
}
}
namespace mem {
uint32_t AllocSystem(uint32_t size,uint32_t){using namespace turret_asset_test;assert(size==256);if(fail_alloc)return 0;++allocations;return allocation;}
void FreeSystem(uint32_t p){using namespace turret_asset_test;assert(p==allocation);++frees;std::memset(Bytes()+p,0xdd,256);}
uint32_t AllocStack(uint32_t size){using namespace turret_asset_test;assert(size==0x10000);if(fail_stack)return 0;++stacks;return call_stack;}
void FreeStack(uint32_t p){using namespace turret_asset_test;assert(p==call_stack);++stack_frees;}
}
[[noreturn]] void x86_trap(X86Context&,uint32_t,const char*){std::abort();}
void x86_resume(X86Context&,uint8_t*,const X86Resume&){assert(false);}
void x86_icall(X86Context& c,uint8_t*,uint32_t target){
    using namespace turret_asset_test;assert(target==0xf0001&&c.ecx==platform);
    const std::string name(reinterpret_cast<char*>(Bytes()+Get(Get(c.esp+4))));
    c.eax=name.ends_with(".amf")&&resources_mode!=1?resource:0;
    if(resources_mode==2)Set(resource,0x123456);c.esp+=8;
}
X86_FUNC(sub_0021B1A0){ctx.esp+=4;}
X86_FUNC(sub_002D5CCD){ctx.esp+=4;}
X86_FUNC(sub_002D57FB){ctx.eax=turret_asset_test::wrapper;ctx.esp+=4;}
X86_FUNC(sub_0020A200){ctx.eax=0;ctx.esp+=4;}
X86_FUNC(sub_0021D0E0){
    using namespace turret_asset_test;assert(ctx.ecx==wrapper);
    consumed=reinterpret_cast<char*>(Bytes()+Get(ctx.esp+4));
    if(throw_model)throw std::runtime_error("model boundary");
    Set(wrapper,fail_model?0:render);ctx.eax=wrapper;ctx.esp+=12;
}
X86_FUNC(sub_0021AEC0){ctx.eax=0;ctx.esp+=4;}
X86_FUNC(sub_0021AD80){ctx.esp+=8;}
X86_FUNC(sub_0021ADC0){ctx.esp+=8;}
// ORIGINAL_COOP_TURRET_ASSET_WITNESSES
namespace kernel {
std::string ResolvePath(const std::string& path,uint32_t){
    using namespace turret_asset_test;
    assert(path=="D:\\coopturrets03.lst"||path=="D:\\coopturrets08.lst");return (directory/path.substr(3)).string();
}
uint32_t CallGuest(X86Context& c,uint32_t address,const uint32_t* args,size_t count){
    using namespace turret_asset_test;assert(address==0x54b70&&count==1&&c.esp==call_stack-16&&c.df==0);++loads;
    // The nested guest's temporary exception record must never replace the
    // active 20BC40 SEH chain after the helper returns or throws.
    Set(fs,0x660123);
    const std::string archive(reinterpret_cast<char*>(Bytes()+args[0]));
    const unsigned level=archive=="coopturrets03"?0:1;assert(archive==archives[level]);
    if(throw_load){ChangedContext(c);throw std::runtime_error("load boundary");}
    for(const char* extension:{".amf",".mdlhk",".entsupp"}){
        std::string name=alias[level];name.replace(name.size()-4,4,extension);
        if(resources_mode==3)name="wrong\\model.amf";
        Text(observer_name,name);X86Context observed=c;observed.esp=call_stack-0x100;
        Set(observed.esp+0x10,observer_name);observed.eax=observed.esp+0x10;
        observed.edi=platform;observed.ebp=vtable;ResourceResult(observed,Bytes());
    }
    ChangedContext(c);return 0;
}
}
namespace turret_asset_test {
void NativeModelCases(){
    for(unsigned level=0;level<2;++level)for(unsigned flags:{0u,0x1000u}){
        auto original_context=Seed(level);Set(vehicle+0x5c,flags);_mm_setcsr(0x5f80);
        OriginalModel(original_context,Bytes());const auto expected_csr=_mm_getcsr();
        assert(consumed==original[level]&&!TrialCoopTurretAssetReady(Bytes(),vehicle));
        auto fixed=Seed(level);Set(vehicle+0x5c,flags);_mm_setcsr(0x5f80);CoopModel(fixed,Bytes());
        assert(consumed==alias[level]&&TrialCoopTurretAssetReady(Bytes(),vehicle));
        assert(!std::memcmp(&original_context,&fixed,sizeof fixed));assert(_mm_getcsr()==expected_csr);
        assert(Get(name_cell)==name_text&&std::string(reinterpret_cast<char*>(Bytes()+name_text))==original[level]);
        assert(Get(fs)==0xffffffffu);
        assert(loads==1&&allocations==frees&&stacks==stack_frees);
        // Same-world model recreation reuses only successfully parsed resources.
        fixed=Seed(level,false);Set(vehicle+0x5c,flags);CoopModel(fixed,Bytes());assert(loads==1);
        assert(TrialCoopTurretAssetReady(Bytes(),vehicle));
        Set(vehicle+0x54,382);assert(!TrialCoopTurretAssetReady(Bytes(),vehicle));Set(vehicle+0x54,381);
        Set(vehicle+0x74,wrapper+8);assert(!TrialCoopTurretAssetReady(Bytes(),vehicle));Set(vehicle+0x74,wrapper);
        Set(vehicle+0x5c,flags|1);assert(!TrialCoopTurretAssetReady(Bytes(),vehicle));Set(vehicle+0x5c,flags);
        TrialCoopTurretAssetsReset();assert(!TrialCoopTurretAssetReady(Bytes(),vehicle));
    }
}
void FailureAndQualification(){
    for(unsigned mode=0;mode<12;++mode){
        auto c=Seed(0);
        switch(mode){
        case 0:trial_coop::Reset();break;
        case 1:trial_coop::SetAvailable(false);break;
        case 2:Set(vehicle,0x46fbe8);break;
        case 3:Set(stack,0x12345);break;
        case 4:Text(name_text,"other\\model.amf");break;
        case 5:Set(0x51c4dc,0);break;
        case 6:fail_alloc=true;break;
        case 7:fail_stack=true;break;
        case 8:resources_mode=1;break;
        case 9:resources_mode=2;break;
        case 10:resources_mode=3;break;
        case 11:std::filesystem::rename(directory/"coopturrets03.lst",directory/"absent.lst");break;
        }
        CoopModel(c,Bytes());assert(!TrialCoopTurretAssetReady(Bytes(),vehicle));
        assert(consumed==(mode==4?"other\\model.amf":original[0]));assert(allocations==frees&&stacks==stack_frees);
        if(mode==11)std::filesystem::rename(directory/"absent.lst",directory/"coopturrets03.lst");
        if(mode>=8&&mode<=10){
            const unsigned attempts=loads;c=Seed(0,false);CoopModel(c,Bytes());assert(loads==attempts+1);
            assert(TrialCoopTurretAssetReady(Bytes(),vehicle));
        }
    }
    for(bool during_load:{true,false}){
        auto c=Seed(0);throw_load=during_load;throw_model=!during_load;_mm_setcsr(0x5f80);
        try{CoopModel(c,Bytes());assert(false);}catch(const std::runtime_error&){}
        assert(_mm_getcsr()==0x5f80&&!TrialCoopTurretAssetReady(Bytes(),vehicle));
        // The original model callback's live SEH record remains. Its native
        // C++ exception boundary has not run a guest epilogue in this witness.
        assert(Get(fs)==stack-12);
        assert(allocations==frees&&stacks==stack_frees&&Get(name_cell)==name_text);
    }
    auto c=Seed(0);fail_model=true;CoopModel(c,Bytes());assert(!TrialCoopTurretAssetReady(Bytes(),vehicle));
}
}
int main(){
    using namespace turret_asset_test;
    directory=std::filesystem::temp_directory_path()/"urban-chaos-coop-turret-assets-fixture";
    std::filesystem::create_directories(directory);
    for(const char* name:{"coopturrets03.lst","coopturrets08.lst"}){
        std::ofstream stream(directory/name,std::ios::binary);const std::array<char,256> bytes{};stream.write(bytes.data(),bytes.size());
    }
    NativeModelCases();FailureAndQualification();
    std::filesystem::remove(directory/"coopturrets03.lst");std::filesystem::remove(directory/"coopturrets08.lst");
    std::filesystem::remove(directory);std::puts("Co-op helicopter alias native caller checks passed");
}
