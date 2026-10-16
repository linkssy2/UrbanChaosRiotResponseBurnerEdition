// Production selector and original setter/callback/wait code. A blank is a
// controlled callback event here, not evidence of live timing or game speed.
#undef NDEBUG
#include <array>
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
static unsigned forwarded, blanks, swaps;
static bool execute_original = true;
X86_FUNC(__imp__sub_0008BD50) {
    ++forwarded;
    if (execute_original) OriginalSetter(ctx, base);
}
X86_FUNC(sub_003E2C20) { ctx.esp += 4; }
X86_FUNC(sub_003E2CC0) {
    assert(++blanks <= 10);
    OriginalCallback(ctx, base);
}
X86_FUNC(sub_003E5140) { ++swaps; ctx.eax = 0; ctx.esp += 8; }
// GENERATED_WAIT
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
static void Wait(uint32_t mode, uint32_t already, unsigned expected_blanks) {
    W32(0x515120, 1); W32(0x515124, 0); W32(0x51511c, 1);
    W32(0x515118, 0x70000); W32(0x70000, 0x71000); W32(0x71000, mode);
    W32(object + 0x1c, already);
    auto ctx = Context();
    blanks = swaps = 0;
    OriginalWait(ctx, g_base);
    assert(ctx.esp == stack + 4 && ctx.esi == 12 && ctx.edi == 13);
    assert(blanks == expected_blanks && swaps == 1 && R32(object + 0x1c) == 0);
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
    for (uint32_t cap : {30u, 60u, 0u}) {
        Seed(); app::Get().fps_cap = cap;
        auto ctx = Context();
        uint32_t interval = cap == 30 ? 2 : 1;
        CheckSetter(ctx, interval, 0x5d0c8, 2);
        Wait(0, 0, interval); Wait(1, 0, interval);
        Wait(0, interval, 0); Wait(1, interval, 1);
    }
    std::puts("target60: production guards/settings, original setter ABI, real callback/wait parity passed; clocks unchanged");
}
