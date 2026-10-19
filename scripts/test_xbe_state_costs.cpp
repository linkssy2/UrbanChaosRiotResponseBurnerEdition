// Actual queue/delta/replay code before and after the bounded comparison change.
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

enum Mode { Unchanged, SparseOne, SparseFour, RepeatFour, AllChanging, Modes };
static const char* names[] = {"unchanged","one-block-change","four-block-change","repeat-four-draws","all-blocks-change"};
__declspec(noinline) static void Mutate(State& s, Mode mode, unsigned i) {
    if (mode == Unchanged || (mode == RepeatFour && i%4)) return;
    if (mode == AllChanging) {
        auto* p = reinterpret_cast<uint8_t*>(&s);
        for (auto range : ranges) for (size_t j = 0; j < range.len; j += 64) p[range.off+j] ^= 0x81;
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
int main(int argc, char** argv) {
    ComparisonEdges(); Equivalent();
    if (argc > 1 && !std::strcmp(argv[1],"--bench")) Benchmark();
}
