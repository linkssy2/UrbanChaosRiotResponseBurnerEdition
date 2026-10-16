// Private diagnostic only: observe the original allocator without repairing it.
#include <array>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <unordered_map>

namespace uc_heap_probe {
constexpr size_t kHistoryLimit = 65536;
struct Call {
    uint32_t esp = 0, ebp = 0, fs = 0, words = 0;
    std::array<uint32_t, 32> stack{};
    std::array<uint32_t, 8> frames{};
};
struct Entry { bool live = false; uint64_t serial = 0; Call allocation, release; };
static std::mutex mutex;
static std::unordered_map<uint32_t, Entry> history;
static uint64_t serial = 0;
static unsigned reports = 0;

static bool Enabled() {
    static const bool enabled = [] { const char* p = std::getenv("UCRR_XBE_HEAP_PROBE"); return p && !std::strcmp(p, "1"); }();
    return enabled;
}
static Call Capture(const X86Context& ctx, uint8_t* base) {
    Call c; c.esp = ctx.esp; c.ebp = ctx.ebp; c.fs = ctx.fs_base;
    if (TrialGuestReadable(ctx.esp, sizeof(c.stack))) {
        std::memcpy(c.stack.data(), base + ctx.esp, sizeof(c.stack));
        c.words = unsigned(c.stack.size());
    }
    for (unsigned i = c.words; i < c.stack.size(); ++i) {
        uint64_t address = uint64_t(ctx.esp) + i * 4;
        if (address > UINT32_MAX || !TrialGuestReadable(uint32_t(address), 4)) break;
        c.stack[c.words++] = X86_MEM32(uint32_t(address));
    }
    uint32_t frame = ctx.ebp;
    for (unsigned i = 0; i < c.frames.size(); ++i) {
        if (!TrialGuestReadable(frame, 8)) break;
        c.frames[i] = X86_MEM32(frame + 4);
        uint32_t next = X86_MEM32(frame);
        if (next <= frame || uint64_t(next) - frame > 1024 * 1024) break;
        frame = next;
    }
    return c;
}
static void Print(const char* label, const Call& c) {
    LOGR("UC heap %s esp=%08X ebp=%08X fs=%08X words=%u", label, c.esp, c.ebp, c.fs, c.words);
    for (unsigned i = 0; i < c.words; i += 8)
        LOGR("UC heap stack +%03X: %08X %08X %08X %08X %08X %08X %08X %08X", i * 4,
             c.stack[i], c.stack[i+1], c.stack[i+2], c.stack[i+3], c.stack[i+4], c.stack[i+5], c.stack[i+6], c.stack[i+7]);
    LOGR("UC heap frame candidates: %08X %08X %08X %08X %08X %08X %08X %08X", c.frames[0], c.frames[1], c.frames[2], c.frames[3], c.frames[4], c.frames[5], c.frames[6], c.frames[7]);
}
static void Report(const char* reason, uint32_t heap, uint32_t ptr, uint32_t flags, uint8_t* base, const Call& current, const Entry* prior) {
    if (reports++ >= 8) return;
    LOGR("UC heap anomaly %s heap=%08X pointer=%08X flags=%08X serial=%llu tracked=%zu", reason, heap, ptr, flags, (unsigned long long)serial, history.size());
    if (ptr >= 16 && TrialGuestReadable(ptr - 16, 32))
        LOGR("UC heap block: %08X %08X %08X %08X user: %08X %08X %08X %08X", X86_MEM32(ptr-16), X86_MEM32(ptr-12), X86_MEM32(ptr-8), X86_MEM32(ptr-4), X86_MEM32(ptr), X86_MEM32(ptr+4), X86_MEM32(ptr+8), X86_MEM32(ptr+12));
    Print("current", current);
    if (prior) { Print("last allocation", prior->allocation); Print("last release", prior->release); }
}
static bool Arguments(const X86Context& ctx, uint8_t* base, uint32_t& heap, uint32_t& flags, uint32_t& value) {
    if (!Enabled() || !TrialGuestReadable(ctx.esp, 16) || !TrialGuestReadable(0x5206c4, 4)) return false;
    heap = X86_MEM32(ctx.esp+4); flags = X86_MEM32(ctx.esp+8); value = X86_MEM32(ctx.esp+12);
    // 84C7C returns this process heap; don't classify other allocator owners.
    return heap && heap == X86_MEM32(0x5206c4);
}
}

X86_EXTERN_FUNC(__imp__sub_00083B5D);
X86_EXTERN_FUNC(__imp__sub_000842D8);
X86_FUNC(sub_00083B5D) {
    using namespace uc_heap_probe;
    uint32_t heap, flags, size;
    if (!Arguments(ctx, base, heap, flags, size)) { __imp__sub_00083B5D(ctx, base); return; }
    const Call call = Capture(ctx, base);
    __imp__sub_00083B5D(ctx, base);
    const uint32_t ptr = ctx.eax;
    if (!ptr) return;
    std::lock_guard<std::mutex> lock(mutex);
    auto found = history.find(ptr);
    if (found != history.end() && found->second.live)
        Report("allocated-live-address", heap, ptr, flags, base, call, &found->second);
    if (found == history.end() && history.size() == kHistoryLimit) return;
    Entry& entry = history[ptr]; entry.live = true; entry.serial = ++serial; entry.allocation = call;
}
X86_FUNC(sub_000842D8) {
    using namespace uc_heap_probe;
    uint32_t heap, flags, ptr;
    if (!Arguments(ctx, base, heap, flags, ptr) || !ptr) { __imp__sub_000842D8(ctx, base); return; }
    const Call call = Capture(ctx, base);
    uint64_t release_serial = 0;
    bool previous_live = false;
    {
        std::lock_guard<std::mutex> lock(mutex);
        auto found = history.find(ptr);
        const bool free_header = ptr >= 16 && TrialGuestReadable(ptr - 16, 16) && !(X86_MEM8(ptr - 11) & 1);
        if (free_header || (found != history.end() && !found->second.live))
            Report(free_header ? "free-nonbusy-block" : "free-already-released", heap, ptr, flags, base, call, found == history.end() ? nullptr : &found->second);
        if (found != history.end() || history.size() < kHistoryLimit) {
            Entry& entry = history[ptr]; previous_live = entry.live;
            entry.live = false; entry.serial = release_serial = ++serial; entry.release = call;
        }
    }
    __imp__sub_000842D8(ctx, base);
    // This API returns BOOLEAN in AL. A failed release must not be recorded as successful.
    if (!(ctx.eax & 255) && release_serial) {
        std::lock_guard<std::mutex> lock(mutex);
        Entry& entry = history.at(ptr);
        if (entry.serial == release_serial) entry.live = previous_live;
    }
}
