// Production selector and original setter/callback/wait code. A blank is a
// controlled callback event here, not evidence of live timing or game speed.
#undef NDEBUG
#include <array>
#include <atomic>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "xdk/hle.h"
#include "cpu/x86_ops.h"

namespace kernel {
static std::array<uint8_t, 0x800000> memory{};
uint8_t* g_base = memory.data();
}
using namespace kernel;
namespace app {
struct Settings { uint32_t fps_cap = 60; };
static Settings settings;
Settings& Get() { return settings; }
}
namespace mem {
static uint32_t denied_page = 0xffffffff;
bool IsCommitted(uint32_t page) {
    return page < kernel::memory.size() && page != denied_page;
}
}
#include "scripts/xbe_input_memory.h"

[[noreturn]] void x86_trap(X86Context&, uint32_t, const char*) { std::abort(); }
// GENERATED_SETTER
// GENERATED_CALLBACK
static unsigned forwarded, blanks, swaps, flushes, wait_calls, skipped;
static unsigned constructed, destroyed;
static bool execute_original = true;
static bool TrialCompletedDisplayInterval(const X86Context& ctx);
X86_FUNC(__imp__sub_0008BD50) {
    ++forwarded;
    if (execute_original) OriginalSetter(ctx, base);
}
// Lifecycle originals are forwarding witnesses; their pinned production bodies
// remain untouched, including construction's direct initial interval write.
X86_FUNC(__imp__sub_0008BD80) { ++constructed; }
X86_FUNC(__imp__sub_0008BB90) { ++destroyed; }
X86_FUNC(sub_003E2C20) { ++flushes; ctx.esp += 4; }
X86_FUNC(sub_003E2CC0) {
    ++wait_calls;
    if (TrialCompletedDisplayInterval(ctx)) {
        ++skipped;
        ctx.esp += 4;
        return;
    }
    assert(++blanks <= 10);
    OriginalCallback(ctx, base);
}
X86_FUNC(sub_003E5140) { ++swaps; ctx.eax = 0; ctx.esp += 8; }
// GENERATED_WAIT
X86_FUNC(__imp__sub_0008BFE0) { OriginalWait(ctx, base); }
// GENERATED_WRAPPER

static constexpr uint32_t object = 0x10000, stack = 0x20000;
static X86Context Context(uint32_t caller = 0x5d0c8, uint32_t requested = 2) {
    X86Context ctx{};
    ctx.ecx = object; ctx.esp = stack;
    ctx.eax = 9; ctx.edx = 10; ctx.ebx = 11; ctx.esi = 12; ctx.edi = 13; ctx.ebp = 14;
    W32(stack, caller); W32(stack + 4, requested); W32(stack + 8, 0xabcdef01);
    return ctx;
}
static void Seed() {
    g_trial_target60_owner.store(0);
    W32(0x519f70, object); W32(object, 0x4563e0);
    W32(object + 0x18, 77); W32(object + 0x1c, 0);
    mem::denied_page = 0xffffffff;
    execute_original = true;
}
static void CheckSetter(X86Context& ctx, uint32_t expected, uint32_t caller, uint32_t requested) {
    sub_0008BD50(ctx, g_base);
    assert(ctx.eax == expected && R32(object + 0x18) == expected);
    assert(ctx.esp == stack + 8 && ctx.ecx == object && ctx.edx == 10);
    assert(ctx.ebx == 11 && ctx.esi == 12 && ctx.edi == 13 && ctx.ebp == 14);
    assert(R32(stack) == caller && R32(stack + 4) == requested && R32(stack + 8) == 0xabcdef01);
    assert(R32(object + 0x1c) == 0); // setter never manufactures a blank
}
static void Wait(uint32_t mode, uint32_t already, unsigned expected_blanks,
                 unsigned expected_skips = 0) {
    W32(0x515120, 1); W32(0x515124, 0); W32(0x51511c, 1);
    W32(0x515118, 0x70000); W32(0x70000, 0x71000); W32(0x71000, mode);
    W32(object + 0x1c, already);
    auto ctx = Context();
    blanks = swaps = flushes = wait_calls = skipped = 0;
    sub_0008BFE0(ctx, g_base);
    assert(ctx.esp == stack + 4 && ctx.esi == 12 && ctx.edi == 13);
    assert(blanks == expected_blanks && swaps == 1 && R32(object + 0x1c) == 0);
    assert(flushes == 1 && skipped == expected_skips);
    assert(wait_calls == blanks + skipped);
    assert(R32(stack) == 0x5d0c8 && R32(stack + 4) == 2);
}

