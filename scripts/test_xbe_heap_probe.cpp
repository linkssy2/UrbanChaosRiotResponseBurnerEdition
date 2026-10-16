// Execute the production wrappers with controlled allocator results; no guest mutation by the probe.
#undef NDEBUG
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <string>
#include <vector>
#include "cpu/x86_ops.h"

static std::vector<uint8_t> memory(0x600000);
static std::vector<std::string> messages;
static unsigned allocations, releases;
static uint32_t next_pointer = 0x5d8450, free_result = 0xdeadbe01;
static bool TrialGuestReadable(uint32_t p, uint32_t n) { return p >= 0x10000 && uint64_t(p) + n <= memory.size(); }
static void Log(const char* format, ...) {
    char line[2048]; va_list a; va_start(a, format); vsnprintf(line, sizeof(line), format, a); va_end(a);
    messages.emplace_back(line);
}
#define LOGR(...) Log(__VA_ARGS__)
#include "scripts/xbe_heap_probe.inc"

X86_FUNC(__imp__sub_00083B5D) {
    ++allocations; ctx.eax = next_pointer; ctx.esp += 16; ctx.cf = 1;
    if (next_pointer) X86_MEM8(next_pointer - 11) = 1;
}
X86_FUNC(__imp__sub_000842D8) {
    ++releases;
    uint32_t p = X86_MEM32(ctx.esp + 12);
    if ((free_result & 255) && p) X86_MEM8(p - 11) = 0;
    ctx.eax = free_result; ctx.esp += 16; ctx.zf = 1;
}
static X86Context Args(uint32_t value, uint32_t heap = 0x530000) {
    uint8_t* base = memory.data();
    X86Context ctx{}; ctx.esp = 0x10000; ctx.ebp = 0x10100; ctx.fs_base = 0x20000;
    ctx.ebx = 0x12345678; ctx.esi = 0x87654321; ctx.edi = 0x11223344;
    for (unsigned i = 0; i < 32; ++i) X86_MEM32(ctx.esp + i * 4) = 0x80000 + i * 4;
    X86_MEM32(ctx.esp + 4) = heap; X86_MEM32(ctx.esp + 8) = 0; X86_MEM32(ctx.esp + 12) = value;
    X86_MEM32(0x5206c4) = 0x530000;
    X86_MEM32(ctx.ebp) = ctx.ebp + 16; X86_MEM32(ctx.ebp + 4) = 0x2d73eb;
    X86_MEM32(ctx.ebp + 16) = 0; X86_MEM32(ctx.ebp + 20) = 0xb9c1e;
    return ctx;
}
static void Allocate(uint32_t heap = 0x530000) {
    X86Context ctx = Args(32, heap), expected = ctx;
    expected.eax = next_pointer; expected.esp += 16; expected.cf = 1;
    sub_00083B5D(ctx, memory.data());
    assert(std::memcmp(&ctx, &expected, sizeof(ctx)) == 0);
}
static void Free(uint32_t p) {
    X86Context ctx = Args(p), expected = ctx;
    expected.eax = free_result; expected.esp += 16; expected.zf = 1;
    sub_000842D8(ctx, memory.data());
    assert(std::memcmp(&ctx, &expected, sizeof(ctx)) == 0);
}
int main(int argc, char**) {
    using namespace uc_heap_probe;
    const bool disabled = argc > 1;
    _putenv_s("UCRR_XBE_HEAP_PROBE", disabled ? "0" : "1");
    Allocate(); Free(next_pointer);
    assert(allocations == 1 && releases == 1);
    if (disabled) { assert(history.empty() && messages.empty()); puts("heap probe disabled delegation passed"); return 0; }
    assert(messages.empty() && !history.at(next_pointer).live);
    Allocate(); assert(history.at(next_pointer).live && messages.empty());
    const auto first = history.at(next_pointer).allocation;
    assert(first.stack[0] == 0x80000 && first.frames[0] == 0x2d73eb && first.frames[1] == 0xb9c1e);
    Allocate(); assert(reports == 1 && allocations == 3); // Still delegates the bad allocation.
    assert(messages[0].find("allocated-live-address") != std::string::npos);
    Free(next_pointer); Free(next_pointer);
    assert(reports == 2 && releases == 3); // Still delegates the duplicate free.
    bool found = false;
    for (auto& line : messages) found |= line.find("free-nonbusy-block") != std::string::npos;
    assert(found);
    Allocate(); free_result = 0xaabbcc00; Free(next_pointer);
    assert(history.at(next_pointer).live); // AL, not full EAX, determines success.
    free_result = 1; Free(next_pointer); assert(!history.at(next_pointer).live);
    const auto count = history.size(); Allocate(0x540000); assert(history.size() == count);
    next_pointer = 0; Allocate(); assert(history.size() == count);
    next_pointer = 0x5d8480; Free(next_pointer); assert(reports == 3);
    history.clear();
    for (size_t i = 0; i < kHistoryLimit; ++i) history.emplace(uint32_t(i + 1), Entry{});
    Allocate(); assert(history.size() == kHistoryLimit);
    for (unsigned i = 0; i < 20; ++i) Free(next_pointer);
    unsigned anomalies = 0;
    for (auto& line : messages) anomalies += line.find("UC heap anomaly") == 0;
    assert(anomalies == 8);
    puts("heap probe history, diagnostics, limits and unchanged ABI passed");
}
