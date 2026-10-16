// Generated with the production host getters, CallGuest and retail resource
// reference routines. Only final GPU/heap destruction is replaced by a ledger.
#undef NDEBUG
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <vector>
#include "xdk/hle.h"
#include "cpu/x86_ops.h"

namespace kernel {
static std::array<uint8_t, 0x800000> memory{};
uint8_t* g_base = memory.data();
}
using namespace kernel;

static constexpr uint32_t kDevice = 0x3f4240, kDevRenderTarget = 0x1a04, kDevDepth = 0x1a08;
static constexpr uint32_t parent = 0x10000, child = 0x10100, spare = 0x10200;
static std::array<uint32_t, 3> g_buffers{};
static uint32_t g_buffer_count = 3;
static std::recursive_mutex g_lock;
static std::vector<uint32_t> freed;
static unsigned public_calls;
static uint32_t Dev(uint32_t offset) { return R32(kDevice + offset); }

X86_EXTERN_FUNC(sub_003E5530);
X86_EXTERN_FUNC(sub_003E5570);
X86_EXTERN_FUNC(sub_003E55B0);
X86_EXTERN_FUNC(sub_003E56D0);
[[noreturn]] void x86_trap(X86Context&, uint32_t, const char*) { std::abort(); }
X86_FUNC(sub_003E5450) {
    (void)base;
    freed.push_back(R32(ctx.esp + 4));
    ctx.esp += 8;
}

// GENERATED_ORIGINAL

void x86_icall(X86Context& ctx, uint8_t* base, uint32_t address) {
    switch (address) {
    case 0x3e5530: sub_003E5530(ctx, base); break;
    case 0x3e5570: ++public_calls; sub_003E5570(ctx, base); break;
    case 0x3e55b0: sub_003E55B0(ctx, base); break;
    case 0x3e56d0: sub_003E56D0(ctx, base); break;
    default: std::abort();
    }
}

// GENERATED_CALL_GUEST
// GENERATED_GETTERS

using Guest = void (*)(X86Context&, uint8_t*);
static uint32_t Invoke(Guest function, bool has_arg = false, uint32_t argument = 0) {
    X86Context ctx{};
    ctx.esp = 0x60000;
    ctx.eax = 21; ctx.ecx = 22; ctx.edx = 23;
    ctx.ebx = 31; ctx.esi = 32; ctx.edi = 33; ctx.ebp = 34;
    W32(ctx.esp, 0xfedcba98);
    W32(ctx.esp + 4, argument);
    W32(ctx.esp + 8, 0x87654321);
    function(ctx, g_base);
    assert(ctx.esp == 0x60004 + (has_arg ? 4 : 0));
    assert(ctx.ebx == 31 && ctx.esi == 32 && ctx.edi == 33 && ctx.ebp == 34);
    assert(R32(0x60008) == 0x87654321);
    return ctx.eax;
}

static void Reset() {
    memory.fill(0); freed.clear(); public_calls = 0;
    W32(0x3f4238, kDevice);
    W32(parent, 0x010c0001);  // texture: one public and one device reference
    W32(child, 0x010d0000);   // surface: only its render-target device reference
    W32(child + 0x14, parent);
    W32(spare, 0x01050001);
    W32(kDevice + kDevRenderTarget, child);
    W32(kDevice + kDevDepth, child);
    g_buffers = {spare, child, spare};
    W32(kDevice + 0x1a14, g_buffers[1]);
    W32(kDevice + 0x1a18, g_buffers[0]);
    W32(kDevice + 0x1a1c, g_buffers[2]);
}

static void ReleasePair() {
    Invoke(sub_003E55B0, true, child); // title releases the getter's public hold
    Invoke(sub_003E56D0, true, child); // restoring RT releases the device hold
}

int main() {
    // Reproduce the observed defect with the old raw increment. Original
    // releases then destroy the texture while the application still owns it.
    Reset(); W32(child, R32(child) + 1); ReleasePair();
    assert((freed == std::vector<uint32_t>{parent, child}));

    const Guest hooks[] = {sub_003E35D0, sub_003E35F0, sub_003E3250};
    const Guest originals[] = {__imp__sub_003E35D0, __imp__sub_003E35F0, __imp__sub_003E3250};
    for (unsigned getter = 0; getter < 3; ++getter) {
        for (unsigned refs : {0u, 1u, 7u}) {
            Reset(); W32(child, 0x010d0000 | refs);
            assert(Invoke(originals[getter], getter == 2) == child);
            const auto expected_parent = R32(parent), expected_child = R32(child);
            Reset(); W32(child, 0x010d0000 | refs);
            assert(Invoke(hooks[getter], getter == 2) == child);
            assert(R32(parent) == expected_parent && R32(child) == expected_child);
            assert(public_calls == 1 && freed.empty());
        }
        Reset();
        assert(Invoke(hooks[getter], getter == 2) == child);
        assert(R32(parent) == 0x010c0002 && R32(child) == 0x010d0001);
        ReleasePair();
        assert((freed == std::vector<uint32_t>{child}));
        assert(R32(parent) == 0x01040001);
        // A subsequent bind/unbind cannot free the application's texture.
        Invoke(sub_003E5530, true, parent);
        Invoke(sub_003E56D0, true, parent);
        assert((freed == std::vector<uint32_t>{child}));
        Invoke(sub_003E55B0, true, parent);
        assert((freed == std::vector<uint32_t>{child, parent}));
    }

    // Multiple public getters hold the parent once, until the last release.
    Reset(); Invoke(sub_003E35D0); Invoke(sub_003E35D0);
    assert(R32(parent) == 0x010c0002 && R32(child) == 0x010d0002);
    Invoke(sub_003E55B0, true, child);
    assert(R32(parent) == 0x010c0002);
    ReleasePair(); assert((freed == std::vector<uint32_t>{child}));

    // Parentless surfaces and non-surface resources do not chase parent data.
    for (uint32_t type : {0x50000u, 0x40000u}) {
        Reset(); W32(child, type | 0x80000); W32(child + 0x14, type == 0x50000 ? 0 : parent);
        Invoke(sub_003E35D0);
        assert(R32(child) == (type | 0x80001) && R32(parent) == 0x010c0001);
    }
    Reset(); W32(kDevice + kDevRenderTarget, 0); W32(kDevice + kDevDepth, 0);
    assert(Invoke(sub_003E35D0) == 0 && Invoke(sub_003E35F0) == 0 && public_calls == 0);

    for (uint32_t index : {0xffffffffu, 0u, 1u}) {
        Reset(); auto result = Invoke(__imp__sub_003E3250, true, index);
        auto common = R32(result); auto owner = R32(parent);
        Reset(); assert(Invoke(sub_003E3250, true, index) == result);
        assert(R32(result) == common && R32(parent) == owner);
    }
    std::puts("resource getter ownership: original parity and premature-free regression passed");
}