static void Qualify() {
    Seed(); app::Get().fps_cap = 60;
    auto ctx = Context(); CheckSetter(ctx, 1, 0x5d0c8, 2);
    assert(g_trial_target60_owner.load() == object);
}

static X86Context WaitContext() {
    auto ctx = Context(0x8c02c);
    ctx.edi = object;
    W32(object + 0x1c, 1);
    return ctx;
}

static void CheckAdmission(X86Context& ctx, bool expected) {
    const auto before = ctx;
    const std::array<uint32_t, 4> display{
        R32(0x519f70), R32(object), R32(object + 0x18), R32(object + 0x1c)};
    assert(TrialCompletedDisplayInterval(ctx) == expected);
    assert(!std::memcmp(&ctx, &before, sizeof(ctx)));
    assert((display == std::array<uint32_t, 4>{
        R32(0x519f70), R32(object), R32(object + 0x18), R32(object + 0x1c)}));
}

static void CheckCompletedInterval() {
    for (uint32_t cap : {0u, 30u, 59u, 60u, 120u})
    for (uint32_t count : {0u, 1u, 2u, 0x7fffffffu, 0x80000000u, 0xffffffffu}) {
        Qualify(); auto ctx = WaitContext();
        app::Get().fps_cap = cap; W32(object + 0x1c, count);
        CheckAdmission(ctx, (cap == 0 || cap >= 60) && static_cast<int32_t>(count) >= 1);
    }
    for (unsigned invalid = 0; invalid < 13; ++invalid) {
        Qualify(); auto ctx = WaitContext();
        switch (invalid) {
        case 0: ctx.edi += 0x100; break;
        case 1: W32(0x519f70, 0); break;
        case 2: W32(0x519f70, object + 0x100); break;
        case 3: W32(object, 0x453acc); break;
        case 4: W32(object + 0x18, 2); break;
        case 5: W32(stack, 0x8c055); break;
        case 6: W32(stack, 0x3e47f0); break;
        case 7: mem::denied_page = object; break;
        case 8: mem::denied_page = 0x519000; break;
        case 9: mem::denied_page = stack; break;
        case 10: ctx.esp = 0xfffffffe; break;
        case 11: ctx.esp = 0; break;
        case 12: g_trial_target60_owner.store(0); break;
        }
        CheckAdmission(ctx, false);
    }
    // A cap change alone cannot enable an unqualified/menu-owned interval.
    Qualify(); auto wait_ctx = WaitContext();
    app::Get().fps_cap = 30; CheckAdmission(wait_ctx, false);
    app::Get().fps_cap = 60; CheckAdmission(wait_ctx, true);
    for (uint32_t cap : {30u, 60u}) {
        Qualify(); app::Get().fps_cap = cap;
        auto setter = Context(0x5d0c8, cap == 30 ? 2 : 1);
        sub_0008BD50(setter, g_base);
        assert(g_trial_target60_owner.load() == 0);
        app::Get().fps_cap = 60; W32(object + 0x18, 1);
        wait_ctx = WaitContext(); CheckAdmission(wait_ctx, false);
        setter = Context(); sub_0008BD50(setter, g_base);
        wait_ctx = WaitContext(); CheckAdmission(wait_ctx, true);
    }
    // Any later setter on this owner clears qualification, even if it retains1.
    Qualify(); auto setter = Context(0x1fef21, 1);
    sub_0008BD50(setter, g_base);
    wait_ctx = WaitContext(); CheckAdmission(wait_ctx, false);
    // Failed eligibility must never leave stale qualification behind.
    for (unsigned failure = 0; failure < 5; ++failure) {
        Qualify(); setter = Context(); execute_original = false;
        if (failure == 0) mem::denied_page = stack;
        if (failure == 1) mem::denied_page = object;
        if (failure == 2) mem::denied_page = 0x519000;
        if (failure == 3) { setter.ecx = object + 0x100; W32(setter.ecx, 0x453acc); }
        if (failure == 4) { setter.esp = 0xfffffffc; setter.ecx = object + 0x100; }
        sub_0008BD50(setter, g_base);
        assert(g_trial_target60_owner.load() == 0);
    }
    // Unrelated callback setters do not disable the display's valid policy.
    Qualify(); setter = Context(0x1fef21, 1); setter.ecx = object + 0x100;
    sub_0008BD50(setter, g_base);
    wait_ctx = WaitContext(); CheckAdmission(wait_ctx, true);
    // Exact lifecycle wrappers delegate once, preserving registers/stack and
    // clearing ownership even when a new display reuses the identical address.
    for (bool construction : {false, true}) {
        Qualify(); auto ctx = Context(); const auto before = ctx;
        const auto calls = construction ? constructed : destroyed;
        if (construction) sub_0008BD80(ctx, g_base); else sub_0008BB90(ctx, g_base);
        assert((construction ? constructed : destroyed) == calls + 1);
        assert(!std::memcmp(&ctx, &before, sizeof(ctx)));
        wait_ctx = WaitContext(); CheckAdmission(wait_ctx, false);
    }
}

