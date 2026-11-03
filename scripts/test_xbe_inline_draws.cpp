// Production Begin/EndPush, declaration parsing and DrawUp consume packets made
// by the actual 9B4C4..9B688 title instructions. GPU submission alone is captured.
#undef NDEBUG
#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <new>
#include <vector>
#include "xdk/hle.h"
#include "cpu/x86_ops.h"
#include "gpu/state.h"

// GENERATED_SYMBOLS

static bool count_cpp_allocations = false;
static size_t cpp_allocations = 0;
void* operator new(size_t n) {
    if (count_cpp_allocations) ++cpp_allocations;
    if (void* p = std::malloc(n ? n : 1)) return p;
    throw std::bad_alloc();
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, size_t) noexcept { std::free(p); }

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
static bool benchmark = false;
static uint64_t benchmark_sum = 0;
struct Event {
    uint32_t kind, argument, prim = 0, count = 0, stride = 0;
    std::vector<uint32_t> simple;
    std::vector<uint8_t> bytes;
    bool operator==(const Event&) const = default;
};
static std::vector<Event> events;
#define LOG_FIRST(...) (++warnings)

namespace mem {
static uint32_t AllocContiguous(uint32_t bytes, uint32_t, uint32_t, uint32_t) {
    assert(bytes <= 0x10000);
    ++allocations;
    return 0x100000;
}
static void FreeContiguous(uint32_t) { assert(false); }
uint32_t VirtualFromPhysical(uint32_t pa) {
    // The real helper's mapping; report addresses do not dereference this alias.
    return pa < kPhysicalSize ? kContiguousBase | pa : pa;
}
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
    assert(bytes <= 0x10000);
    std::memcpy(HostPtr(0x500000), vertices, bytes);
    return 0x510000;
}
struct Backend {
    void ClearReport() {
        Event e{1, 0};
        e.simple.assign(g_state.simple.begin(), g_state.simple.end());
        events.push_back(std::move(e));
    }
    void GetReport(uint32_t address) {
        Event e{2, address};
        e.simple.assign(g_state.simple.begin(), g_state.simple.end());
        events.push_back(std::move(e));
    }
    void Draw(const gpu::State& state, uint32_t prim, uint32_t start, uint32_t count, const uint16_t* indices) {
        assert(!start && !indices && state.streams[0].vb == 0x510000);
        auto data = static_cast<uint8_t*>(HostPtr(0x500000));
        if (benchmark) {
            benchmark_sum += count + prim + R32(0x500000) + R32(0x500000 + count * state.streams[0].stride - 4);
            return;
        }
        submitted_state = state;
        submitted_count = count;
        submitted_prim = prim;
        submitted.assign(data, data + count * state.streams[0].stride);
        Event e{3, 0, prim, count, state.streams[0].stride};
        e.simple.assign(state.simple.begin(), state.simple.end());
        e.bytes = submitted;
        events.push_back(std::move(e));
        ++draws;
    }
} backend;
static Backend* g_backend = &backend;
// GENERATED_VISIBILITY
// GENERATED_DRAWUP
// GENERATED_PUSH
// GENERATED_REFERENCE_PUSH

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

