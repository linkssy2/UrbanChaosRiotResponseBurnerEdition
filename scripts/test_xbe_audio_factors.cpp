// Production factor owners and mixer calculations, original public ABI and
// title producer, and original 427DC1 pitch are compiled into this private test.
#undef NDEBUG
#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <map>
#include <mutex>
#include "xdk/hle.h"
#include "cpu/x86_ops.h"
#include "xbe_audio_cone.h"
#include "xbe_voice_mode.h"
namespace kernel {
static std::array<uint8_t, 0x800000> memory{};
uint8_t* g_base = memory.data();
}
using namespace kernel;
namespace audio { constexpr unsigned kRate = 48000; }
static float F32(uint32_t bits) { return std::bit_cast<float>(bits); }
static uint32_t Bits(float value) { return std::bit_cast<uint32_t>(value); }
// GENERATED_STATE
struct Voice {
    UcAudioFactors factors;
    float p[3] = {}, min_dist = 1, max_dist = 100;
    uint32_t freq = 0, rate = 48000, mode = 0;
    bool is3d = true;
    uint32_t pending_mode = UINT32_MAX;
    UcCone cone, pending_cone;
    uint32_t cone_dirty = 0;
};
static std::map<uint32_t, Voice> g_voices;
static std::recursive_mutex g_lock;
static UcAudioFactors g_listener_factors;
static float g_listener[3] = {};
static Voice* Find(uint32_t address) {
    auto it = g_voices.find(address);
    return it == g_voices.end() ? nullptr : &it->second;
}
// GENERATED_COMMIT
// GENERATED_MATH
// GENERATED_DEFERRED
// GENERATED_LISTENER
// GENERATED_VOICE

[[noreturn]] void x86_trap(X86Context&, uint32_t, const char*) { std::abort(); }
static uint32_t captured[3];
static void Capture(X86Context& ctx) {
    for (unsigned i = 0; i < 3; ++i) captured[i] = R32(ctx.esp + 4 + i * 4);
    ctx.eax = 0; ctx.esp += 16;
}
X86_FUNC(sub_00425AED) { Capture(ctx); }
X86_FUNC(sub_00425B43) { Capture(ctx); }
X86_FUNC(sub_00425B99) { Capture(ctx); }
// GENERATED_ORIGINAL
// GENERATED_TITLE

using Guest = void (*)(X86Context&, uint8_t*);
static uint32_t Invoke(Guest f, std::initializer_list<uint32_t> args) {
    X86Context ctx{}; ctx.esp = 0x60000; ctx.fcw = 0x37f;
    ctx.ebx = 11; ctx.esi = 12; ctx.edi = 13; ctx.ebp = 14;
    W32(ctx.esp, 0xeeeeeeee);
    unsigned i = 1;
    for (auto value : args) W32(ctx.esp + 4 * i++, value);
    W32(ctx.esp + 4 * i, 0xabcdef01);
    f(ctx, g_base);
    assert(ctx.esp == 0x60000 + 4 * i);
    assert(ctx.ebx == 11 && ctx.esi == 12 && ctx.edi == 13 && ctx.ebp == 14);
    assert(R32(ctx.esp) == 0xabcdef01);
    return ctx.eax;
}
static uint32_t Apply(Guest f, float x, unsigned apply = 0, unsigned object = 1) {
    return Invoke(f, {object, Bits(x), apply});
}
static bool Near(double a, double b) { return std::abs(a - b) < 1e-6 * std::max(1.0, std::abs(b)); }

