// Actual queue/delta/replay code before and after the bounded full-block changes.
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
#include <unordered_map>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
#include <vector>
#include <smmintrin.h>
#include "gpu/state.h"
using namespace gpu;
namespace baseline { /* BASELINE_TRANSPORT */ }
namespace optimized { /* OPTIMIZED_TRANSPORT */ }

static const auto ranges = baseline::StateRanges();
static volatile uint64_t sink = 0;

static void SurfaceEqual(const Surface& a, const Surface& b) {
    assert(a.header == b.header && a.data == b.data && a.format == b.format && a.size == b.size);
    assert(std::bit_cast<uint32_t>(a.present_x) == std::bit_cast<uint32_t>(b.present_x));
    assert(std::bit_cast<uint32_t>(a.present_y) == std::bit_cast<uint32_t>(b.present_y));
}
static void StateEqual(const State& a, const State& b) {
    for (auto range : ranges)
        assert(!std::memcmp(reinterpret_cast<const uint8_t*>(&a) + range.off,
                            reinterpret_cast<const uint8_t*>(&b) + range.off, range.len));
    assert(a.textures == b.textures && a.palettes == b.palettes && a.vs == b.vs && a.ps == b.ps);
    for (unsigned i = 0; i < 16; ++i)
        assert(a.streams[i].vb == b.streams[i].vb && a.streams[i].stride == b.streams[i].stride);
    SurfaceEqual(a.rt, b.rt); SurfaceEqual(a.zs, b.zs);
}
static void BatchEqual(const baseline::Replay& a, const optimized::Replay& b) {
    assert(a.cur_->bytes == b.cur_->bytes && a.cur_->vs == b.cur_->vs && a.cur_->ps == b.cur_->ps);
    assert(a.cur_->cmds.size() == b.cur_->cmds.size());
    for (unsigned i = 0; i < a.cur_->cmds.size(); ++i) {
        const auto& x = a.cur_->cmds[i]; const auto& y = b.cur_->cmds[i];
        assert(unsigned(x.op) == unsigned(y.op));
        assert(std::tie(x.a,x.b,x.c,x.d,x.data,x.size,x.vs,x.ps) == std::tie(y.a,y.b,y.c,y.d,y.data,y.size,y.vs,y.ps));
        assert(std::bit_cast<uint32_t>(x.f) == std::bit_cast<uint32_t>(y.f));
        SurfaceEqual(x.s1, y.s1); SurfaceEqual(x.s2, y.s2);
    }
    StateEqual(a.sent_, b.sent_);
}
static State Scene() {
    State s{};
    for (unsigned i = 0; i < s.simple.size(); ++i) s.simple[i] = i * 13;
    for (unsigned i = 0; i < s.vsc.size(); ++i)
        for (unsigned j = 0; j < 4; ++j) s.vsc[i][j] = float(i * 4 + j) / 17;
    s.uc_prompt_variant = 1; s.uc_fog_pass = 2; s.uc_fog_params_valid = 1; s.uc_fog_params[3] = 10000;
    s.vs = std::make_shared<VertexShader>(); s.ps = std::make_shared<PixelShader>();
    s.textures = {0x1000,0,0x3000,0x4000}; s.streams[0] = {0x8000,24}; s.streams[4] = {0x9000,12};
    s.rt.header = 0x10000; s.rt.data = 0x20000; s.zs.header = 0x30000; s.zs.data = 0x40000;
    return s;
}
static void ComparisonEdges() {
    uint8_t a[1100], b[1100];
    for (unsigned i = 0; i < 1100; ++i) a[i] = b[i] = uint8_t(i * 29);
    unsigned checked = 0;
    for (unsigned shift = 0; shift < 32; ++shift) for (size_t n = 0; n <= 1024; ++n) {
        assert(baseline::Same(a + shift,b + shift,n) && optimized::Same(a + shift,b + shift,n));
        for (size_t pos = 0; pos < n; pos += n > 128 ? 17 : 1) {
            b[shift + pos] ^= 0x80;
            assert(!baseline::Same(a + shift,b + shift,n) && !optimized::Same(a + shift,b + shift,n));
            b[shift + pos] ^= 0x80; ++checked;
        }
    }
    std::printf("state comparisons: %u byte mutations, 32 alignments, lengths 0..1024 pass\n", checked);
}
static void Equivalent() {
    baseline::Replay a; optimized::Replay b;
    State s = Scene(); unsigned queued = 0, checked = 0;
    auto flush = [&] {
        a.Drain(); b.Drain(); assert(a.draws.size() == b.draws.size());
        for (unsigned i = 0; i < a.draws.size(); ++i) StateEqual(a.draws[i],b.draws[i]);
        a.draws.clear(); b.draws.clear(); queued = 0;
    };
    auto queue = [&] { a.Queue(s); b.Queue(s); BatchEqual(a,b); ++checked; if (++queued == 32) flush(); };
    queue(); queue();
    // Mutate every transmitted byte, including each short tail and prompt/fog
    // metadata; float bit patterns are compared without numeric conversion.
    for (auto range : ranges) for (size_t i = 0; i < range.len; ++i) {
        auto* p = reinterpret_cast<uint8_t*>(&s) + range.off + i;
        *p ^= 0x81; queue(); *p ^= 0x81; queue();
    }
    for (unsigned i = 0; i < 96; ++i) {
        s.textures[i%4] ^= 0x100; s.palettes[(i+1)%4] ^= 0x200; s.streams[i%16].vb ^= 0x400;
        s.streams[(i+3)%16].stride ^= 8; s.rt.header ^= 0x1000; s.zs.data ^= 0x4000;
        s.vs = i%3 ? std::make_shared<VertexShader>() : nullptr;
        s.ps = i%2 ? std::make_shared<PixelShader>() : nullptr;
        queue(); queue();
    }
    flush();
    std::printf("state queue: %u states with identical delta bytes, command fields, shader ownership and delayed replay pass\n", checked);
}

