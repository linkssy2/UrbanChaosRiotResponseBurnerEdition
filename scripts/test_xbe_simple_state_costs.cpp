// Actual producer delta and replay bodies; only guest-header dereferencing is stubbed.
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
namespace mapping { /* PRODUCTION_PIXEL_MAPPING */ }
namespace baseline_mapping { /* BASELINE_PIXEL_MAPPING */ }
template<bool tracked> static void LoadPixelShader(State& s, const uint32_t* d) {
    auto put=[&](uint32_t method,uint32_t value) {
        if constexpr(tracked)s.SetSimple(method >> 2,value);else s.simple[method >> 2]=value;
    };
/* PRODUCTION_SHADER_WRITES */
}
// Production EndPush loop; only guest memory, draw dispatch and visibility commands are witnessed.
template<unsigned tracked,class Transport> struct Packet {
    State g_state{};Transport queue;std::vector<uint32_t> memory{0};
    std::vector<std::vector<uint32_t>> vertices_drawn;
    std::vector<std::pair<uint32_t,uint32_t>> reports;
    uint32_t g_push_begin=4;uint32_t a[1]{};
    Packet(){if constexpr(tracked)g_state.TrackSimple();}
    uint32_t R32(uint32_t address)const{assert(address%4==0&&address/4<memory.size());return memory[address/4];}
    const void* HostPtr(uint32_t address)const{assert(address%4==0&&address/4<memory.size());return memory.data()+address/4;}
    static uint32_t VertexBytes(){return 4;}
    template<class... Args>static void LOG_FIRST(Args... ){}
    void ApplyVisibilityMethod(uint32_t m,uint32_t v){
        if(m==kMCountEnable){if constexpr(tracked)g_state.SetSimple(m>>2,v);else g_state.simple[m>>2]=v;}
        if(m==kMClearReport||m==kMGetReport)reports.emplace_back(m,v);
    }
    void DrawUp(uint32_t,unsigned count,const void* vertices,uint32_t){
        queue.Queue(g_state);const auto* first=static_cast<const uint32_t*>(vertices);
        vertices_drawn.emplace_back(first,first+count);
    }
    void Load(const std::vector<uint32_t>& words){memory.assign(1,0);memory.insert(memory.end(),words.begin(),words.end());a[0]=uint32_t(memory.size()*4);}
    void End(){if constexpr(tracked==2){
/* CANDIDATE_PACKET */
    }else if constexpr(tracked==1){
/* PER_VALUE_PACKET */
    }else{
/* BASELINE_PACKET */
    }}
};