using Words = std::vector<uint32_t>;
static void Packet(Words& out, uint32_t method, const Words& values, bool nonincrement = true) {
    assert(values.size() <= 2047);
    out.push_back((nonincrement ? 0x40000000u : 0u) | (uint32_t(values.size()) << 18) | method);
    out.insert(out.end(), values.begin(), values.end());
}
static Words Payload(uint32_t n) {
    Words words(n);
    // Opaque bits, including non-finite floats and values that resemble commands.
    const uint32_t edge[] = {0, 0xffffffffu, 0x7fc00001u, 0x80000000u, 0x417fcu};
    for (uint32_t i = 0; i < n; ++i) words[i] = i < 5 ? edge[i] : i * 0x9e3779b9u;
    return words;
}
static uint32_t Parse(bool reference, uint32_t end) {
    X86Context ctx{};
    ctx.esp = 0x600000;
    W32(ctx.esp + 4, end);
    hle_args_D3DDevice_EndPush a{ctx};
    return reference ? ReferenceEndPush(ctx, a) : hle_D3DDevice_EndPush(ctx, a);
}
struct Result {
    std::vector<Event> trace;
    std::vector<uint32_t> simple;
    unsigned warnings, draws, walks;
    bool operator==(const Result&) const = default;
};
static Result Execute(const Words& words, uint32_t published, bool reference, bool backend_present = true) {
    auto initial = g_state;
    g_state.simple.fill(0x11223344u);
    events.clear(); warnings = draws = walks = 0;
    submitted.clear();
    g_backend = backend_present ? &backend : nullptr;
    uint32_t start = ScratchPush(uint32_t(words.size() * 4 + 4));
    if (!words.empty()) std::memcpy(HostPtr(start), words.data(), words.size() * 4);
    W32(start + uint32_t(words.size() * 4), 0xabcddcbau);
    g_push_begin = start;
    assert(published <= words.size() * 4);
    assert(Parse(reference, start + published) == start + published);
    assert(!g_push_begin && Dev(kDevPush) == start && walks == 2);
    assert(g_state.streams[0].vb == initial.streams[0].vb && g_state.streams[0].stride == initial.streams[0].stride);
    assert(g_state.vs == initial.vs && g_state.vs_handle == initial.vs_handle);
    assert(R32(start + uint32_t(words.size() * 4)) == 0xabcddcbau);
    Result result{events, {g_state.simple.begin(), g_state.simple.end()}, warnings, draws, walks};
    g_state = initial; g_backend = &backend;
    return result;
}
static unsigned cases = 0;
static Result Compare(const Words& words, uint32_t published, bool backend_present = true) {
    auto original = Execute(words, published, true, backend_present);
    auto candidate = Execute(words, published, false, backend_present);
    assert(original == candidate);
    ++cases;
    return candidate;
}
static Result Compare(const Words& words) { return Compare(words, uint32_t(words.size() * 4)); }

