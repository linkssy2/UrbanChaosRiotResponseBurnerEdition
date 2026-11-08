// Production writers and original/candidate queue replay; no GPU or guest resource headers.
#undef NDEBUG
#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <tuple>
#include <vector>
#include <smmintrin.h>
#include "gpu/state.h"
using namespace gpu;
namespace baseline { /* BASELINE_TRANSPORT */ }
namespace candidate { /* CANDIDATE_TRANSPORT */ }
namespace xdk { constexpr uint32_t kUnknown = ~0u; }
struct WriterMemory {
    State g_state{};
    std::array<uint32_t, 2048> memory{};
    bool g_own_constants = false;
    static constexpr uint32_t kDevice = 4096;
    uint32_t XDK_CDevice_Viewport = 128;
    WriterMemory() {
        for(unsigned i=0;i<memory.size();++i)memory[i]=0x3f000000u+i;
        g_state.TrackSimple();
    }
    void* HostPtr(uint32_t address) { assert(address%4==0 && address/4<memory.size()); return memory.data()+address/4; }
    void W32(uint32_t address,uint32_t value) { *static_cast<uint32_t*>(HostPtr(address))=value; }
};
struct OriginalWriters:WriterMemory { /* BASELINE_WRITERS */ };
struct TrackedWriters:WriterMemory {
    TrackedWriters() { g_state.TrackVertexConstants(); }
    /* CANDIDATE_WRITERS */
};
static const auto ranges = baseline::StateRanges();
static unsigned checks=0;
static volatile uint64_t sink=0;
static void StateEqual(const State& a,const State& b) {
    assert(a.simple==b.simple);
    for(auto r:ranges)assert(!std::memcmp(reinterpret_cast<const uint8_t*>(&a)+r.off,
        reinterpret_cast<const uint8_t*>(&b)+r.off,r.len));
    assert(a.textures==b.textures&&a.palettes==b.palettes&&a.vs==b.vs&&a.ps==b.ps);
    assert(!std::memcmp(a.streams.data(),b.streams.data(),sizeof(a.streams)));
    assert(!std::memcmp(&a.rt,&b.rt,sizeof(a.rt))&&!std::memcmp(&a.zs,&b.zs,sizeof(a.zs)));
}
static void BatchEqual(const baseline::Replay& a,const candidate::Replay& b) {
    assert(a.cur_->bytes==b.cur_->bytes&&a.cur_->vs==b.cur_->vs&&a.cur_->ps==b.cur_->ps);
    assert(a.cur_->cmds.size()==b.cur_->cmds.size());
    for(size_t i=0;i<a.cur_->cmds.size();++i) {
        const auto& x=a.cur_->cmds[i];const auto& y=b.cur_->cmds[i];
        assert(unsigned(x.op)==unsigned(y.op));
        assert(std::tie(x.a,x.b,x.c,x.d,x.data,x.size,x.vs,x.ps)==std::tie(y.a,y.b,y.c,y.d,y.data,y.size,y.vs,y.ps));
    }
    StateEqual(a.sent_,b.sent_);++checks;
}
struct Check {
    baseline::Replay a; candidate::Replay b;
    void Drain() {
        a.Drain();b.Drain();assert(a.draws.size()==b.draws.size());
        for(size_t i=0;i<a.draws.size();++i)StateEqual(a.draws[i],b.draws[i]);
        assert(a.vsc_revision_==b.vsc_revision_);
        a.draws.clear();b.draws.clear();
    }
    void Queue(const State& x,const State& y) {
        StateEqual(x,y);a.Queue(x);b.Queue(y);BatchEqual(a,b);
        if(a.cur_->cmds.size()>64)Drain();
    }
};
static void WritersAndReplay() {
    OriginalWriters x;TrackedWriters y;Check q;
    auto queue=[&] { assert(x.memory==y.memory);q.Queue(x.g_state,y.g_state); };
    auto call=[&](unsigned kind,unsigned reg,unsigned count) {
        uint32_t args[]={reg,128,count*4};
        switch(kind) {
            case 0:x.D3DDevice_SetVertexShaderConstant1Fast(args);y.D3DDevice_SetVertexShaderConstant1Fast(args);break;
            case 1:x.D3DDevice_SetVertexShaderConstant1(args);y.D3DDevice_SetVertexShaderConstant1(args);break;
            case 2:x.D3DDevice_SetVertexShaderConstant4(args);y.D3DDevice_SetVertexShaderConstant4(args);break;
            default:x.D3DDevice_SetVertexShaderConstantNotInlineFast(args);y.D3DDevice_SetVertexShaderConstantNotInlineFast(args);break;
        }
        queue();
    };
    queue();queue();
    for(unsigned reg=0;reg<192;++reg)for(unsigned word=0;word<4;++word) {
        for(uint32_t bits:{0u,0x80000000u,0x7fc00011u,0x7fc00012u,0x7f800000u,0xff800000u,0x3f800000u}) {
            x.memory[32+word]=y.memory[32+word]=bits;call(word%2,reg,1);call(word%2,reg,1);
        }
    }
    for(unsigned reg:{0u,1u,3u,4u,57u,58u,59u,60u,188u,189u,191u,192u,~0u}) {
        call(2,reg,4);
        for(unsigned n:{0u,1u,3u,4u,5u,12u,32u,64u,192u,193u})call(3,reg,n);
    }
    // Original integer truncation of a dword count must remain intact.
    for(unsigned n=0;n<8;++n){uint32_t args[]={191,128,n};
        x.D3DDevice_SetVertexShaderConstantNotInlineFast(args);y.D3DDevice_SetVertexShaderConstantNotInlineFast(args);queue();}
    for(bool own:{false,true,false})for(uint32_t format:{0u,0x2cu,0x30u,0x2eu}) {
        x.g_own_constants=y.g_own_constants=own;
        x.g_state.zs=y.g_state.zs=Surface{7,0,format<<8};
        x.g_state.viewport=y.g_state.viewport=Viewport{3,5,1280,360,.25f,.875f};
        x.g_state.ss_offset_x=y.g_state.ss_offset_x=.5f;x.g_state.ss_offset_y=y.g_state.ss_offset_y=-.5f;
        x.UpdateViewportConstants();y.UpdateViewportConstants();queue();
    }
    decltype(VertexShader::constants) declared;
    for(unsigned reg:{0u,3u,58u,59u,191u,192u,~0u,59u,0u})declared.emplace_back(reg,std::array<float,4>{1,2,3,float(declared.size())});
    x.Shader(declared);y.Shader(declared);queue();x.Shader(declared);y.Shader(declared);queue();
    auto saved=x.g_state.vsc[17];
    x.g_state.vsc[17]=y.g_state.vsc[17]={9,8,7,6};y.g_state.MarkVertexConstants(17,1);
    x.g_state.vsc[17]=y.g_state.vsc[17]=saved;y.g_state.MarkVertexConstants(17,1);queue();
    // Remaining ranges and shared simple tracking still compare exactly as before.
    for(auto r:ranges)for(size_t i=0;i<r.len;++i) {
        size_t off=r.off+i,first=baseline::At(x.g_state,&x.g_state.vsc);
        if(off>=first&&off<first+sizeof(x.g_state.vsc))continue;
        auto* a=reinterpret_cast<uint8_t*>(&x.g_state)+off;auto* b=reinterpret_cast<uint8_t*>(&y.g_state)+off;
        *a^=0x81;*b^=0x81;queue();*a^=0x81;*b^=0x81;queue();
    }
    x.g_state.SetSimple(1025,45);y.g_state.SetSimple(1025,45);queue();
    // Clear and BlurScene call the identical SendState before publishing their own command.
    for(unsigned op=0;op<2;++op){x.g_state.vsc[3][0]=y.g_state.vsc[3][0]=float(op+17);y.g_state.MarkVertexConstants(3,1);
        q.a.SendState(x.g_state);q.b.SendState(y.g_state);BatchEqual(q.a,q.b);queue();}
    q.Drain();q.a.vsc_revision_=q.b.vsc_revision_=UINT64_MAX;
    x.g_state.vsc[0][0]=y.g_state.vsc[0][0]=77;y.g_state.MarkVertexConstants(0,1);queue();q.Drain();
    assert(q.a.vsc_revision_==0&&q.b.vsc_revision_==0);
    assert(y.g_state.vsc_dirty==0);
}
static void FallbacksAndLifetime() {
    Check q;State a{},b{},other_a{},other_b{},raw_a{},raw_b{};b.TrackVertexConstants();other_b.TrackVertexConstants();
    q.Queue(a,b);q.Queue(a,b);
    a.vsc[190][3]=b.vsc[190][3]=9;b.MarkVertexConstants(190,1);q.Queue(a,b);
    other_a.vsc[7][1]=other_b.vsc[7][1]=11;other_b.MarkVertexConstants(7,1);q.Queue(other_a,other_b);
    q.Queue(a,b);q.Queue(other_a,other_b);
    raw_a.vsc[191][3]=raw_b.vsc[191][3]=13;q.Queue(raw_a,raw_b);
    raw_a.vsc[191][3]=raw_b.vsc[191][3]=14;q.Queue(raw_a,raw_b);q.Queue(a,b);
    a=State{};b=State{};b.TrackVertexConstants();q.Queue(a,b);
    a.vs=b.vs=std::make_shared<VertexShader>();a.ps=b.ps=std::make_shared<PixelShader>();
    a.vsc[88][2]=b.vsc[88][2]=19;b.MarkVertexConstants(88,1);q.Queue(a,b);
    a.vsc[88][2]=b.vsc[88][2]=21;b.MarkVertexConstants(88,1);a.vs.reset();b.vs.reset();a.ps.reset();b.ps.reset();
    q.Drain();q.Queue(a,b);q.Drain();
    for(auto r:ranges)assert(r.off>=baseline::At(a,&a.simple));
    // Negative witness proves an omitted writer would be caught, rather than passing by fallback.
    candidate::Replay broken;State omitted{};omitted.TrackVertexConstants();broken.Queue(omitted);broken.Drain();
    omitted.vsc[31][1]=3;broken.Queue(omitted);broken.Drain();assert(broken.draws.back().vsc[31][1]!=3);
    omitted.vsc_tracked=false;broken.Queue(omitted);broken.Drain();assert(broken.draws.back().vsc[31][1]==3);
}
static void Ranges() {
    static_assert(sizeof(State::vsc)==48*64);
    for(unsigned first=0;first<=192;++first)for(unsigned n:{0u,1u,3u,4u,5u,12u,32u,64u,191u,192u,~0u}) {
        State s{};s.vsc_tracked=true;s.MarkVertexConstants(first,n);
        const unsigned count=std::min(n,192-first);
        for(unsigned block=0;block<64;++block)assert(bool(s.vsc_dirty&(uint64_t(1)<<block))==
            (block<48&&count&&block*4<first+count&&(block+1)*4>first));
    }
    State s{};s.MarkVertexConstants(0,192);assert(!s.vsc_dirty);
    s.TrackVertexConstants();s.vsc_dirty=0;s.MarkVertexConstants(~0u,~0u);assert(!s.vsc_dirty);
}
template<class Writer,class Transport> static double Bench(unsigned registers,unsigned iterations,bool all_changed=false,bool shader=false) {
    Writer writer;Transport q;auto& s=writer.g_state;
    decltype(VertexShader::constants) declared;
    for(unsigned i=0;i<registers;++i)declared.emplace_back(i,std::array<float,4>{1,2,3,4});
    q.Queue(s);q.Discard();q.cur_->bytes.reserve(16384);q.cur_->cmds.reserve(8);
    const auto start=std::chrono::steady_clock::now();
    for(unsigned i=0;i<iterations;++i) {
        if(all_changed)for(unsigned j=0;j<registers*4;++j)writer.memory[32+j]=0x3f000000u+i+j;
        else writer.memory[32]=0x3f000000u+i;
        uint32_t args[]={0,128,registers*4};
        if(shader){for(auto& [reg,value]:declared)value[0]=std::bit_cast<float>(0x3f000000u+i+reg);writer.Shader(declared);}
        else if(registers==1)writer.D3DDevice_SetVertexShaderConstant1Fast(args);
        else if(registers==4)writer.D3DDevice_SetVertexShaderConstant4(args);
        else if(registers)writer.D3DDevice_SetVertexShaderConstantNotInlineFast(args);
        q.Queue(s);sink=sink+q.cur_->bytes.size()+q.cur_->cmds.size();q.Discard();
    }
    return std::chrono::duration<double,std::nano>(std::chrono::steady_clock::now()-start).count()/iterations;
}
static void Benchmark() {
    for(unsigned mode=0;mode<3;++mode)for(unsigned n:{0u,1u,4u,12u,32u,64u,192u}) {
        const bool all=mode==1,shader=mode==2;
        if(shader&&n!=4&&n!=32&&n!=192)continue;
        Bench<OriginalWriters,baseline::Replay>(n,2000,all,shader);Bench<TrackedWriters,candidate::Replay>(n,2000,all,shader);
        std::array<double,9>a{},b{};
        for(unsigned r=0;r<9;++r) {
            if(r%2){b[r]=Bench<TrackedWriters,candidate::Replay>(n,40000,all,shader);a[r]=Bench<OriginalWriters,baseline::Replay>(n,40000,all,shader);}
            else {a[r]=Bench<OriginalWriters,baseline::Replay>(n,40000,all,shader);b[r]=Bench<TrackedWriters,candidate::Replay>(n,40000,all,shader);}
        }
        std::sort(a.begin(),a.end());std::sort(b.begin(),b.end());
        std::printf("pattern=%s registers=%u original_ns=%.3f candidate_ns=%.3f change_pct=%+.2f\n",shader?"shader-declared":all?"all-changing":"first-changing",n,a[4],b[4],100*(b[4]/a[4]-1));
    }
}
int main(int argc,char** argv) {
    Ranges();WritersAndReplay();FallbacksAndLifetime();
    std::printf("vertex constants: %u exact command/replay checks passed\n",checks);
    if(argc>1&&std::strcmp(argv[1],"--bench")==0)Benchmark();
}