int main() {
    // GENERATED_CONSTANTS
    auto& v = g_voices[1]; auto& other = g_voices[2];
    const Guest host[] = {sub_004260A6, sub_004260CA, sub_004260EE};
    const Guest original[] = {__imp__sub_004260A6, __imp__sub_004260CA, __imp__sub_004260EE};
    for (unsigned slot = 0; slot < 3; ++slot) {
        assert(Apply(original[slot], 2.5f, 1, 0x11000) == 0);
        assert(captured[0] == 0x11000 - 0x1c && captured[1] == Bits(2.5f) && captured[2] == 1);
        assert(Apply(host[slot], 2.5f, 1) == 0);
        assert(v.factors.value[slot] == 1 && v.factors.pending[slot] == 2.5f);
        assert(other.factors.value[slot] == 1 && other.factors.dirty == 0);
    }
    v.pending_mode = 1;
    v.pending_cone.volume = -1200; v.cone_dirty = 16;
    assert(Apply(sub_004260CA, 3) == 0); // immediate also commits queued distance/rolloff/mode
    assert(v.mode == 1 && v.factors.value[0] == 2.5f && v.factors.value[1] == 3 && v.factors.value[2] == 2.5f);
    assert(v.factors.dirty == 0 && v.cone.volume == -1200 && v.cone_dirty == 0 && v.pending_mode == UINT32_MAX);

    v = Voice{};
    X86Context producer{}; producer.esp = 0x60000; producer.esi = 0x11000; producer.edi = 0x12000;
    W32(producer.esi + 0x20, 1);
    W32(producer.edi + 0x18, Bits(.01f)); W32(producer.edi + 0x1c, Bits(2.f)); W32(producer.edi + 0x20, Bits(.5f));
    OriginalTitleFactors(producer, g_base);
    assert(producer.esp == 0x60000 && v.factors.dirty == 7);
    assert(v.factors.value[0] == 1 && v.factors.pending[0] == .01f && v.factors.pending[1] == .5f && v.factors.pending[2] == 2);
    CommitVoice3D(v);

    for (auto f : host) {
        auto before = v.factors;
        assert(Apply(f, -1) == 0x80070057u);
        assert(Apply(f, std::numeric_limits<float>::quiet_NaN()) == 0x80070057u);
        assert(Apply(f, std::numeric_limits<float>::infinity()) == 0x80070057u);
        assert(Apply(f, 1, 0, 999) == 0x80070057u);
        assert(std::memcmp(&before, &v.factors, sizeof(before)) == 0);
    }
    assert(Apply(sub_004260A6, 0) == 0x80070057u);
    // Retail tests only the low apply bit: 3 defers, 2 commits.
    for (unsigned slot = 0; slot < 3; ++slot) {
        float old = v.factors.value[slot];
        assert(Apply(host[slot], 4, 3) == 0 && v.factors.value[slot] == old);
        assert(Apply(host[slot], 5, 2) == 0 && v.factors.value[slot] == 5 && v.factors.dirty == 0);
    }
    assert(Invoke(sub_00427248, {0, Bits(2), Bits(3), Bits(4), 3}) == 0);
    assert(g_listener_factors.value[3] == 0 && g_listener_factors.pending[3] == 2);
    assert(Invoke(sub_00427248, {0, Bits(5), Bits(6), Bits(7), 2}) == 0);
    assert(g_listener_factors.value[3] == 5 && g_listener_factors.dirty == 0);

    v = Voice{}; v.p[2] = 10;
    assert(Near(FactorAttenuation(v, 10), .1));
    Apply(sub_004260EE, 0); assert(FactorAttenuation(v, 10) == 1);
    Apply(sub_004260EE, 2); assert(Near(FactorAttenuation(v, 10), 1.0 / 19));
    Apply(sub_004260A6, .01f); assert(Near(FactorAttenuation(v, 10), 1.0 / 19));
    assert(Apply(sub_004271A5, 2, 1) == 0 && g_listener_factors.value[0] == 1);
    other.pending_mode = 1; other.pending_cone.volume = -600; other.cone_dirty = 16;
    assert(Invoke(GUEST(IDirectSound_CommitDeferredSettings), {0}) == 0);
    assert(g_listener_factors.value[0] == 2 && other.mode == 1 && other.cone.volume == -600 && other.cone_dirty == 0);

    // Compare host playback-rate shifts with the retail pitch quantization,
    // including receding/approaching, disabled, and speed-of-sound endpoints.
    v = Voice{}; g_listener_factors = UcAudioFactors{}; v.p[2] = 10;
    for (float distance : {.01f, 1.f, 2.f}) for (float doppler : {0.f, .5f, 1.f})
        for (float speed : {-400.f, -342.f, -100.f, 0.f, 50.f, 342.f, 400.f}) {
            Apply(sub_004260A6, distance); Apply(sub_004260CA, doppler);
            Invoke(sub_00427248, {0, Bits(0), Bits(0), Bits(0), 0});
            X86Context velocity{}; velocity.esp = 0x60000;
            W32(0x60004, 1); W32(0x60008, Bits(0)); W32(0x6000c, Bits(0)); W32(0x60010, Bits(speed)); W32(0x60014, 0);
            Args args{velocity}; assert(VoiceVelocity(v, args) == 0);
            Invoke(__imp__sub_00427DC1, {Bits(distance), Bits(doppler), Bits(speed), 0x13000});
            double expected = std::exp2(int32_t(R32(0x13000)) / 4096.0);
            assert(Near(AudioPlaybackStep(v), expected));
        }
    v.mode = 2; assert(AudioPlaybackStep(v) == 1);
    v.mode = 0; v.is3d = false; assert(AudioPlaybackStep(v) == 1);
    v.is3d = true; v.freq = 24000; v.factors.value[1] = 0; assert(AudioPlaybackStep(v) == .5);
    // Finite setters can still overflow/underflow their combined factors.
    // A zero Doppler effect remains neutral; saturated nonzero speeds keep
    // the retail endpoints without converting NaN into an integer pitch.
    v = Voice{}; g_listener_factors = UcAudioFactors{}; v.p[2] = 10;
    float huge = std::numeric_limits<float>::max(), tiny = std::numeric_limits<float>::denorm_min();
    Apply(sub_004260A6, huge); Apply(sub_004271A5, huge);
    Apply(sub_004260CA, tiny); g_listener_factors.value[1] = tiny;
    v.factors.value[5] = huge; g_listener_factors.value[5] = -huge;
    assert(AudioPlaybackStep(v) == 1);
    Apply(sub_004260CA, 1); g_listener_factors.value[1] = 1;
    assert(Near(AudioPlaybackStep(v), std::exp2(-32767 / 4096.0)));
    v.factors.value[5] = -huge; g_listener_factors.value[5] = huge;
    assert(AudioPlaybackStep(v) == 2);
    std::puts("audio factors: retail ABI/title calls, deferred ownership, attenuation and Doppler oracle passed");
}