int main() {
    // Reuse the qualified guard matrix; settings now choose its existing opt-in.
    for (uint32_t cap : {0u, 30u, 59u, 60u, 120u})
    for (unsigned same = 0; same < 2; ++same)
    for (unsigned caller_ok = 0; caller_ok < 2; ++caller_ok)
    for (unsigned vtable_ok = 0; vtable_ok < 2; ++vtable_ok)
    for (uint32_t requested : {0u, 1u, 2u, 3u, 0xffffffffu}) {
        Seed(); app::Get().fps_cap = cap;
        uint32_t caller = caller_ok ? 0x5d0c8 : 0x1fef21;
        W32(0x519f70, same ? object : object + 0x100);
        W32(object, vtable_ok ? 0x4563e0 : 0x453acc);
        auto ctx = Context(caller, requested);
        uint32_t expected = (cap == 0 || cap >= 60) && same && caller_ok && vtable_ok && requested == 2 ? 1 : requested;
        CheckSetter(ctx, expected, caller, requested);
    }

    // The actual mapped-page guard disables the policy but still delegates.
    for (uint32_t denied : {0x519000u, object, stack}) {
        Seed(); app::Get().fps_cap = 60; mem::denied_page = denied;
        auto ctx = Context(); CheckSetter(ctx, 2, 0x5d0c8, 2);
    }
    // Invalid original inputs are forwarding-only witnesses; the wrapper must
    // not introduce its own guest reads or claim to validate the original call.
    for (unsigned which = 0; which < 4; ++which) {
        Seed(); execute_original = false;
        auto ctx = Context();
        if (which == 0) ctx.ecx = 0;
        if (which == 1) ctx.ecx = 0xfffffff0;
        if (which == 2) ctx.esp = 0xfffffffc;
        if (which == 3) ctx.esp = 0x7ffffc;
        auto before = ctx; unsigned calls = forwarded;
        sub_0008BD50(ctx, g_base);
        assert(forwarded == calls + 1 && !std::memcmp(&ctx, &before, sizeof(ctx)));
        assert(R32(object + 0x18) == 77 && R32(stack + 4) == 2);
    }

    // Original display wait demonstrates two callbacks at retail30 and one at
    // enhanced60, for both ring modes. Already-published callbacks still count.
    for (uint32_t cap : {30u, 60u, 120u, 144u, 240u, 0u}) {
        Seed(); app::Get().fps_cap = cap;
        auto ctx = Context();
        uint32_t interval = cap == 30 ? 2 : cap == 60 ? 1 : 0;
        CheckSetter(ctx, cap == 30 ? 2 : 1, 0x5d0c8, 2);
        Wait(0, 0, interval); Wait(1, 0, interval, interval == 0 ? 1 : 0);
        Wait(0, interval, 0);
        Wait(1, interval, cap == 30 ? 1 : 0, cap == 30 ? 0 : 1);
        Wait(2, 0, 0); // Non-vblank load mode is unchanged.
    }
    Qualify();
    for (uint32_t cap : {120u, 60u, 240u, 0u, 60u}) {
        app::Get().fps_cap = cap;
        Wait(0, 0, cap == 60 ? 1 : 0);
        Wait(1, 0, cap == 60 ? 1 : 0, cap == 60 ? 0 : 1);
        assert(R32(object + 0x18) == 1);
    }
    // Movies/menus and unknown interval callers retain original waits at every cap.
    for (uint32_t cap : {0u, 30u, 60u, 120u, 144u, 240u}) {
        Seed(); app::Get().fps_cap = cap;
        auto movie = Context(0x5d0c8, 1); CheckSetter(movie, 1, 0x5d0c8, 1);
        Wait(0, 0, 1); Wait(1, 0, 1); Wait(1, 1, 1);
        auto unrelated = Context(0x1fef21, 2); CheckSetter(unrelated, 2, 0x1fef21, 2);
        Wait(0, 0, 2); Wait(1, 0, 2);
    }
    // An ordinary menu interval1 retains its unconditional white-fade blank.
    Seed(); app::Get().fps_cap = 60;
    auto menu = Context(0x5d0c8, 1); CheckSetter(menu, 1, 0x5d0c8, 1);
    Wait(1, 1, 1);
    CheckCompletedInterval();
    std::puts("target60: production interval/owner guards, lifecycle invalidation, signed completed counts and original GPU-idle/wait/reset/swap ABI passed; clocks unchanged");
}
