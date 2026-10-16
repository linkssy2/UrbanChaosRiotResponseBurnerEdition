// Production Begin/EndPush, declaration parsing and DrawUp consume packets made
// by the actual 9B4C4..9B688 title instructions. GPU submission alone is captured.
#undef NDEBUG
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <vector>
#include "xdk/hle.h"
#include "cpu/x86_ops.h"
#include "gpu/state.h"

// GENERATED_SYMBOLS

namespace kernel {
static std::array<uint8_t, 0x800000> memory{};
uint8_t* g_base = memory.data();
}
using namespace kernel;

namespace gpu {
constexpr uint32_t kTypeFloat2H = 0x72;
// GENERATED_PARSE
// GENERATED_FVF
}

static constexpr uint32_t kDevice = 0x3f4240, kDevPush = 0, kDevThreshold = 4;
static constexpr uint32_t kRenderState = 0x3f3d98, kTextureState = 0x3f3e48, kStageStates = 32;
static uint32_t g_scratch_push = 0, g_scratch_size = 0, g_walk = 0, g_push_begin = 0;
static std::recursive_mutex g_lock;
static gpu::State g_state;
static unsigned walks = 0, draws = 0, allocations = 0, warnings = 0;
static std::vector<uint8_t> submitted;
static gpu::State submitted_state;
static uint32_t submitted_count = 0, submitted_prim = 0;
#define LOG_FIRST(...) (++warnings)

namespace mem {
static uint32_t AllocContiguous(uint32_t bytes, uint32_t, uint32_t, uint32_t) {
    assert(bytes <= 0x10000);
    ++allocations;
    return 0x100000;
}
static void FreeContiguous(uint32_t) { assert(false); }
}
static uint32_t Dev(uint32_t offset) { return R32(kDevice + offset); }
static void SetDev(uint32_t offset, uint32_t value) { W32(kDevice + offset, value); }
static void Walk(uint32_t end) {
    // BeginPush drains any earlier raw commands. EndPush must not replay the
    // packed array through Walk, whose immediate-attribute path is different.
    assert(!g_walk || end == g_walk);
    ++walks;
}
// GENERATED_SCRATCH
// GENERATED_SYNC

static uint32_t UpVertices(const void* vertices, uint32_t bytes) {
    assert(bytes <= 39 * 4 * 52);
    std::memcpy(HostPtr(0x500000), vertices, bytes);
    return 0x510000;
}
struct Backend {
    void Draw(const gpu::State& state, uint32_t prim, uint32_t start, uint32_t count, const uint16_t* indices) {
        assert(!start && !indices && state.streams[0].vb == 0x510000);
        submitted_state = state;
        submitted_count = count;
        submitted_prim = prim;
        auto data = static_cast<uint8_t*>(HostPtr(0x500000));
        submitted.assign(data, data + count * state.streams[0].stride);
        ++draws;
    }
} backend;
static Backend* g_backend = &backend;
static void ApplyVisibilityMethod(uint32_t, uint32_t) {}
// GENERATED_DRAWUP
// GENERATED_PUSH

[[noreturn]] void x86_trap(X86Context&, uint32_t, const char*) { std::abort(); }
// GENERATED_ORIGINAL
// GENERATED_DECLARATION

static void StoreFloat(uint32_t address, float value) { std::memcpy(HostPtr(address), &value, 4); }
static void Run(uint32_t quads) {
    constexpr uint32_t stack = 0x600000;
    X86Context ctx{};
    ctx.esp = stack;
    ctx.edi = quads;
    ctx.ebp = 0x12345678;
    std::memset(HostPtr(stack), 0, 0x1000);
    W32(stack - 0x40, 0xabcdef01);
    W32(stack + 0x1000, 0x10fedcba);
    StoreFloat(stack + 0x54, 1.f);
    StoreFloat(stack + 0x34, 1.f);
    for (uint32_t i = 0; i < quads; ++i) {
        for (uint32_t j = 0; j < 4; ++j) {
            StoreFloat(stack + 0x190 + i * 16 + j * 4, float(10 * i + j + 1));
            StoreFloat(stack + 0x400 + i * 16 + j * 4, float(j + 1) / 4);
        }
    }
    auto bound = g_state.vs;
    auto old_stream = g_state.streams[0];
    unsigned old_draws = draws, old_walks = walks;
    EmitOriginal(ctx, g_base);
    assert(ctx.esp == stack && ctx.ebp == 0x12345678);
    assert(R32(stack - 0x40) == 0xabcdef01 && R32(stack + 0x1000) == 0x10fedcba);
    assert(walks == old_walks + 2 && !g_push_begin && Dev(kDevPush) == g_scratch_push);
    assert(g_state.vs == bound && g_state.streams[0].vb == old_stream.vb && g_state.streams[0].stride == old_stream.stride);
    assert(draws == old_draws + (quads != 0));
    assert(R32(g_scratch_push) == 0x417fc && R32(g_scratch_push + 4) == 8);
    assert(R32(g_scratch_push + 8) == (0x40001818 | ((quads * 52) << 18)));
    assert(R32(g_scratch_push + 12 + quads * 208) == 0x417fc);
    assert(R32(g_scratch_push + 16 + quads * 208) == 0);
    if (!quads) return;
    assert(submitted_prim == 8 && submitted_count == quads * 4);
    assert(submitted_state.vs == bound && submitted_state.streams[0].stride == 52);
    assert(submitted_state.rs[gpu::kRsFogEnable] == 7);
    assert(submitted.size() == quads * 208 && !std::memcmp(submitted.data(), HostPtr(g_scratch_push + 12), submitted.size()));
    // A subsequent scratch write cannot alter the backend's submitted bytes.
    auto stable = submitted;
    std::memset(HostPtr(g_scratch_push + 12), 0xcd, submitted.size());
    assert(stable == submitted);
}

int main() {
    g_state.vs_handle = 0x700001;
    g_state.vs = std::make_shared<gpu::VertexShader>();
    g_state.vs->decl = title_decl;
    gpu::ParseDeclaration(*g_state.vs);
    assert(VertexBytes() == 52);
    const uint32_t offsets[] = {0, 12, 16, 28, 40};
    const uint32_t types[] = {0x32, 0x40, 0x32, 0x32, 0x32};
    for (size_t i = 0; i < 5; ++i) {
        assert(g_state.vs->inputs[i].stream == 0);
        assert(g_state.vs->inputs[i].offset == offsets[i] && g_state.vs->inputs[i].type == types[i]);
    }
    g_state.streams[0] = {0x123000, 999}; // Must use shader declaration, not a stale model stream.
    W32(kRenderState + gpu::kRsFogEnable * 4, 7);
    StoreFloat(0x44bf94, 1.f); StoreFloat(0x44c530, 2.f); StoreFloat(0x46c31c, 3.f); StoreFloat(0x4505cc, 255.f);
    for (uint32_t n : {1u, 2u, 38u, 39u, 0u, 1u}) Run(n);
    assert(allocations == 1 && warnings == 0 && draws == 5);
    std::puts("inline quad draws: original producer, ABI, declaration, state, bytes and reuse passed");
}