static const auto ranges = baseline::StateRanges();
static unsigned checks = 0;
static volatile uint64_t sink = 0;
static void StateEqual(const State& a, const State& b) {
    for (auto r : ranges)
        assert(!std::memcmp(reinterpret_cast<const uint8_t*>(&a)+r.off,
                            reinterpret_cast<const uint8_t*>(&b)+r.off,r.len));
    assert(a.textures==b.textures && a.palettes==b.palettes && a.vs==b.vs && a.ps==b.ps);
    assert(!std::memcmp(a.streams.data(),b.streams.data(),sizeof(a.streams)));
    assert(!std::memcmp(&a.rt,&b.rt,sizeof(a.rt)) && !std::memcmp(&a.zs,&b.zs,sizeof(a.zs)));
}
static void BatchEqual(const baseline::Replay& a,const candidate::Replay& b) {
    assert(a.cur_->bytes==b.cur_->bytes && a.cur_->vs==b.cur_->vs && a.cur_->ps==b.cur_->ps);
    assert(a.cur_->cmds.size()==b.cur_->cmds.size());
    for(size_t i=0;i<a.cur_->cmds.size();++i) {
        const auto& x=a.cur_->cmds[i];const auto& y=b.cur_->cmds[i];
        assert(unsigned(x.op)==unsigned(y.op));
        assert(std::tie(x.a,x.b,x.c,x.d,x.data,x.size,x.vs,x.ps)==
               std::tie(y.a,y.b,y.c,y.d,y.data,y.size,y.vs,y.ps));
    }
    StateEqual(a.sent_,b.sent_);++checks;
}
static void Equivalent() {
    baseline::Replay a;candidate::Replay b;State s{},other{},raw{};
    s.TrackSimple();other.TrackSimple();
    auto flush=[&]{a.Drain();b.Drain();assert(a.draws.size()==b.draws.size());
        for(size_t i=0;i<a.draws.size();++i)StateEqual(a.draws[i],b.draws[i]);
        a.draws.clear();b.draws.clear();};
    auto queue=[&](const State& state){a.Queue(state);b.Queue(state);BatchEqual(a,b);
        if(a.cur_->cmds.size()>64)flush();};
    queue(s);queue(s);
    // Every simple register and both sides of every block/word boundary, including arbitrary raw methods.
    for(unsigned i=0;i<s.simple.size();++i) {
        s.SetSimple(i,i+1);queue(s);s.SetSimple(i,i+1);queue(s);
        s.SetSimple(i,0);queue(s);
    }
    // Dirty-then-restored values must not produce extra deltas.
    s.SetSimple(17,1);s.SetSimple(17,0);queue(s);
    // Other state remains fully compared, including guest deferred and queued prompt/blur ownership.
    for(auto r:candidate::StateRanges())for(size_t i=0;i<r.len;++i) {
        auto* p=reinterpret_cast<uint8_t*>(&s)+r.off+i;*p^=0x81;queue(s);*p^=0x81;queue(s);
    }
    // No source pointer is a mutation key: untracked callers and source changes always compare all simple bytes.
    raw.simple[100]=71;queue(raw);raw.simple[100]=72;queue(raw);queue(s);
    other.SetSimple(1024,93);queue(other);queue(s);queue(other);queue(s);
    // CreateDevice can reset the same State while keeping the same backend alive.
    s.SetSimple(2047,0xdeadbeef);queue(s);s=State{};s.TrackSimple();queue(s);
    // Recorded shader objects and queued values must survive source destruction/mutation.
    s.vs=std::make_shared<VertexShader>();s.ps=std::make_shared<PixelShader>();
    s.SetSimple(10,123);queue(s);s.SetSimple(10,456);s.vs.reset();s.ps.reset();flush();queue(s);flush();
    // Producer-only metadata is not part of any serialized range.
    for(auto r:ranges)assert(r.off>=baseline::At(s,&s.simple));
    assert(s.simple_dirty[0]==0 && s.simple_dirty[1]==0);
}
static void PixelMappings() {
    State s{};s.TrackSimple();s.ps=std::make_shared<PixelShader>();
    s.ps->def[57]=0x76543210;s.ps->def[58]=0x01234567;s.ps->def[59]=0x10;
    baseline::Replay a;candidate::Replay b;
    a.Queue(s);b.Queue(s);BatchEqual(a,b);
    for(unsigned i=0;i<16;++i) {
        s.psc[i]={float(i)/16,.25f,.5f,1};
        mapping::ApplyPixelConstants(s,i,1);
        a.Queue(s);b.Queue(s);BatchEqual(a,b);
    }
    a.Drain();b.Drain();
    for(size_t i=0;i<a.draws.size();++i)StateEqual(a.draws[i],b.draws[i]);
    uint32_t definition[64]{};State original{};State tracked{};tracked.TrackSimple();
    baseline::Replay shader_a;candidate::Replay shader_b;
    for(unsigned i=0;i<64;++i){definition[i]=i+1;
        LoadPixelShader<false>(original,definition);LoadPixelShader<true>(tracked,definition);
        StateEqual(original,tracked);shader_a.Queue(original);shader_b.Queue(tracked);BatchEqual(shader_a,shader_b);
    }
    shader_a.Drain();shader_b.Drain();
    for(size_t i=0;i<shader_a.draws.size();++i)StateEqual(shader_a.draws[i],shader_b.draws[i]);
}
static void NegativeWitness() {
    candidate::Replay broken;State s{};s.TrackSimple();broken.Queue(s);broken.Drain();
    s.simple[31]=7; // Deliberately bypass the audited setter: this must be detectable.
    broken.Queue(s);broken.Drain();assert(broken.draws.back().simple[31]!=s.simple[31]);
    s.simple_tracked=false;broken.Queue(s);broken.Drain();assert(broken.draws.back().simple[31]==7);
}
static void PacketRanges(){
    for(unsigned first=0;first<=2048;++first)for(unsigned count:{0u,1u,15u,16u,17u,63u,64u,65u,1023u,1024u,1025u,2047u,2048u,~0u}){
        State s{};s.simple_tracked=true;s.MarkSimpleRange(first,count);
        const unsigned bounded=std::min(count,2048-first);
        for(unsigned block=0;block<128;++block){
            bool expected=bounded&&block*16<first+bounded&&(block+1)*16>first;
            assert(bool(s.simple_dirty[block/64]&(uint64_t(1)<<(block%64)))==expected);
        }
    }
    State untracked{};untracked.MarkSimpleRange(0,2048);assert(untracked.simple_dirty[0]==0&&untracked.simple_dirty[1]==0);
    untracked.TrackSimple();untracked.simple_dirty.fill(0);untracked.MarkSimpleRange(~0u,~0u);
    assert(untracked.simple_dirty[0]==0&&untracked.simple_dirty[1]==0);
    auto header=[](unsigned method,unsigned count,bool repeat=false){return method|(count<<18)|(repeat?0x40000000u:0u);};
    Packet<0,baseline::Replay>a;Packet<2,candidate::Replay>b;Packet<1,candidate::Replay>c;
    auto check=[&](const std::vector<uint32_t>& words){a.Load(words);b.Load(words);a.End();b.End();
        a.queue.Queue(a.g_state);b.queue.Queue(b.g_state);BatchEqual(a.queue,b.queue);
        c.Load(words);c.End();c.queue.Queue(c.g_state);BatchEqual(a.queue,c.queue);c.queue.Drain();c.queue.draws.clear();
        StateEqual(a.g_state,b.g_state);assert(a.vertices_drawn==b.vertices_drawn&&a.reports==b.reports);
        a.queue.Drain();b.queue.Drain();assert(a.queue.draws.size()==b.queue.draws.size());
        for(size_t i=0;i<a.queue.draws.size();++i)StateEqual(a.queue.draws[i],b.queue.draws[i]);
        a.queue.draws.clear();b.queue.draws.clear();};
    for(unsigned n:{0u,1u,16u,17u,64u,128u,2047u}){
        std::vector<uint32_t> words{header(0,n)};for(unsigned i=0;i<n;++i)words.push_back(i+1);
        check(words);words[0]=header(kMBlendEnable,n,true);check(words);
    }
    // A draw consumes the mask mid-packet. Remaining simple stores must reach the following draw.
    check({header(0x17fc,1,true),4,header(0x1818,3,true),7,8,9,
           header(0x17f8,4),11,0,22,33,header(kMCountEnable,1),1});
    check({header(kMClearReport,1),0,header(kMGetReport,1),0x1234,header(0x1ffc,3),77,88,99});
    check({header(0x300,2),1}); // Truncated packet must stop before writes.
}
enum Mode {Unchanged,OneWrite,FourWrites,Writes36,Writes64,Writes128,Shader56,Constants18,AllWrites,RepeatedValue,Modes};
static const char* names[]={"unchanged","one-write","four-writes","36-writes","64-writes","128-writes","pixel-shader-56","pixel-constants-18","all-writes","same-value-write"};
template<class T> static double Bench(Mode mode,unsigned count,bool tracked) {
    T queue;State s{};if(tracked)s.TrackSimple();
    uint32_t definition[64];for(unsigned j=0;j<64;++j)definition[j]=j*7+3;
    s.ps=std::make_shared<PixelShader>();s.ps->def[57]=0x76543210;
    s.ps->def[58]=0x01234567;s.ps->def[59]=0x10;
    queue.Queue(s);queue.Discard();
    queue.cur_->bytes.reserve(1024*1024);queue.cur_->cmds.reserve(128);
    auto start=std::chrono::steady_clock::now();
    for(unsigned i=0;i<count;++i) {
        auto write=[&](unsigned at,unsigned v){if(tracked)s.SetSimple(at,v);else s.simple[at]=v;};
        if(mode==OneWrite||mode==FourWrites)write((i*17)%2048,i+1);
        if(mode==FourWrites)for(unsigned j=1;j<4;++j)write((i*17+j*64)%2048,i+1);
        if(mode==Writes36||mode==Writes64||mode==Writes128){
            const unsigned writes=mode==Writes36?36:mode==Writes64?64:128;
            for(unsigned j=0;j<writes;++j)write((i*17+j)%2048,i+j+1);
        }
        if(mode==Shader56){definition[8]=i+1;
            if(tracked)LoadPixelShader<true>(s,definition);else LoadPixelShader<false>(s,definition);}
        if(mode==Constants18){
            for(unsigned j=0;j<16;++j)s.psc[j]={float((i+j)&255)/255,.25f,.5f,1};
            if(tracked)mapping::ApplyPixelConstants(s,0,16);else baseline_mapping::ApplyPixelConstants(s,0,16);
        }
        if(mode==AllWrites)for(unsigned j=0;j<2048;++j)write(j,i+1);
        if(mode==RepeatedValue)write(99,1);
        queue.Queue(s);
        if((i&31)==31){sink=sink+queue.cur_->bytes.size();queue.Discard();}
    }
    sink=sink+queue.cur_->bytes.size();
    return std::chrono::duration<double,std::nano>(std::chrono::steady_clock::now()-start).count()/count;
}
static double Median(std::vector<double> v){std::sort(v.begin(),v.end());return v[v.size()/2];}
static void Benchmark() {
    for(unsigned m=0;m<Modes;++m){std::vector<double>a,b;
        Bench<baseline::Replay>(Mode(m),2000,false);Bench<candidate::Replay>(Mode(m),2000,true);
        for(unsigned t=0;t<9;++t)for(unsigned order=0;order<2;++order)
            if((t+order)%2)b.push_back(Bench<candidate::Replay>(Mode(m),24000,true));
            else a.push_back(Bench<baseline::Replay>(Mode(m),24000,false));
        std::printf("simple state %s: %.2f -> %.2f ns/write-and-draw (%+.2f%%); includes setter cost, no GPU/header reads\n",
                    names[m],Median(a),Median(b),100*(Median(b)/Median(a)-1));
    }
}
template<unsigned tracked,class Transport>static double PacketBench(unsigned count,bool repeat){
    Packet<tracked,Transport> packet;
    std::vector<uint32_t> words{(repeat?kMBlendEnable:0u)|(count<<18)|(repeat?0x40000000u:0u)};
    words.resize(count+1);for(unsigned i=0;i<count;++i)words[i+1]=i+1;
    if(!repeat&&count>0x17fc/4)words[0x17fc/4+1]=0;
    packet.Load(words);packet.End();packet.queue.Queue(packet.g_state);packet.queue.Discard();
    packet.queue.cur_->bytes.reserve(1024*1024);packet.queue.cur_->cmds.reserve(128);
    constexpr unsigned iterations=24000;auto start=std::chrono::steady_clock::now();
    for(unsigned i=0;i<iterations;++i){packet.memory[2]=i+1;packet.End();packet.queue.Queue(packet.g_state);
        if((i&31)==31){sink=sink+packet.queue.cur_->bytes.size();packet.queue.Discard();}}
    return std::chrono::duration<double,std::nano>(std::chrono::steady_clock::now()-start).count()/iterations;
}
static void PacketBenchmark(){for(unsigned count:{1u,64u,2047u})for(bool repeat:{false,true}){
    std::vector<double>a,b,c;for(unsigned trial=0;trial<9;++trial)for(unsigned order=0;order<3;++order)
        switch((trial+order)%3){case 0:a.push_back(PacketBench<0,baseline::Replay>(count,repeat));break;
            case 1:b.push_back(PacketBench<1,candidate::Replay>(count,repeat));break;
            case 2:c.push_back(PacketBench<2,candidate::Replay>(count,repeat));break;}
    std::printf("production EndPush %u %s values: original %.2f, per-value %.2f, per-packet %.2f ns/packet-and-draw (%+.2f%% original, %+.2f%% per-value)\n",count,
        repeat?"repeated":"incrementing",Median(a),Median(b),Median(c),100*(Median(c)/Median(a)-1),100*(Median(c)/Median(b)-1));
}}
int main(int argc,char**argv){Equivalent();PixelMappings();NegativeWitness();PacketRanges();
    std::printf("simple state: %u byte-identical queue comparisons; every register, deferred/tail metadata, resets, identity switches, delayed replay and omitted-writer negative witness pass\n",checks);
    if(argc>1&&!std::strcmp(argv[1],"--bench")){Benchmark();PacketBenchmark();}}