static void PacketChecks() {
    Words words;
    Packet(words, 0x1818, Payload(17)); // Ignored before BEGIN.
    Packet(words, 0x17fc, {8});
    Packet(words, 0x1818, Payload(7));
    Packet(words, 0x1800, {0xfedcba98u});
    Packet(words, gpu::kMCountEnable, {1});
    Packet(words, gpu::kMClearReport, {0});
    Packet(words, 0x1818, Payload(45));
    Packet(words, gpu::kMGetReport, {0xab123456u});
    Packet(words, 0x17fc, {0});
    Packet(words, gpu::kMCountEnable, {0});
    Packet(words, gpu::kMGetReport, {0xff654321u});
    auto r = Compare(words);
    assert(r.draws == 1 && !r.warnings && r.trace.size() == 4);
    assert(r.trace[0].kind == 1 && r.trace[1].kind == 2 && r.trace[2].kind == 3 && r.trace[3].kind == 2);
    assert(r.trace[1].argument == 0x80123456u && r.trace[3].argument == 0x80654321u);
    assert(r.trace[2].simple[gpu::kMCountEnable >> 2] == 1 && r.trace[3].simple[gpu::kMCountEnable >> 2] == 0);
    assert(r.trace[2].simple[0x1800 >> 2] == 0xfedcba98u);
    auto expected = Payload(7), tail = Payload(45); expected.insert(expected.end(), tail.begin(), tail.end());
    assert(r.trace[2].bytes.size() == expected.size() * 4 && !std::memcmp(r.trace[2].bytes.data(), expected.data(), expected.size() * 4));
    Compare(words, uint32_t(words.size() * 4), false);
    // Every incomplete byte boundary, including a valid prefix followed by a partial packet.
    for (uint32_t n = 0; n < words.size() * 4; ++n) Compare(words, n);
    words.clear();
    Packet(words, 0x17fc, {8});
    for (unsigned i = 0; i < 13; ++i) Packet(words, 0x1814, {0x12345678u, i, 0x87654321u}, false);
    Packet(words, 0x17fc, {0});
    r = Compare(words); // Incrementing packets cross INLINE_ARRAY once, not in bulk.
    assert(r.draws == 1 && r.trace[0].bytes.size() == 52);
    for (unsigned i = 0; i < 13; ++i) assert(std::memcmp(r.trace[0].bytes.data() + i * 4, &i, 4) == 0);
    words.clear(); Packet(words, 0x17fc, {8}); Packet(words, 0x1818, Payload(13), false); Packet(words, 0x17fc, {0});
    r = Compare(words); assert(!r.draws && r.warnings == 1);
    for (uint32_t n : {0u, 1u, 12u, 13u, 52u, 1976u, 2028u, 2047u}) {
        words.clear(); Packet(words, 0x17fc, {8}); Packet(words, 0x1818, Payload(n)); Packet(words, 0x17fc, {0});
        r = Compare(words);
        assert(r.draws == unsigned(n && n % 13 == 0) && r.warnings == unsigned(n % 13 != 0));
    }
    words.clear(); Packet(words, 0x17fc, {8}); Packet(words, 0x1818, Payload(2047)); Packet(words, 0x1818, Payload(7)); Packet(words, 0x17fc, {0});
    r = Compare(words); assert(r.draws == 1 && r.trace[0].bytes.size() == 2054 * 4);
    for (uint32_t bad : {0x40001819u, 0x40011818u, 0x40021818u, 0x60001818u, 0xc0001818u, 0xffffffffu, 0x5ffc1818u}) {
        words.clear(); Packet(words, 0x17fc, {8}); words.push_back(bad); words.push_back(123); Packet(words, 0x17fc, {0});
        r = Compare(words); assert(!r.draws && r.warnings == 1);
    }
    words.clear(); Packet(words, 0x17fc, {8}); Packet(words, 0x1818, Payload(13));
    Packet(words, 0x17fc, {5}); Packet(words, 0x1818, Payload(26)); Packet(words, 0x17fc, {0}); Packet(words, 0x17fc, {0});
    r = Compare(words); assert(r.draws == 1 && r.trace[0].prim == 5 && r.trace[0].bytes.size() == 104);
    Compare({});
}

static void Benchmark(bool reference) {
    Words words; Packet(words, 0x17fc, {8}); Packet(words, 0x1818, Payload(39 * 52)); Packet(words, 0x17fc, {0});
    uint32_t start = ScratchPush(uint32_t(words.size() * 4));
    std::memcpy(HostPtr(start), words.data(), words.size() * 4);
    benchmark = true;
    for (unsigned i = 0; i < 100; ++i) { g_push_begin = start; Parse(reference, start + uint32_t(words.size() * 4)); }
    cpp_allocations = 0; count_cpp_allocations = true;
    auto before = std::chrono::steady_clock::now();
    constexpr unsigned iterations = 100000;
    for (unsigned i = 0; i < iterations; ++i) { g_push_begin = start; Parse(reference, start + uint32_t(words.size() * 4)); }
    auto elapsed = std::chrono::duration<double, std::nano>(std::chrono::steady_clock::now() - before).count();
    count_cpp_allocations = false; benchmark = false;
    assert(cpp_allocations == 0 && benchmark_sum);
    std::printf("{\"parser\":\"%s\",\"iterations\":%u,\"ns_per_packet\":%.3f,\"steady_allocations\":%zu,\"checksum\":%llu}\n",
        reference ? "reference" : "bulk", iterations, elapsed / iterations, cpp_allocations, (unsigned long long)benchmark_sum);
}

int main(int argc, char** argv) {
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
    PacketChecks();
    std::printf("inline quad draws: original producer and %u parser parity cases passed\n", cases);
    if (argc == 2 && !std::strcmp(argv[1], "--bench-reference")) Benchmark(true);
    if (argc == 2 && !std::strcmp(argv[1], "--bench-candidate")) Benchmark(false);
}
