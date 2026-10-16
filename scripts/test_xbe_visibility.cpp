// The generated fixture exercises production result delivery and the real XBE
// reader. The fake GPU completes only submitted queries, not fabricated results.
#undef NDEBUG
#include <array>
#include <atomic>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <mutex>
#include <vector>
#include "xdk/hle.h"
#include "cpu/x86_ops.h"

namespace kernel {
static std::array<uint8_t, 0x800000> memory{};
uint8_t* g_base = memory.data();
}
using namespace kernel;

static constexpr uint32_t kDevice = 0x3f4240, kDevPush = 0;
static std::recursive_mutex g_lock;
static unsigned walks, submissions;
static uint32_t queued_report;
static bool complete_submission = true;
static bool TrialGuestReadable(uint32_t p, uint32_t n) {
    return p >= 0x10000 && n && uint64_t(p) + n <= memory.size();
}
static uint32_t Dev(uint32_t offset) { return R32(kDevice + offset); }

struct D3D12Backend {
    struct Fence {
        uint64_t done = 0;
        uint64_t GetCompletedValue() const { return done; }
    } fence_object;
    Fence* fence_ = &fence_object;
    struct Query { uint32_t index; float samples_per_pixel; };
    struct Report { uint32_t va; std::vector<Query> queries; uint64_t fence; };
    std::deque<Report> reports_;
    std::array<uint64_t, 4> query_counts_{};
    uint64_t presents_ = 91;
    void Submit(bool wait) {
        assert(wait && walks);
        ++submissions;
        if (complete_submission) fence_object.done = 7;
    }
    void Flush();
    void DeliverReports();
};
static D3D12Backend backend;
static D3D12Backend* g_backend = &backend;
static void Walk(uint32_t end) {
    assert(end == 0x70000);
    ++walks;
    if (queued_report) {
        backend.reports_.push_back({queued_report, {{0, 2.f}, {1, 4.f}}, 7});
        queued_report = 0;
    }
}

// GENERATED_FLUSH
// GENERATED_DELIVER
// GENERATED_ADAPTER

[[noreturn]] void x86_trap(X86Context&, uint32_t, const char*) { std::abort(); }
X86_FUNC(sub_003E94A0) { (void)base; ctx.eax = 0; ctx.esp += 8; }
// GENERATED_ORIGINAL

static uint32_t invoke(bool original, uint32_t index, uint32_t pixels, uint32_t time) {
    X86Context ctx{};
    ctx.esp = 0x60000;
    ctx.ebx = 11; ctx.esi = 12; ctx.edi = 13; ctx.ebp = 14;
    W32(ctx.esp, 0xfedcba98); W32(ctx.esp + 4, index);
    W32(ctx.esp + 8, pixels); W32(ctx.esp + 12, time);
    W32(ctx.esp - 4, 0x12345678); W32(ctx.esp + 16, 0x87654321);
    if (original) __imp__sub_003E2C30(ctx, g_base);
    else sub_003E2C30(ctx, g_base);
    assert(ctx.esp == 0x60010);
    assert(ctx.ebx == 11 && ctx.esi == 12 && ctx.edi == 13 && ctx.ebp == 14);
    assert(R32(0x60010) == 0x87654321);
    return ctx.eax;
}

int main() {
    constexpr uint32_t index = 0x1ff, page = 0x30000, report = page + 255 * 16;
    constexpr uint32_t pixels = 0x10000, timestamp = 0x10004;
    W32(0x3f4238, kDevice); W32(kDevice, 0x70000);
    W32(kDevice + 0x7d4 + 4, page);
    backend.query_counts_ = {30, 20, 0, 0};

    // Complete during this poll, without Present. Walk must queue the report
    // before Flush submits it; original readback then agrees byte for byte.
    W32(report + 12, 0xffffffff); queued_report = report;
    assert(invoke(false, index, pixels, timestamp) == 0);
    assert(walks == 1 && submissions == 1 && backend.reports_.empty());
    assert(R32(pixels) == 20 && R32(timestamp) == 91 && R32(timestamp + 4) == 0);
    assert(R32(report + 12) == 0);
    W32(pixels, 0); W32(timestamp, 0); W32(timestamp + 4, 0);
    assert(invoke(true, index, pixels, timestamp) == 0);
    assert(R32(pixels) == 20 && R32(timestamp) == 91 && R32(timestamp + 4) == 0);

    // Already completed reports do not flush; any non-sentinel status is ready.
    W32(report + 12, 5); W32(report, 0x11223344); W32(report + 4, 0x55667788);
    assert(invoke(false, index, pixels, timestamp) == 0 && submissions == 1);
    assert(R32(timestamp) == 0x11223344 && R32(timestamp + 4) == 0x55667788);
    W32(timestamp, 0xdeadbeef);
    assert(invoke(false, index, pixels, 0) == 0 && R32(timestamp) == 0xdeadbeef);

    // No completed GPU query means pending, never a fake pixel count. Preserve
    // outputs, and check the oracle's matching pending status.
    W32(report + 12, 0xffffffff); W32(report, 100); W32(kDevice + 0x2c, 101);
    W32(pixels, 0xaabbccdd); W32(timestamp, 0xdeadbeef);
    g_backend = nullptr;
    assert(invoke(false, index, pixels, timestamp) == 0x88760828);
    assert(invoke(true, index, pixels, timestamp) == 0x88760828);
    assert(R32(pixels) == 0xaabbccdd && R32(timestamp) == 0xdeadbeef);
    g_backend = &backend; complete_submission = false; backend.fence_object.done = 0;
    queued_report = report;
    assert(invoke(false, index, pixels, timestamp) == 0x88760828);
    assert(backend.reports_.size() == 1 && R32(pixels) == 0xaabbccdd);
    complete_submission = true;
    assert(invoke(false, index, pixels, 0) == 0 && R32(pixels) == 20);

    // Invalid index/output/page cannot corrupt another guest allocation.
    auto before = walks;
    assert(invoke(false, 65536, pixels, 0) == 0x80070057);
    assert(invoke(false, index, 0, 0) == 0x80070057);
    assert(invoke(false, index, pixels, 0x7ffffc) == 0x80070057);
    W32(kDevice + 0x7d4 + 4, 0xfffffff0);
    assert(invoke(false, index, pixels, 0) == 0x80070057);
    assert(walks == before);
    std::puts("Visibility: real completion delivery, raw-command ordering, XBE result parity, pending preservation and stdcall ABI passed");
}
