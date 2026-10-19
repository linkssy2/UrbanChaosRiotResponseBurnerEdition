"""Carry the qualified, narrowly scoped PC display interval policy into the trial."""
from pathlib import Path
import hashlib
import struct


WRAPPER = '''#include "src/display_interval_policy.h"
X86_EXTERN_FUNC(__imp__sub_0008BD50);
X86_EXTERN_FUNC(__imp__sub_0008BD80);
X86_EXTERN_FUNC(__imp__sub_0008BB90);

// Only a verified retail-two-blank request may use the enhanced PC cadence.
// A later setter invalidates ownership before changing that display's interval.
static std::atomic<uint32_t> g_trial_target60_owner{0};

// Construction writes the initial interval directly. Invalidate even when the
// allocator reuses the previous display address with the same vtable.
X86_FUNC(sub_0008BD80) {
    g_trial_target60_owner.store(0, std::memory_order_release);
    __imp__sub_0008BD80(ctx, base);
}

X86_FUNC(sub_0008BB90) {
    g_trial_target60_owner.store(0, std::memory_order_release);
    __imp__sub_0008BB90(ctx, base);
}

static bool TrialCompletedDisplayInterval(const X86Context& ctx) {
    const uint32_t cap = app::Get().fps_cap;
    const uint32_t owner = g_trial_target60_owner.load(std::memory_order_acquire);
    if (!(cap == 0 || cap >= 60) || !owner || ctx.edi != owner ||
        !TrialGuestReadable(ctx.esp, 4) || !TrialGuestReadable(0x519f70, 4) ||
        !TrialGuestReadable(owner, 32)) return false;
    // 8C02C is the white-fade mode's unconditional first blank. The caller has
    // already waited for GPU idle; its original loop still checks/resets the
    // count and swaps. Use only a completed callback's current count (the HLE
    // caller holds the vertical-blank completion gate while evaluating this).
    return R32(ctx.esp) == 0x0008c02cu && R32(0x519f70) == owner &&
        R32(owner) == 0x004563e0u && R32(owner + 0x18) == 1 &&
        static_cast<int32_t>(R32(owner + 0x1c)) >= 1;
}

// Explicit PC enhancement: only the game's verified two-blank display request.
// The original setter, callbacks, clocks and simulation deltas retain ownership.
X86_FUNC(sub_0008BD50) {
    const uint32_t cap = app::Get().fps_cap;
    const uint32_t owner = ctx.ecx;
    const bool caller_readable = TrialGuestReadable(ctx.esp, 4);
    if (!caller_readable || owner == g_trial_target60_owner.load(std::memory_order_acquire) ||
        R32(ctx.esp) == 0x0005d0c8u)
        g_trial_target60_owner.store(0, std::memory_order_release);
    uint32_t slot = 0, requested = 0;
    if ((cap == 0 || cap >= 60) && TrialGuestReadable(0x519f70, 4) &&
        TrialGuestReadable(ctx.ecx, 32) && TrialGuestReadable(ctx.esp, 8)) {
        requested = R32(ctx.esp + 4);
        const uint32_t effective = ucrr_display_interval(1, R32(ctx.esp), ctx.ecx,
            R32(0x519f70), R32(ctx.ecx), requested);
        if (effective != requested) {
            slot = ctx.esp + 4;
            W32(slot, effective);
        }
    }
    __imp__sub_0008BD50(ctx, base);
    if (slot) {
        W32(slot, requested); // preserve the consumed caller argument
        g_trial_target60_owner.store(owner, std::memory_order_release);
    }
}

'''


def once(text, old, new):
    assert text.count(old) == 1, old
    return text.replace(old, new, 1)


def between(text, begin, end):
    start = text.index(begin)
    return text[start:text.index(end, start)]


def prepare(source, area):
    area = Path(area)
    xbe = (area / 'assets/Default.xbe').read_bytes()
    base = struct.unpack_from('<I', xbe, 0x104)[0]
    count, table = struct.unpack_from('<II', xbe, 0x11c)
    sections = [struct.unpack_from('<6I', xbe, table - base + i * 56) for i in range(count)]

    def image(va, size):
        s = next(s for s in sections if s[1] <= va and va + size <= s[1] + s[4])
        pos = s[3] + va - s[1]
        return xbe[pos:pos + size]

    # Setter, actual vblank callback and wait loop, title interval selection,
    # vtable slot, and the unchanged wall-clock timestep path.
    pins = {
        (0x8bd50, 10): 'f707ae2ccd6108f77a22f32dd3a544e8d9c1831da5491cfc20acc261d3e27db4',
        (0x8bd70, 13): 'c3499044916cd17b31254eb43f3289cb466db6b3eb7c6355bcf9324cdaec1b7d',
        (0x8bd80, 564): 'bfdc38ac41e70a959bbb9c8d789870dbb011230a3034f185c4687f45fe33819d',
        (0x8bb90, 39): 'eb05d5d7e9df2411e931eb320db2992afacd24c3103fa9e4baea392b8d329881',
        (0x8bfe0, 142): '7dd6651a1b32e7eb28c47e72500dcd83d5e37575cfc310aa55f2d5f84c1297a9',
        (0x5d073, 85): '90095ce9ac433e05dc9c58ffaa820d85364993b3fdfcbfee60754438569eccd7',
        (0x4563f4, 4): 'b4968894429b93396b7fa980dc06e1a1da2959db5b4d2a5a0145dbbd5b6ace2b',
        (0x5d2c4, 339): 'a44ac467c7c4a07e7b09d0a27375115d5db3432f63ff80ff3cc3d3319c3f2c31',
    }
    for (va, size), digest in pins.items():
        assert hashlib.sha256(image(va, size)).hexdigest() == digest, hex(va)

    source = once(source, 'using namespace kernel;\n', 'using namespace kernel;\n\n' + WRAPPER)
    original = (area / 'generated/game/x86_recomp.14.cpp').read_text()
    bodies = {}
    for address, name in [('0008BD50', 'OriginalSetter'), ('0008BD70', 'OriginalCallback'),
                          ('0008BFE0', 'OriginalWait')]:
        body = between(original, f'X86_FUNC(__imp__sub_{address}) {{', f'\nX86_WEAK_FUNC(sub_{address}')
        bodies[name] = body.replace(f'__imp__sub_{address}', name, 1)
    fixture = Path(__file__).with_name('test_xbe_target60.cpp').read_text()
    for marker, body in [('// GENERATED_SETTER', bodies['OriginalSetter']),
                         ('// GENERATED_CALLBACK', bodies['OriginalCallback']),
                         ('// GENERATED_WAIT', bodies['OriginalWait']),
                         ('// GENERATED_WRAPPER', WRAPPER)]:
        fixture = once(fixture, marker, body)
    (area / 'src/compat/test_target60.cpp').write_text(fixture)
    return source
