// Actual 22DFA0 normalization, 232D80 source addresses and 22EDF0 descriptor
// evaluation. Only XInput polling and the saved-profile lookup are substituted.
#undef NDEBUG
#include <array>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <xmmintrin.h>
#include "xbe_controls.h"
constexpr uint16_t controller_digital[]={16,32,64,128,1,2,4,8};
// CONTROLLER_BINDING_HELPERS
namespace deadzone_test {
std::array<uint8_t,0x800000> memory{};
constexpr uint32_t P=0x600000,Q=0x601000,D=0x610000,O=0x620000,S=0x7f0000;
UCRRControllerConfig profiles[2];
input::Pad samples[2];
unsigned port=0,pending=4;
uint32_t axes[2]{},packet=0;
int16_t raw[4]{};
uint32_t Word(uint32_t at){uint32_t value;std::memcpy(&value,memory.data()+at,4);return value;}
void Put(uint32_t at,uint32_t value){std::memcpy(memory.data()+at,&value,4);}
void Real(uint32_t at,float value){std::memcpy(memory.data()+at,&value,4);}
float Float(uint32_t at){float value;std::memcpy(&value,memory.data()+at,4);return value;}
}
namespace mem {bool IsCommitted(uint32_t page){return page<deadzone_test::memory.size();}}
namespace trial_controls {
void BeginControllerSample(unsigned port){deadzone_test::pending=port;}
void RegisterControllerAxes(uint32_t axes){using namespace deadzone_test;assert(pending<2);deadzone_test::axes[pending]=axes;pending=4;}
bool NativeControllerResponse(uint32_t source,float minimum,float gain,float& value){
    using namespace deadzone_test;
    for(unsigned player=0;player<2;++player)if(axes[player]&&source>=axes[player]&&source-axes[player]<16)
        return controller_axis_response(samples[player],profiles[player],(source-axes[player])/4,minimum,gain,value);
    return false;
}
}
#include "../common/xbe_controller_deadzone.h"
#include "cpu/x86_ops.h"
[[noreturn]] void x86_trap(X86Context&,uint32_t,const char*){std::abort();}
X86_FUNC(sub_0044A26A){
    using namespace deadzone_test;
    assert(pending==4);trial_controls::BeginControllerSample(port);
    const uint32_t at=Word(ctx.esp+8);Put(at,++packet);Put(at+4,0);Put(at+8,0);Put(at+12,0);
    for(unsigned axis=0;axis<4;++axis)std::memcpy(base+at+14+axis*2,&raw[axis],2);
    ctx.eax=0;ctx.esp+=12;
}
X86_FUNC(sub_0044A2DD){assert(false);}
X86_FUNC(sub_00232DF0){ctx.esp+=4;}
// ORIGINAL_CONTROLLER_BODIES
namespace deadzone_test {
void Seed(){
    memory.fill(0);packet=0;pending=4;axes[0]=axes[1]=0;
    for(auto& profile:profiles){profile={};for(unsigned action=0;action<24;++action)profile.binding[action][0]=action+1;}
    for(uint32_t device:{P,Q}){Put(device+0x74,device+0x100);Put(device+0x156,997);}
    // ORIGINAL_CONSTANTS
    assert(Float(0x4fd08c)==.4f&&Float(0x4fd090)==.85f);
}
void Poll(unsigned player,int16_t value){
    port=player;std::fill(std::begin(raw),std::end(raw),value);
    samples[player]={};samples[player].connected=true;
    samples[player].lx=samples[player].ly=samples[player].rx=samples[player].ry=value;
    X86Context ctx{};ctx.ecx=player?Q:P;ctx.esp=S-8;Put(S-4,0);ctx.fcw=0x37f;
    NativePad(ctx,memory.data());assert(ctx.esp==S&&axes[player]==(player?Q:P)+0x14);
    const float expected=value<0?value/32768.f:value/32767.f;
    for(unsigned axis=0;axis<4;++axis){
        ctx.ecx=player?Q:P;ctx.esp=S-8;Put(S-4,axis);NativeAxisAddress(ctx,memory.data());
        assert(ctx.esp==S&&ctx.eax==axes[player]+axis*4);
        assert(std::fabs(Float(ctx.eax)-expected)<1.e-7f);
    }
}
X86Context Evaluate(unsigned player,unsigned axis,unsigned flags,bool custom){
    Put(D,flags);Put(D+4,axes[player]+axis*4);Put(D+8,O);Put(D+12,O+4);
    Real(D+0x10,1.f/(Float(0x4fd090)-Float(0x4fd08c)));Real(D+0x14,Float(0x4fd08c));
    X86Context ctx{};ctx.ecx=D;ctx.esp=S-4;ctx.fcw=0x37f;
    ctx.ebx=0x12345678;ctx.esi=0x87654321;ctx.edi=0xabcdef01;ctx.ebp=0xfacebeef;
    if(custom)ConfiguredDescriptor(ctx,memory.data());else OriginalDescriptor(ctx,memory.data());
    assert(ctx.esp==S&&ctx.ebx==0x12345678&&ctx.esi==0x87654321&&ctx.edi==0xabcdef01&&ctx.ebp==0xfacebeef);
    return ctx;
}
void OriginalParity(){
    unsigned count=0;
    for(unsigned player=0;player<2;++player)for(int value:{-32768,-28000,-20000,-13107,-1,0,1,13107,20000,28000,32767}){
        Poll(player,int16_t(value));
        for(unsigned axis=0;axis<4;++axis)for(unsigned flags:{0x25u,0x27u,0x35u,0x3du,0x65u,0x125u}){
            Real(O,.1f);Put(O+4,0);const std::vector<uint8_t> before(memory.begin(),memory.end());const auto csr=_mm_getcsr();
            const auto original=Evaluate(player,axis,flags,false);const std::vector<uint8_t> expected(memory.begin(),memory.end());const auto after_csr=_mm_getcsr();
            std::memcpy(memory.data(),before.data(),before.size());_mm_setcsr(csr);const auto configured=Evaluate(player,axis,flags,true);
            assert(!std::memcmp(&original,&configured,sizeof original));
            assert(!std::memcmp(memory.data(),expected.data(),memory.size())&&_mm_getcsr()==after_csr);++count;
        }
    }
    std::printf("Original controller curve: %u complete context/RAM comparisons passed\n",count);
}
void Configured(){
    for(unsigned player=0;player<2;++player)for(unsigned stick=0;stick<2;++stick)
    for(unsigned percent:{0u,1u,10u,20u,30u})for(int value:{-32768,-28000,-20000,-10000,-1,0,1,10000,20000,28000,32767}){
        for(auto& profile:profiles)for(auto& v:profile.stick_deadzone)v=31;
        profiles[player].stick_deadzone[stick]=percent;Poll(0,int16_t(value));Poll(1,int16_t(value));
        for(unsigned sampled=0;sampled<2;++sampled)for(unsigned axis=0;axis<4;++axis){
            const float input=Float(axes[sampled]+axis*4),threshold=(sampled==player&&axis/2==stick)?percent/100.f:.4f;
            const float expected=std::fabs(input)<=threshold?0.f:std::copysign((std::fabs(input)-threshold)/(.85f-threshold),input);
            Evaluate(sampled,axis,0x25,true);assert(std::fabs(Float(O)-expected)<4.e-7f);
            Evaluate(sampled,axis,0x35,true);assert(std::fabs(Float(O)-std::clamp(expected,-1.f,1.f))<4.e-7f);
        }
    }
    profiles[0].stick_deadzone[1]=0;Poll(0,1);Evaluate(0,2,0x25,true);assert(Float(O)>0);Evaluate(0,2,0x25,false);assert(Float(O)==0);
    profiles[0].stick_deadzone[1]=20;
    for(int value:{-6553,0,6553}){Poll(0,int16_t(value));Evaluate(0,2,0x25,true);assert(Float(O)==0);}
    for(int value:{-27853,27852}){Poll(0,int16_t(value));Evaluate(0,2,0x25,true);assert(std::fabs(std::fabs(Float(O))-1.f)<.0001f);}
    // Non-axis descriptors keep their original edge and button conversion.
    Poll(0,10000);for(unsigned flags:{0x65u,0x125u,0x225u}){
        Real(O,0);Put(O+4,0);Evaluate(0,2,flags,false);const auto expected=Word(O);
        Real(O,0);Put(O+4,0);Evaluate(0,2,flags,true);assert(Word(O)==expected);
    }
    // Physical-stick settings follow crossed bindings, including a mixed
    // secondary source. A button-only axis keeps its original descriptor.
    profiles[0].stick_deadzone[0]=0;profiles[0].stick_deadzone[1]=30;
    profiles[0].binding[20][0]=17;profiles[0].binding[21][0]=18;
    profiles[0].binding[16][0]=21;profiles[0].binding[17][0]=22;
    Poll(0,1000);Evaluate(0,2,0x25,true);assert(Float(O)>.03f);
    Evaluate(0,2,0x27,true);assert(Float(O)<-.03f);
    Evaluate(0,0,0x25,true);assert(Float(O)==0);
    profiles[0].binding[21][1]=22;Evaluate(0,2,0x25,true);assert(Float(O)>.03f);
    profiles[0].binding[20][0]=1;profiles[0].binding[21][0]=2;profiles[0].binding[21][1]=0;
    Evaluate(0,2,0x25,false);const auto button_axis=Word(O);
    Evaluate(0,2,0x25,true);assert(Word(O)==button_axis);
    // A legacy host filter may suppress the normalizer's axis. Controller-owned
    // numeric zero still uses the physical pre-remap sample, exactly once.
    profiles[0].binding[16][0]=17;profiles[0].binding[17][0]=18;
    profiles[0].deadzone[0]=20;profiles[0].stick_deadzone[0]=0;
    Poll(0,1000);Real(axes[0],0);
    Evaluate(0,0,0x25,false);assert(Float(O)==0);
    Evaluate(0,0,0x25,true);assert(Float(O)>.03f);
    puts("Controller deadzones: native normalization, zero response, thresholds, endpoints and per-player/stick isolation passed");
}
}
int main(){deadzone_test::Seed();deadzone_test::OriginalParity();deadzone_test::Configured();}
