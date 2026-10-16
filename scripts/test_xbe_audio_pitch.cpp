// Actual host SetPitch/playback math and retail frequency conversion, setter,
// pitch composition, and title caller. Hardware publication is a call ledger.
#undef NDEBUG
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <map>
#include <mutex>
#include "xdk/hle.h"
#include "cpu/x86_ops.h"

namespace kernel {
static std::array<uint8_t, 0x800000> memory{};
uint8_t* g_base = memory.data();
}
using namespace kernel;
namespace audio { constexpr uint32_t kRate = 48000; }
struct Factors { float value[6] = {1, 1, 1, 0, 0, 0}; };
struct Voice {
    uint32_t rate = 48000, freq = 0, mode = 0;
    bool is3d = false;
    float p[3] = {};
    Factors factors;
};
static std::map<uint32_t, Voice> g_voices;
static std::recursive_mutex g_lock;
static Factors g_listener_factors;
static float g_listener[3] = {};
static Voice* Find(uint32_t object) {
    auto it = g_voices.find(object);
    return it == g_voices.end() ? nullptr : &it->second;
}
// GENERATED_PITCH
// GENERATED_PLAYBACK

[[noreturn]] void x86_trap(X86Context&, uint32_t, const char*) { std::abort(); }
X86_FUNC(sub_004238E9) { ctx.esp += 4; }
X86_FUNC(sub_0042390E) { ctx.esp += 4; }
static unsigned published;
X86_FUNC(sub_0042B8F0) { ++published; ctx.eax = 0; ctx.esp += 4; }
// GENERATED_ORIGINAL
// GENERATED_TITLE

using Guest = void (*)(X86Context&, uint8_t*);
static uint32_t Invoke(Guest function, std::initializer_list<uint32_t> args, uint32_t object = 0) {
    X86Context ctx{};
    ctx.esp = 0x60000; ctx.fcw = 0x37f; ctx.ecx = object;
    ctx.ebx = 11; ctx.esi = 12; ctx.edi = 13; ctx.ebp = 14;
    W32(ctx.esp, 0xeeeeeeee);
    unsigned i = 1;
    for (auto value : args) W32(ctx.esp + 4 * i++, value);
    W32(ctx.esp + 4 * i, 0xabcdef01);
    function(ctx, g_base);
    assert(ctx.esp == 0x60000 + 4 * i);
    assert(ctx.ebx == 11 && ctx.esi == 12 && ctx.edi == 13 && ctx.ebp == 14);
    assert(R32(ctx.esp) == 0xabcdef01);
    return ctx.eax;
}

static constexpr uint32_t object = 0x10000, state = 0x11000, hardware = 0x12000;
static int32_t OriginalPitch(int32_t base, int32_t doppler = 0) {
    W32(object + 0x10, state); W32(object + 0x0c, hardware);
    W32(hardware + 0x70, state);
    W32(state + 8, doppler ? 0x1000000 : 0);
    W32(state + 0xb8, doppler ? 0x13000 : 0);
    W32(0x13024, uint32_t(doppler));
    Invoke(__imp__sub_00424088, {object, uint32_t(base)});
    assert(R32(state + 0x18) == uint32_t(base)); // setter replaces the format pitch
    Invoke(__imp__sub_0042AC20, {0x14000}, hardware);
    return int16_t(R32(0x14000) >> 16);
}

int main() {
    // GENERATED_CONSTANTS
    auto& buffer = g_voices[1]; auto& stream = g_voices[2];
    const Guest hooks[] = {sub_00424F31, sub_0042503E};
    for (uint32_t rate : {12000u, 22050u, 24000u, 44100u, 48000u}) {
        int32_t format_pitch = int32_t(Invoke(__imp__sub_00423BD7, {rate}));
        int32_t reference = OriginalPitch(format_pitch);
        uint32_t expected = uint32_t(std::lround(48000.0 * std::exp2(reference / 4096.0)));
        for (unsigned i = 0; i < 2; ++i) {
            auto& voice = g_voices[i + 1]; voice = Voice{}; voice.rate = rate;
            assert(Invoke(hooks[i], {i + 1, uint32_t(format_pitch)}) == 0);
            assert(voice.freq == expected);
            assert(AudioPlaybackStep(voice) == double(expected) / 48000);
            assert(Invoke(hooks[i], {i + 1, 0}) == 0 && voice.freq == 48000);
        }
    }
    // A 24 kHz asset supplies an absolute -4096 pitch. The old hook produced
    // 12 kHz and half-speed playback by applying its format rate twice.
    int32_t half = int32_t(Invoke(__imp__sub_00423BD7, {24000}));
    assert(half == -4096);
    assert(std::lround(24000 * std::exp2(half / 4096.0)) == 12000);
    buffer.rate = stream.rate = 24000;
    W32(0x15020, 1); W32(0x15024, 2);
    Invoke(OriginalTitlePitch, {uint32_t(half)}, 0x15000);
    assert(buffer.freq == 24000 && stream.freq == 24000);

    // The retail setter stores the supplied pitch; its publisher clamps the
    // complete pitch to -32767..8191. Values below -8192 are valid slow voices.
    for (int32_t pitch : {std::numeric_limits<int32_t>::min(), -32768, -32767,
                          -16384, -8192, -4096, 0, 4096, 8191, 8192,
                          std::numeric_limits<int32_t>::max()}) {
        int32_t reference = OriginalPitch(pitch);
        uint32_t expected = uint32_t(std::lround(48000.0 * std::exp2(reference / 4096.0)));
        assert(Invoke(sub_00424F31, {1, uint32_t(pitch)}) == 0);
        assert(buffer.freq == expected);
        assert(Invoke(sub_0042503E, {2, uint32_t(pitch)}) == 0);
        assert(stream.freq == expected);
    }
    assert(Invoke(sub_00424F31, {999, 0}) == 0x80070057u);
    assert(published > 0);

    // Both contributions may be valid separately but exceed the final APU
    // pitch range together. Compare those saturation endpoints with 42AC20.
    buffer = Voice{}; buffer.is3d = true; buffer.p[2] = 10;
    Invoke(sub_00424F31, {1, 4096}); buffer.factors.value[5] = -400;
    assert(AudioPlaybackStep(buffer) == std::exp2(OriginalPitch(4096, 4096) / 4096.0));
    Invoke(sub_00424F31, {1, uint32_t(-8192)}); buffer.factors.value[5] = 400;
    assert(AudioPlaybackStep(buffer) == std::exp2(OriginalPitch(-8192, -32767) / 4096.0));
    std::puts("audio pitch: retail absolute-rate/title parity, low-rate regression and final clamp passed");
}