enum Mode { Unchanged, SparseOne, SparseFour, RepeatFour, AllChanging, EarlyChanged, MiddleChanged, LateChanged, Modes };
static const char* names[] = {"unchanged","one-block-change","four-block-change","repeat-four-draws","all-blocks-change",
                              "early-block-change","middle-block-change","late-block-change"};
__declspec(noinline) static void Mutate(State& s, Mode mode, unsigned i) {
    if (mode == Unchanged || (mode == RepeatFour && i%4)) return;
    if (mode == AllChanging) {
        auto* p = reinterpret_cast<uint8_t*>(&s);
        for (auto range : ranges) for (size_t j = 0; j < range.len; j += 64) p[range.off+j] ^= 0x81;
        return;
    }
    if (mode == EarlyChanged || mode == MiddleChanged || mode == LateChanged) {
        const auto r = mode == EarlyChanged ? ranges.front() : mode == MiddleChanged ? ranges[2] : ranges.back();
        const size_t at = r.off + (mode == EarlyChanged ? 0 : mode == MiddleChanged ? r.len/2 : r.len-1);
        reinterpret_cast<uint8_t*>(&s)[at] ^= 0x81;
        return;
    }
    s.vsc[104][0] = std::bit_cast<float>(0x3f000000u + i);
    if (mode == SparseOne) return;
    s.vsc[4][1] = std::bit_cast<float>(0xbf000000u + i); s.simple[99] = i; s.tss[1][9] = i;
}
template<class Pipeline> __declspec(noinline) static double Bench(Mode mode, unsigned count) {
    Pipeline p; State s = Scene(); p.Queue(s); p.Discard();
    p.cur_->bytes.reserve(1024*1024); p.cur_->cmds.reserve(128);
    const auto start = std::chrono::steady_clock::now();
    for (unsigned i = 0; i < count; ++i) {
        Mutate(s,mode,i); p.Queue(s);
        if ((i&31) == 31) { sink = sink + p.cur_->bytes.size() + p.cur_->cmds.size(); p.Discard(); }
    }
    const auto end = std::chrono::steady_clock::now();
    sink = sink + p.cur_->bytes.size() + p.cur_->cmds.size();
    return std::chrono::duration<double,std::nano>(end-start).count() / count;
}
static double Median(std::vector<double> v) { std::sort(v.begin(),v.end()); return v[v.size()/2]; }
static void Benchmark() {
    for (unsigned mode = 0; mode < Modes; ++mode) {
        double(*runs[])(Mode,unsigned) = {Bench<baseline::Replay>,Bench<optimized::Replay>};
        for (auto run : runs) run(Mode(mode),3500*2);
        std::vector<double> times[2];
        for (unsigned trial = 0; trial < 9; ++trial) for (unsigned order = 0; order < 2; ++order) {
            const unsigned which = (order+trial)%2;
            times[which].push_back(runs[which](Mode(mode),3500*16));
        }
        std::printf("state bench %s: median %.2f -> %.2f ns/draw (%.2f%%); mutation + Queue, reused storage, no GPU/header reads\n",
                    names[mode],Median(times[0]),Median(times[1]),100*(Median(times[1])/Median(times[0])-1));
    }
}
// Production-derived header snapshots complement the state replay fixture.
namespace header_test {
/* HEADER_CONSTANTS */
constexpr uint32_t kRingBase=0x20000000,kGuardBase=0x10000000;
struct Memory {
    std::vector<uint8_t> source=std::vector<uint8_t>(65536), ring=std::vector<uint8_t>(kHeaderRing);
    uint8_t* guard=nullptr;
};
namespace kernel {
static Memory* memory;
void* HostPtr(uint32_t address) {
    if (address>=kRingBase) {assert(address-kRingBase<kHeaderRing);return memory->ring.data()+address-kRingBase;}
    if (address==kGuardBase) {assert(memory->guard);return memory->guard;}
    assert(address<memory->source.size());return memory->source.data()+address;
}
uint32_t R32(uint32_t address) {uint32_t out;std::memcpy(&out,HostPtr(address),4);return out;}
}
namespace baseline {struct Pipeline {/* BASELINE_HEADER */};}
namespace candidate {struct Pipeline {/* OPTIMIZED_HEADER */};}
static void Fill(Memory& m,uint32_t at,uint32_t type) {
    auto* bytes=at==kGuardBase?m.guard:m.source.data()+at;
    const unsigned n=type<0x20000||type==0x30000?12:20;
    for(unsigned i=0;i<n;++i)bytes[i]=uint8_t(13*i+at);
    std::memcpy(bytes,&type,4);
}
static unsigned checks=0;
template<class A,class B>
static void Equal(const A& a,const B& b,const Memory& x,const Memory& y) {
    assert(a.ring_pos_==b.ring_pos_ && a.headers_.size()==b.headers_.size() && x.ring==y.ring);
    for (const auto& [key,v]:a.headers_) {
        const auto& w=b.headers_.at(key);
        assert(v.at==w.at&&v.made==w.made&&!std::memcmp(v.bytes,w.bytes,kHeaderBytes));
    }
    for(size_t i=0;i<64;++i) {
        assert(a.recent_headers_[i].key==b.recent_headers_[i].key);
        if(a.recent_headers_[i].key) assert(a.recent_headers_[i].value->at==b.recent_headers_[i].value->at);
    }
}
static void Equivalent() {
    Memory x,y; baseline::Pipeline a;candidate::Pipeline b;
    a.ring_=b.ring_=kRingBase;
    auto step=[&](uint32_t h) {
        kernel::memory=&x;const auto u=a.Header(h);
        kernel::memory=&y;const auto v=b.Header(h);
        assert(u==v);Equal(a,b,x,y);++checks;return u;
    };
    step(0); a.ring_=b.ring_=0;step(0xffffffff);a.ring_=b.ring_=kRingBase;
    for(unsigned type=0;type<8;++type) for(unsigned alignment=0;alignment<32;++alignment) {
        const unsigned at=256+alignment;
        Fill(x,at,type<<16);y.source=x.source;
        const auto copy=step(at);const auto made=a.ring_pos_;assert(step(at)==copy&&a.ring_pos_==made);
        if(type<2||type==3)for(unsigned i=12;i<20;++i)assert(x.ring[copy-kRingBase+i]==0);
        for(unsigned i=0;i<20;++i) {
            x.source[at+i]^=0x81;y.source=x.source;step(at);
            x.source[at+i]^=0x81;y.source=x.source;step(at);
        }
    }
    for(unsigned i=0;i<128;++i) {const auto at=1024+(i%2)*512;Fill(x,at,0x40000);y.source=x.source;step(at);}
    Fill(x,8192,0x40000);y.source=x.source;const auto old=step(8192);
    std::array<uint8_t,20> snapshot{};std::memcpy(snapshot.data(),x.ring.data()+old-kRingBase,20);
    x.source[8192+8]^=0x81;y.source=x.source;step(8192);
    assert(!std::memcmp(snapshot.data(),x.ring.data()+old-kRingBase,20));
    a.ring_pos_+=kHeaderRing/2;b.ring_pos_=a.ring_pos_;step(8192);
    a.ring_pos_=3*kHeaderRing-16;b.ring_pos_=a.ring_pos_;step(8192);
    assert(a.ring_pos_==3*kHeaderRing+32);
#ifdef _WIN32
    SYSTEM_INFO si;GetSystemInfo(&si);
    void* gx=VirtualAlloc(nullptr,2*si.dwPageSize,MEM_RESERVE,PAGE_NOACCESS);
    void* gy=VirtualAlloc(nullptr,2*si.dwPageSize,MEM_RESERVE,PAGE_NOACCESS);
    assert(gx&&gy&&VirtualAlloc(gx,si.dwPageSize,MEM_COMMIT,PAGE_READWRITE)&&VirtualAlloc(gy,si.dwPageSize,MEM_COMMIT,PAGE_READWRITE));
    for(unsigned type=0;type<=0x70000;type+=0x10000) {
        const unsigned bytes=type<0x20000||type==0x30000?12:20;
        x.guard=static_cast<uint8_t*>(gx)+si.dwPageSize-bytes;y.guard=static_cast<uint8_t*>(gy)+si.dwPageSize-bytes;
        Fill(x,kGuardBase,type);Fill(y,kGuardBase,type);step(kGuardBase);
        for(unsigned i=4;i<bytes;++i){x.guard[i]^=0x80;y.guard[i]^=0x80;step(kGuardBase);}
    }
    assert(VirtualFree(gx,0,MEM_RELEASE)&&VirtualFree(gy,0,MEM_RELEASE));
#endif
    std::printf("Header equivalence: %u snapshots, all types/alignments, every source byte, zero tails, recent collisions, ring expiry/wrap, delayed bytes, 12/20-byte guard-page boundaries pass\n",checks);
}
}

int main(int argc, char** argv) {
    ComparisonEdges(); Equivalent(); header_test::Equivalent();
    if (argc > 1 && !std::strcmp(argv[1],"--bench")) Benchmark();
}
