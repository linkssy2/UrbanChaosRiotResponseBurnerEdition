// Run the pinned retail pulse lifecycle, descriptor setters/getters, render
// queue constructor and 97B50 consumer. GPU filtering is covered separately.
#undef NDEBUG
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
#include "kernel/kernel.h"
namespace kernel {alignas(16) static std::array<uint8_t,0x800000> memory{};}
namespace mem {bool IsCommitted(uint32_t page){return page<kernel::memory.size();}}
#include "xbe_coop_aim_blur.cpp"
namespace aim_test {
constexpr uint32_t world=0x620000,state=0x624000,layers=0x630000;
constexpr uint32_t owner0=0x610000,owner1=0x612000,layer0=0x631000,layer1=0x632000;
constexpr uint32_t manager=0x633000,list=0x633100,sentinel=0x633200,heap=0x634000;
constexpr uint32_t graphics=0x640000,table=0x641000,service=0x642000;
constexpr uint32_t payload=0x643000,descriptor=payload+8,descriptor_table=0x644000;
constexpr uint32_t target_service=0x645000,target=0x646000,projection=0x647000,matrix=0x648000;
constexpr uint32_t camera0=0x650000,camera1=0x651000,view0=0x652000,view1=0x653000;
constexpr uint32_t queue0=0x654000,queue1=0x655000,params=0x656000;
constexpr uint32_t arg=0x657000,arg1=0x658000,binding=0x659000,blur=0x65a000,stack=0x7f0000;
unsigned presentation=1,next_node=0;
bool throw_render=false;
std::vector<uint32_t> freed,recycled;
struct Call {int slot;float start,end,amplitude;};
std::vector<Call> calls;
uint8_t* Bytes(){return kernel::memory.data();}
uint32_t Word(uint32_t a){uint32_t v;std::memcpy(&v,Bytes()+a,4);return v;}
float Real(uint32_t a){float v;std::memcpy(&v,Bytes()+a,4);return v;}
void Put(uint32_t a,uint32_t v){std::memcpy(Bytes()+a,&v,4);}
void Float(uint32_t a,float v){std::memcpy(Bytes()+a,&v,4);}
X86Context Context(){
    X86Context c{};c.eax=0x1234;c.ecx=0x2345;c.edx=0x3456;c.ebx=0x4567;
    c.esi=0x5678;c.edi=0x6789;c.ebp=0x789a;c.esp=stack;c.fs_base=0x7e0000;
    c.cf=1;c.zf=1;c.sf=1;c.of=1;c.pf=1;c.df=0;c.xflags=0x200000;
    c.fcw=0x37f;c.fsw=0x1234;c.mxcsr=0x1f80;c.top=2;
    for(unsigned i=0;i<8;++i){c.st[i]=double(i)+.125;c.mm[i]=0x1234567800000000ull+i;c.xmm[i].q[0]=i+1;c.xmm[i].q[1]=i+9;}
    return c;
}
void Setup(){
    trial_coop::Reset();auto c=Context();TrialCoopAimBlurBeginUpdate(c,Bytes());
    kernel::memory.fill(0);trial_coop::SetAvailable(true);assert(trial_coop::Request());trial_coop::SetActive(true);
    presentation=1;next_node=0;calls.clear();freed.clear();recycled.clear();throw_render=false;
    Put(0x51c4dc,world);Put(world,0x450c48);Put(world+4,state);Put(world+0x10,0x80);Put(world+0x934,manager);
    Put(state+4,layers);Put(state+8,layers+8);
    for(unsigned i=0;i<2;++i){
        const uint32_t o=i?owner1:owner0,l=i?layer1:layer0,v=i?view1:view0,cam=i?camera1:camera0;
        Put(o,0x46fbe8);Put(o+0x54,349+i);Put(o+0x368,i);
        Put(layers+i*4,l);Put(l+4,o);Put(l+0x20,v);Put(l+0x24,i);
        Put(cam,0x480a6c);Put(cam+4,v);Put(cam+0x128,0xffffffef);
    }
    Put(0x480a6c+8,0xf1080);Float(manager+4,1e30f);Float(manager+8,1e30f);Float(0x44c724,1e30f);
    Put(manager+12,list);Put(list+4,sentinel);Put(sentinel,sentinel);Put(sentinel+4,sentinel);
    Put(0x5150b0,graphics);Put(0x519f60,graphics);Put(graphics,table);
    Put(table+0x60,0xf1060);Put(table+0xa4,0xf10a4);Put(table+0xdc,0xf10dc);Put(table+0x50,0xf1050);
    Put(service,table+0x100);Put(table+0x11c,0xf101c);Put(target_service+0x10,target);
    Put(descriptor,descriptor_table);
    for(const auto& p:std::array<std::array<uint32_t,2>,6>{{{0x48,0x94760},{0x4c,0x94770},{0x50,0x947c0},{0x54,0x94780},{0x58,0x947a0},{0x5c,0x947b0}}})Put(descriptor_table+p[0],p[1]);
    Put(projection,table+0x200);Put(table+0x210,0xf1010);
    for(unsigned i=0;i<16;++i)Float(matrix+i*4,float(i)+.25f);
    Put(arg+4,arg1);Put(arg1+12,binding);Put(binding,table+0x300);Put(table+0x320,0xf1020);
    Put(0x519f18,blur);
}
}
uint32_t TrialCoopPlayer(unsigned slot){return slot==0?aim_test::owner0:slot==1?aim_test::owner1:0;}
unsigned TrialCoopPresentation(){return aim_test::presentation;}
[[noreturn]] void x86_trap(X86Context&,uint32_t,const char*){std::abort();}
void x86_resume(X86Context&,uint8_t*,const X86Resume&){std::abort();}
X86_FUNC(sub_00049180);X86_FUNC(sub_00094760);X86_FUNC(sub_00094770);
X86_FUNC(sub_00094780);X86_FUNC(sub_000947A0);X86_FUNC(sub_000947B0);X86_FUNC(sub_000947C0);
void x86_icall(X86Context& c,uint8_t* b,uint32_t t){
    using namespace aim_test;
    switch(t){
    case 0x94760:sub_00094760(c,b);return;case 0x94770:sub_00094770(c,b);return;
    case 0x94780:sub_00094780(c,b);return;case 0x947a0:sub_000947A0(c,b);return;
    case 0x947b0:sub_000947B0(c,b);return;case 0x947c0:sub_000947C0(c,b);return;
    case 0xf1060:assert(c.ecx==graphics);c.eax=descriptor;break;
    case 0xf10a4:assert(c.ecx==graphics);c.eax=service;break;
    case 0xf101c:assert(c.ecx==service);c.eax=target_service;break;
    case 0xf1080:assert(c.ecx==camera0||c.ecx==camera1);c.eax=c.ecx+0x128;c.esp+=4;break;
    case 0xf1020:assert(c.ecx==binding);break;
    case 0xf10dc:assert(c.ecx==graphics&&Word(c.esp+4)==0);c.eax=payload;c.esp+=4;break;
    case 0xf1050:assert(c.ecx==graphics);c.eax=projection;break;
    case 0xf1010:assert(c.ecx==projection);c.eax=matrix;break;
    default:std::printf("unexpected virtual %08x\n",t);std::abort();
    }
    c.esp+=4;
}
X86_FUNC(sub_002D57FB){
    using namespace aim_test;assert(Word(ctx.esp+4)==36);
    if(recycled.empty())ctx.eax=heap+64*next_node++;
    else{ctx.eax=recycled.back();recycled.pop_back();}
    ctx.esp+=4;
}
X86_FUNC(sub_002D5CCD){
    using namespace aim_test;freed.push_back(Word(ctx.esp+4));recycled.push_back(freed.back());ctx.esp+=4;
}
X86_FUNC(sub_000491B0){
    using namespace aim_test;assert(ctx.ecx==list);Put(list+8,Word(list+8)+Word(ctx.esp+4));ctx.esp+=8;
}
X86_FUNC(sub_002012C0){
    using namespace aim_test;assert(ctx.ecx==list);
    for(auto n=Word(sentinel);n!=sentinel;n=Word(n)){freed.push_back(n);recycled.push_back(n);}
    Put(sentinel,sentinel);Put(sentinel+4,sentinel);Put(list+8,0);ctx.esp+=4;
}
X86_FUNC(sub_00095070){assert(ctx.ecx==aim_test::payload);ctx.eax=aim_test::payload;ctx.esp+=4;}
X86_FUNC(sub_000920B0){
    using namespace aim_test;assert(ctx.ecx==blur&&Word(ctx.esp+4)==target);
    assert(!std::memcmp(Bytes()+Word(ctx.esp+16),Bytes()+matrix,64));
    calls.push_back({TrialCoopAimBlurActiveView(),Real(ctx.esp+8),Real(ctx.esp+12),Real(ctx.esp+20)});
    if(throw_render)throw 23;ctx.esp+=24;
}
// PRODUCTION_AIM_BLUR_BODIES
namespace aim_test {
using Native=void(*)(X86Context&,uint8_t*);
uint32_t Pulse(unsigned owner,float amp=.8f,float hold=.2f,uint32_t caller=0xbef59,Native fn=sub_000490D0){
    auto c=Context();c.ecx=manager;c.esi=owner?owner1:owner0;Put(stack,caller);
    const float args[]{-150.f,600.f,amp,hold,15.f,5.f};std::memcpy(Bytes()+stack+4,args,sizeof args);
    fn(c,Bytes());assert(c.esp==stack+28);return Word(sentinel+4);
}
X86Context Update(float dt=.1f,Native fn=sub_00048E80){
    auto c=Context();c.ecx=manager;Float(stack+4,dt);fn(c,Bytes());assert(c.esp==stack+8);return c;
}
void Direct(float amp=.25f){auto c=Context();c.ecx=manager;Float(stack+4,-400);Float(stack+8,800);Float(stack+12,amp);sub_00048E10(c,Bytes());assert(c.esp==stack+16);}
void Clear(){auto c=Context();c.ecx=manager;sub_00048E60(c,Bytes());}
void Render(unsigned slot){
    const uint32_t q=slot?queue1:queue0,cam=slot?camera1:camera0;
    auto c=Context();c.ecx=q;Put(stack+4,params);Put(params+12,cam);
    sub_00097050(c,Bytes());assert(c.esp==stack+8&&Word(q)==0x456984&&Word(q+0x18)==cam&&Word(q+0x20)==target);
    std::array<uint8_t,0x64> before{};std::memcpy(before.data(),Bytes()+descriptor,before.size());
    c=Context();c.ecx=q;Put(stack+4,arg);sub_00097B50(c,Bytes());assert(c.esp==stack+8);
    assert(!std::memcmp(before.data(),Bytes()+descriptor,before.size()));assert(TrialCoopAimBlurActiveView()==-1);
}
void CheckOwnership(){
    for(unsigned owner=0;owner<2;++owner)for(unsigned order=0;order<2;++order){
        Setup();Pulse(owner);Update();assert(Bytes()[descriptor+0x55]&&Real(descriptor+0x60)==.8f);
        Render(order);Render(1-order);assert(calls.size()==1&&calls[0].slot==int(owner)&&calls[0].amplitude==.8f);
    }
    for(unsigned order=0;order<2;++order){
        Setup();Pulse(0,.4f);Pulse(1,.8f,1,0xbef11);Update();
        Render(order);Render(1-order);assert(calls.size()==2);
        for(const auto& c:calls)assert(c.amplitude==(c.slot?.8f:.4f));
    }
    Setup();Pulse(1);Direct();Update();Render(0);Render(1);
    assert(calls.size()==2&&calls[0].amplitude==.25f&&calls[1].amplitude==.8f);
    assert(calls[0].start==-400&&calls[0].end==800&&calls[1].start==-400&&calls[1].end==600);
    Setup();Pulse(1,.6f,1,0xcd9d1);Update();Render(0);Render(1);
    assert(calls.size()==2&&calls[0].amplitude==.6f&&calls[1].amplitude==.6f);
    std::puts("aim blur: native queue/consumer isolates either owner, both render orders and concurrent pulses; direct/pain remain shared");
}
void CheckTransitions(){
    Setup();presentation=2;Pulse(1);Update();presentation=1;Render(0);Render(1);
    assert(calls.size()==1&&calls[0].slot==1);
    Setup();Pulse(1);Update();presentation=2;Render(0);Render(1);
    assert(calls.size()==2&&calls[0].slot==-1&&calls[1].slot==-1&&calls[0].amplitude==.8f);
    Setup();Pulse(1);Update();Put(world+0x10,0x880);Render(0);Render(1);
    assert(calls.size()==2&&calls[0].slot==-1&&calls[1].slot==-1);
    Setup();Pulse(1,.8f,2);Update();presentation=2;Render(0);Update();presentation=1;Render(0);Render(1);
    assert(calls.size()==2&&calls[0].slot==-1&&calls[1].slot==1);
    Setup();Pulse(1);Update();Put(layer1+0x20,view1+0x100);Put(camera1+4,view1+0x100);
    Render(0);Render(1);assert(calls.size()==1&&calls[0].slot==1);
    Setup();trial_coop::SetActive(false);Pulse(1);Update();Render(0);Render(1);
    assert(calls.size()==2&&calls[0].slot==-1&&calls[1].slot==-1);
    std::puts("aim blur: presentation changes between update and render preserve cinematic/SP aggregate and split ownership");
}
void CheckLifecycle(){
    Setup();const auto node=Pulse(1);Update();Clear();assert(Word(list+8)==0&&freed.size()==1);
    assert(Pulse(0,.6f,1,0xcd9d1)==node);Update();Render(0);Render(1);assert(calls.size()==2);
    Setup();const auto expired=Pulse(1,.8f,.01f);
    for(unsigned i=0;i<20;++i)Update();assert(Word(list+8)==0&&freed==std::vector<uint32_t>{expired});
    Render(0);Render(1);assert(calls.empty());
    assert(Pulse(0,.7f,1)==expired);Update();Render(1);Render(0);assert(calls.size()==1&&calls[0].slot==0);
    Setup();Pulse(1);Update();Put(owner1+0x54,999);Render(0);Render(1);
    assert(calls.size()==2&&calls[0].amplitude==.8f&&calls[1].amplitude==.8f);
    Setup();Pulse(1);Update();Put(world+0x934,manager+0x80);Render(0);Render(1);assert(calls.size()==2);
    std::puts("aim blur: expiry, clear, node reuse, owner-id reuse and world-manager replacement do not retain stale ownership");
}
void CheckNativeParity(){
    // Observers must not change native storage, pulse timing, publication, flags
    // or allocation/deletion. Compare every guest byte and the complete CPU.
    const unsigned original_csr=_mm_getcsr();_mm_setcsr(0x5f80);
    Setup();Pulse(1,.8f,.2f,0xbef59,OriginalPulse);Direct();
    std::array<X86Context,16> expected{};std::array<unsigned,16> expected_csr{};std::vector<std::array<uint8_t,128>> nodes;
    for(unsigned i=0;i<expected.size();++i){expected[i]=Update(.1f,OriginalUpdate);expected_csr[i]=_mm_getcsr();std::array<uint8_t,128> n{};std::memcpy(n.data(),Bytes()+heap,128);nodes.push_back(n);}
    const std::vector<uint8_t> original_memory(kernel::memory.begin(),kernel::memory.end());const auto original_frees=freed;
    _mm_setcsr(0x5f80);Setup();Pulse(1);Direct();
    for(unsigned i=0;i<expected.size();++i){auto c=Update();assert(!std::memcmp(&c,&expected[i],sizeof c));assert(_mm_getcsr()==expected_csr[i]);assert(!std::memcmp(nodes[i].data(),Bytes()+heap,128));}
    assert(std::equal(original_memory.begin(),original_memory.end(),kernel::memory.begin())&&freed==original_frees);
    _mm_setcsr(original_csr);
    std::puts("aim blur: 16 retail lifecycle updates match original CPU/MXCSR, all guest bytes and deallocation exactly");
}
void CheckIsolation(){
    Setup();Pulse(0,.4f);Pulse(1,.8f);Update();Render(0);auto c=Context();c.ecx=queue0;c.eax=payload;
    const auto before=c;const auto csr=_mm_getcsr();_mm_setcsr(0x5f80);
    const std::vector<uint8_t> native_memory(kernel::memory.begin(),kernel::memory.end());
    {TrialCoopAimBlurViewScope scope(c,Bytes());assert(TrialCoopAimBlurActiveView()==0);scope.Apply(c);assert(Real(descriptor+0x60)==.4f);assert(!std::memcmp(&c,&before,sizeof c));assert(_mm_getcsr()==0x5f80);}
    assert(std::equal(native_memory.begin(),native_memory.end(),kernel::memory.begin())&&TrialCoopAimBlurActiveView()==-1&&_mm_getcsr()==0x5f80);_mm_setcsr(csr);
    throw_render=true;try{Render(0);assert(false);}catch(int n){assert(n==23);}
    assert(TrialCoopAimBlurActiveView()==-1&&Bytes()[descriptor+0x55]&&Real(descriptor+0x60)==.8f);
    std::puts("aim blur: descriptor and scope restore on normal/exception exit; observer CPU/MXCSR preserved (no GPU)");
}
}
int main(){aim_test::CheckOwnership();aim_test::CheckTransitions();aim_test::CheckLifecycle();aim_test::CheckNativeParity();aim_test::CheckIsolation();}
