// The current translator's original XG compiler supplies packets, not a hand-made decoder fixture.
#undef NDEBUG
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <vector>
#include "cpu/x86_ops.h"
#include "control_model_receiver.h"

#ifdef UC_RECORDED_DECODER_ORACLE
extern "C" int uc_original_control_model_indices(const void*, size_t, uint32_t, uint32_t*, size_t, size_t*);
extern "C" int uc_original_control_model_indexed(const void*, size_t, uint32_t, uint32_t*, size_t, size_t*, uint32_t*);
#endif

struct RecordedTrap { uint32_t address; const char* reason; };
[[noreturn]] void x86_trap(X86Context&, uint32_t address, const char* reason) { throw RecordedTrap{address, reason}; }

#include "original_xg_compile.inc"

namespace {
constexpr uint32_t kPacket = 0x100000, kInput = 0x200000, kSize = 0x300000, kStack = 0x3F0000;
std::vector<uint8_t> memory(0x400000);
unsigned checks = 0;

uint32_t Read(uint32_t at) {
    uint32_t value;
    std::memcpy(&value, memory.data() + at, sizeof(value));
    return value;
}
void Write(uint32_t at, uint32_t value) { std::memcpy(memory.data() + at, &value, sizeof(value)); }
uint16_t Index(uint32_t i) { return uint16_t(i * 71u + 3u); }

uint32_t Compile(uint32_t topology, uint32_t count) {
    std::fill(memory.begin() + kPacket - 32, memory.begin() + kInput, 0xCD);
    for (uint32_t i = 0; i < count; ++i) {
        uint16_t value = Index(i);
        std::memcpy(memory.data() + kInput + 2 * i, &value, sizeof(value));
    }
    auto invoke = [&](uint32_t output, uint32_t capacity) {
        X86Context ctx{};
        ctx.esp = kStack;
        ctx.ebx = 0x11111111; ctx.esi = 0x22222222; ctx.edi = 0x33333333; ctx.ebp = 0x44444444;
        Write(kStack, 0xFEEDCAFE); Write(kStack + 4, output); Write(kStack + 8, kSize);
        Write(kStack + 12, topology); Write(kStack + 16, count); Write(kStack + 20, kInput);
        Write(kStack + 24, 0xCAFEBABE); Write(kSize, capacity);
        __imp__sub_0038590D(ctx, memory.data());
        assert(ctx.eax == 0 && ctx.esp == kStack + 24);
        assert(ctx.ebx == 0x11111111 && ctx.esi == 0x22222222 && ctx.edi == 0x33333333 && ctx.ebp == 0x44444444);
        assert(Read(kStack) == 0xFEEDCAFE && Read(kStack + 24) == 0xCAFEBABE);
    };
    invoke(0, 0);
    uint32_t bytes = Read(kSize);
    assert(bytes >= 24 && bytes < kInput - kPacket - 32 && !(bytes & 3));
    invoke(kPacket, bytes);
    assert(Read(kSize) == bytes);
    for (uint32_t i = 0; i < 32; ++i) {
        assert(memory[kPacket - 32 + i] == 0xCD && memory[kPacket + bytes + i] == 0xCD);
    }
    return bytes;
}

void Rejected(const std::vector<uint8_t>& packet, size_t bytes, uint32_t vertices, size_t capacity, bool query_fails = true) {
    std::vector<uint32_t> out(capacity + 2, 0xA55AA55A);
    size_t count = 17;
    uint32_t topology = 99;
    assert(!uc_control_model_indexed(packet.data(), bytes, vertices, out.data(), capacity, &count, &topology));
    assert(count == 17 && topology == 99);
    assert(std::all_of(out.begin(), out.end(), [](uint32_t value) { return value == 0xA55AA55A; }));
    std::vector<uint16_t> out16(capacity + 2, 0xA55A);
    assert(!uc_control_model_indexed16(packet.data(), bytes, vertices, out16.data(), capacity, &count, &topology));
    assert(count == 17 && topology == 99);
    assert(std::all_of(out16.begin(), out16.end(), [](uint16_t value) { return value == 0xA55A; }));
    if (query_fails) {
        assert(!uc_control_model_indexed(packet.data(), bytes, vertices, nullptr, 0, &count, &topology));
        assert(count == 17 && topology == 99);
        assert(!uc_control_model_indexed16(packet.data(), bytes, vertices, nullptr, 0, &count, &topology));
        assert(count == 17 && topology == 99);
    }
    ++checks;
}

void Check(uint32_t topology, uint32_t count) {
    uint32_t bytes = Compile(topology, count);
    std::vector<uint8_t> packet(memory.begin() + kPacket, memory.begin() + kPacket + bytes);
    std::vector<uint32_t> decoded(count + 2, 0xDEADBEEF);
    size_t required = 17;
    uint32_t result_topology = 99;
    assert(uc_control_model_indexed(packet.data(), bytes, 65536, nullptr, 0, &required, &result_topology));
    assert(required == count && result_topology == topology);
    for (uint32_t terminal : {0u, 1u, 0xFEEDBEEFu, 0xFFFFFFFFu}) {
        std::memcpy(packet.data() + bytes - 4, &terminal, sizeof(terminal));
        assert(uc_control_model_indexed(packet.data(), bytes, 65536, decoded.data(), count, &required, &result_topology));
        assert(required == count && result_topology == topology);
        std::vector<uint16_t> host_indices(count);
        for (uint32_t i = 0; i < count; ++i) {
            assert(decoded[i] == Index(i) && decoded[i] <= UINT16_MAX);
            host_indices[i] = uint16_t(decoded[i]);
        }
        assert(!std::memcmp(host_indices.data(), memory.data() + kInput, count * sizeof(uint16_t)));
        assert(decoded[count] == 0xDEADBEEF && decoded[count + 1] == 0xDEADBEEF);
        std::vector<uint16_t> direct(count + 2, 0xA55A);
        assert(uc_control_model_indexed16(packet.data(), bytes, 65536, nullptr, 0, &required, &result_topology));
        assert(required == count && result_topology == topology);
        assert(uc_control_model_indexed16(packet.data(), bytes, 65536, direct.data(), count, &required, &result_topology));
        assert(required == count && result_topology == topology);
        assert(std::equal(host_indices.begin(), host_indices.end(), direct.begin()));
        assert(direct[count] == 0xA55A && direct[count + 1] == 0xA55A);
        ++checks;
    }
    Rejected(packet, bytes, 65536, count - 1, false);
    Rejected(packet, bytes, 3, count);
    for (size_t truncated : {size_t(0), size_t(4), size_t(8), size_t(12), size_t(bytes - 12),
                             size_t(bytes - 8), size_t(bytes - 4), size_t(bytes - 1)})
        Rejected(packet, truncated, 65536, count);
    for (auto mutation : {std::array<uint32_t, 2>{0, 0x417F8}, {4, 4}, {8, 0x40041D94}, {8, 0x20000}}) {
        auto bad = packet;
        std::memcpy(bad.data() + mutation[0], &mutation[1], 4);
        Rejected(bad, bytes, 65536, count);
    }
    if (count < 316) {
        auto bad = packet;
        bad[bytes - 8] = 1; // NOP padding must remain zero; only the last dword is mutable.
        Rejected(bad, bytes, 65536, count);
    }
    if (count & 1) {
        auto bad = packet;
        size_t at = 8;
        while (at < bytes - 4) {
            uint32_t header;
            std::memcpy(&header, bad.data() + at, 4);
            if (header == 0x41808) {
                uint32_t too_wide = 65536;
                std::memcpy(bad.data() + at + 4, &too_wide, 4);
                Rejected(bad, bytes, 65536, count);
                break;
            }
            at += 4 + ((header >> 18) & 2047) * 4;
        }
        assert(at < bytes - 4);
    }
    const size_t expanded_count = topology == 5 ? count : (count - 2) * 3;
    std::vector<uint32_t> expanded(expanded_count + 2, 0xDEADBEEF);
    assert(uc_control_model_indices(packet.data(), bytes, 65536, nullptr, 0, &required));
    assert(required == expanded_count);
    assert(uc_control_model_indices(packet.data(), bytes, 65536, expanded.data(), expanded_count, &required));
    for (size_t i = 0; i < required; ++i) {
        size_t source = i;
        if (topology == 6) {
            source = i / 3 + i % 3;
            if (i % 3 < 2 && ((i / 3) & 1)) source = i / 3 + 1 - i % 3;
        }
        assert(expanded[i] == Index(uint32_t(source)));
    }
    assert(expanded[expanded_count] == 0xDEADBEEF && expanded[expanded_count + 1] == 0xDEADBEEF);
    ++checks;
}

void CheckVertexBounds() {
    const uint32_t bytes = Compile(6, 4);
    Write(kPacket + 12, 0xFFFF0000); Write(kPacket + 16, 0xFFFE0001);
    std::vector<uint8_t> packet(memory.begin() + kPacket, memory.begin() + kPacket + bytes);
    for (uint32_t vertices : {0u, 1u, 65534u, 65535u, 65537u}) Rejected(packet, bytes, vertices, bytes);
    size_t count = 17;
    uint32_t topology = 99;
    assert(uc_control_model_indexed(packet.data(), bytes, 65536, nullptr, 0, &count, &topology));
    assert(count == 4 && topology == 6);
    std::array<uint32_t, 6> out{99, 99, 99, 99, 99, 99};
    assert(uc_control_model_indexed(packet.data(), bytes, 65536, out.data(), 4, &count, &topology));
    assert((out == std::array<uint32_t, 6>{0, 65535, 1, 65534, 99, 99}));
    std::array<uint16_t, 6> out16{99, 99, 99, 99, 99, 99};
    assert(uc_control_model_indexed16(packet.data(), bytes, 65536, out16.data(), 4, &count, &topology));
    assert((out16 == std::array<uint16_t, 6>{0, 65535, 1, 65534, 99, 99}));
    Write(kPacket + 12, 0x00010000); Write(kPacket + 16, 0x00020001);
    assert(uc_control_model_indexed(memory.data() + kPacket, bytes, 3, nullptr, 0, &count, &topology));
    assert(count == 4 && topology == 6);
    assert(uc_control_model_indexed(memory.data() + kPacket, bytes, 3, out.data(), 4, &count, &topology));
    assert((out == std::array<uint32_t, 6>{0, 1, 1, 2, 99, 99}));
    assert(uc_control_model_indexed16(memory.data() + kPacket, bytes, 3, out16.data(), 4, &count, &topology));
    assert((out16 == std::array<uint16_t, 6>{0, 1, 1, 2, 99, 99}));
    // Empty payload packets and unaligned source bytes retain the same four indices.
    packet.insert(packet.begin() + 8, 4, 0);
    const uint32_t empty_header = 0x40001800; std::memcpy(packet.data() + 8, &empty_header, 4);
    packet.insert(packet.begin(), 0xAB);
    assert(uc_control_model_indexed16(packet.data() + 1, packet.size() - 1, 65536, out16.data(), 4, &count, &topology));
    assert(count == 4 && topology == 6 && (out16 == std::array<uint16_t, 6>{0, 65535, 1, 65534, 99, 99}));
    const auto before = out16; count = 17; topology = 99;
    assert(!uc_control_model_indexed16(packet.data() + 1, packet.size() - 1, 65536, out16.data(), 4, nullptr, &topology));
    assert(!uc_control_model_indexed16(packet.data() + 1, packet.size() - 1, 65536, out16.data(), 4, &count, nullptr));
    assert(!uc_control_model_indexed16(nullptr, packet.size() - 1, 65536, out16.data(), 4, &count, &topology));
    assert(count == 17 && topology == 99 && out16 == before);
    auto reject_short = [](const std::vector<uint32_t>& words) {
        std::vector<uint8_t> bytes(words.size() * 4);
        std::memcpy(bytes.data(), words.data(), bytes.size());
        Rejected(bytes, bytes.size(), 65536, 8);
    };
    reject_short({0x417FC, 6, 0x40001800, 0x417FC, 0, 0}); // All-empty payload.
    reject_short({0x417FC, 6, 0x40001800, 0x41808, 0, 0x417FC, 0, 0}); // One index.
    reject_short({0x417FC, 6, 0x40041800, 0x00010000, 0x417FC, 0, 0}); // Two indices.
    ++checks;
}

#ifdef UC_RECORDED_DECODER_ORACLE
void CheckOriginalDecoder() {
    uint32_t random = 0x43A2196B;
    auto next = [&]() { random ^= random << 13; random ^= random >> 17; random ^= random << 5; return random; };
    size_t comparisons = 0;
    for (uint32_t topology : {5u, 6u}) for (uint32_t count : {3u, 30u, 31u, 315u, 316u, 317u, 4095u, 65534u, 65537u}) {
        if (topology == 5 && count % 3) continue;
        const uint32_t bytes = Compile(topology, count);
        const std::vector<uint8_t> original(memory.begin() + kPacket, memory.begin() + kPacket + bytes);
        for (unsigned trial = 0; trial < 40; ++trial) {
            auto packet = original;
            size_t length = bytes;
            if (trial % 5 == 1) packet[next() % bytes] ^= uint8_t(1u << (next() & 7));
            if (trial % 5 == 2) length = next() % bytes;
            if (trial % 5 == 3) {
                const size_t at = (next() % (bytes / 4)) * 4;
                const uint32_t value = next(); std::memcpy(packet.data() + at, &value, 4);
            }
            if (trial % 5 == 4) { const uint32_t value = next(); std::memcpy(packet.data() + bytes - 4, &value, 4); }
            for (uint32_t vertices : {0u, 3u, 65535u, 65536u, 65537u}) for (bool expand : {false, true}) {
                const size_t capacity = bytes * 3 / 2 + 8;
                for (unsigned mode = 0; mode < 3; ++mode) {
                    std::vector<uint32_t> old_out(capacity + 2, 0xA55AA55A), new_out = old_out;
                    size_t old_count = 17, new_count = 17;
                    uint32_t old_topology = 99, new_topology = 99;
                    uint32_t* old_ptr = mode == 0 ? nullptr : old_out.data();
                    uint32_t* new_ptr = mode == 0 ? nullptr : new_out.data();
                    const size_t limit = mode == 2 ? count - 1 : capacity;
                    const int old_result = expand ? uc_original_control_model_indices(packet.data(), length, vertices, old_ptr, limit, &old_count)
                        : uc_original_control_model_indexed(packet.data(), length, vertices, old_ptr, limit, &old_count, &old_topology);
                    const int new_result = expand ? uc_control_model_indices(packet.data(), length, vertices, new_ptr, limit, &new_count)
                        : uc_control_model_indexed(packet.data(), length, vertices, new_ptr, limit, &new_count, &new_topology);
                    assert(old_result == new_result && old_count == new_count && old_topology == new_topology && old_out == new_out);
                    ++comparisons;
                    if (!expand) {
                        std::vector<uint16_t> direct(capacity + 2, 0xA55A), expected(capacity + 2, 0xA55A);
                        if (old_result && mode != 0) for (size_t i = 0; i < old_count; ++i) expected[i] = uint16_t(old_out[i]);
                        new_count = 17; new_topology = 99;
                        const int result16 = uc_control_model_indexed16(packet.data(), length, vertices, mode == 0 ? nullptr : direct.data(), limit, &new_count, &new_topology);
                        assert(old_result == result16 && old_count == new_count && old_topology == new_topology && expected == direct);
                        ++comparisons;
                    }
                }
            }
        }
    }
    std::printf("Original decoder differential: %zu query/output/capacity comparisons passed\n", comparisons);
}
#endif

namespace RecordedCaller {
constexpr uint32_t kResource = 0x310000, kDevPush = 0x1234;
std::recursive_mutex g_lock;
struct State { uint32_t vs_handle = 0; struct { uint32_t vb = 0; } streams[1]; } g_state;
struct Draw { uint32_t primitive, start, count; std::vector<uint16_t> indices; };
struct Backend {
    std::vector<Draw> queued;
    void Draw(const State&, uint32_t primitive, uint32_t start, uint32_t count, const uint8_t* indices) {
        RecordedCaller::Draw draw{primitive, start, count, std::vector<uint16_t>(count)};
        std::memcpy(draw.indices.data(), indices, size_t(count) * 2);
        queued.push_back(std::move(draw));
    }
} backend;
Backend* g_backend = &backend;
struct DecodeCall { bool output, success; size_t capacity, count; uint32_t primitive; };
std::vector<DecodeCall> calls;
bool guard_failed_output = false;
unsigned walks = 0, syncs = 0;
bool TrialGuestReadable(uint32_t at, uint32_t bytes) {
    return at >= 0x10000 && bytes && uint64_t(at) + bytes <= memory.size();
}
uint32_t R32(uint32_t at) { return Read(at); }
uint8_t* HostPtr(uint32_t at) { return memory.data() + at; }
uint32_t Dev(uint32_t at) { return at; }
void Walk(uint32_t at) { assert(at == kDevPush); ++walks; }
void SyncDeferred() { assert(walks == syncs + 1); ++syncs; }
int CountedDecode(const void* packet, size_t bytes, uint32_t vertices, uint16_t* out,
                  size_t capacity, size_t* count, uint32_t* primitive) {
    const size_t old_count = *count;
    const uint32_t old_primitive = *primitive;
    std::vector<uint16_t> before;
    if (guard_failed_output && out) before.assign(out, out + capacity);
    int result = uc_control_model_indexed16(packet, bytes, vertices, out, capacity, count, primitive);
    if (!result) {
        assert(*count == old_count && *primitive == old_primitive);
        if (!before.empty()) assert(std::equal(before.begin(), before.end(), out));
    }
    calls.push_back({out != nullptr, result != 0, capacity, *count, *primitive});
    return result;
}
#define HLE_STDCALL(name, argc) uint32_t name(X86Context& ctx, const uint32_t* a)
#define LOGE(...) ((void)0)
#define LOGR(...) ((void)0)
#define uc_control_model_indexed16 CountedDecode
#include "scripts/xbe_recorded_draws.inc"
#undef uc_control_model_indexed16
#undef LOGR
#undef LOGE
#undef HLE_STDCALL
#undef SYM_UrbanChaos_RunPushBuffer

void Invoke(uint32_t bytes, uint32_t resource = kResource, uint32_t fixup = 0) {
    Write(kResource, 0x20000); Write(kResource + 4, kPacket); Write(kResource + 12, bytes);
    const uint32_t args[]{resource, fixup};
    X86Context ctx{};
    assert(UrbanChaos_RunPushBuffer(ctx, args) == 0);
}
void Valid(uint32_t topology, uint32_t count, unsigned expected_calls) {
    const auto prior = backend.queued;
    const unsigned prior_walks = walks;
    calls.clear();
    Invoke(Compile(topology, count));
    assert(calls.size() == expected_calls && walks == prior_walks + 1 && syncs == walks);
    assert(backend.queued.size() == prior.size() + 1);
    const auto& draw = backend.queued.back();
    assert(draw.primitive == topology && draw.start == 0 && draw.count == count && draw.indices.size() == count);
    for (uint32_t i = 0; i < count; ++i) assert(draw.indices[i] == Index(i));
    for (size_t i = 0; i < prior.size(); ++i) assert(prior[i].indices == backend.queued[i].indices);
    ++checks;
}
void Bad(uint32_t bytes, uint32_t resource = kResource, uint32_t fixup = 0) {
    const auto prior = backend.queued;
    const unsigned prior_walks = walks;
    guard_failed_output = true;
    bool trapped = false;
    try { Invoke(bytes, resource, fixup); }
    catch (RecordedTrap trap) { assert(trap.address == 0x3EF870); trapped = true; }
    guard_failed_output = false;
    assert(trapped && walks == prior_walks && syncs == walks && backend.queued.size() == prior.size());
    for (size_t i = 0; i < prior.size(); ++i) assert(prior[i].indices == backend.queued[i].indices);
    ++checks;
}
void CheckCaller() {
    Valid(5, 30, 2); // Cold: count, then validated output.
    Valid(5, 30, 1); // Warm: validated output, without the separate count pass.
    Valid(6, 317, 3); // Failed capacity attempt, count, validated output.
    assert(calls[0].output && !calls[0].success && calls[0].capacity == 30);
    Valid(6, 3, 1); // A small draw must not shrink the index high-water size.
    assert(calls[0].capacity == 317);
    Valid(6, 317, 1);
    uint32_t bytes = Compile(6, 31);
    Write(kPacket + 12, 0x0000FFFF); // Reusing the same guest address must decode its current contents.
    calls.clear();
    Invoke(bytes);
    assert(calls.size() == 1 && backend.queued.back().indices[0] == 65535 && backend.queued.back().indices[1] == 0);
    assert(backend.queued[0].indices[0] == Index(0));
    ++checks;
    // Corruption remains rejected before any Walk, deferred-state sync or Draw.
    bytes = Compile(6, 31);
    Write(kPacket, 0x417F8); Bad(bytes);
    bytes = Compile(6, 31); memory[kPacket + bytes - 8] = 1; Bad(bytes);
    bytes = Compile(6, 31); Bad(bytes - 4); Bad(bytes - 1);
    Bad(bytes, 0); Bad(bytes, kResource, 1); Bad(0); Bad(16u*1024u*1024u + 4);
    Valid(5, 315, 1); // Failed decodes cannot poison a subsequent valid draw.
    Valid(6, 65534, 3);
    Valid(6, 65537, 3); // Index count is not limited to the 16-bit index-value range.
    for (uint32_t count : {3u, 4u, 4095u, 65534u, 31u}) Valid(6, count, 1);
    for (uint32_t topology : {5u, 6u})
        for (uint32_t count : {3u, 4u, 6u, 30u, 31u, 32u, 33u, 315u, 316u, 317u, 318u,
                               4092u, 4093u, 4094u, 4095u, 8190u, 65534u})
            if (topology != 5 || !(count % 3)) Valid(topology, count, 1);
    const auto prior = backend.queued;
    g_backend = nullptr;
    Invoke(Compile(5, 6));
    g_backend = &backend;
    assert(backend.queued.size() == prior.size() && walks == syncs);
    // The queued copy must outlive both source-packet mutation and scratch reuse.
    std::fill(memory.begin() + kPacket, memory.begin() + kInput, 0xFF);
    for (size_t i = 0; i < prior.size(); ++i) assert(prior[i].indices == backend.queued[i].indices);
    ++checks;
}
}
}

int main() {
    unsigned cases = 0;
    for (uint32_t topology : {5u, 6u}) {
        for (uint32_t count : {3u, 4u, 6u, 30u, 31u, 32u, 33u, 315u, 316u, 317u, 318u,
                               4092u, 4093u, 4094u, 4095u, 8190u, 65534u, 65537u}) {
            if (topology == 5 && count % 3) continue;
            Check(topology, count);
            ++cases;
        }
    }
    RecordedCaller::CheckCaller();
    CheckVertexBounds();
#ifdef UC_RECORDED_DECODER_ORACLE
    CheckOriginalDecoder();
#endif
    std::printf("Recorded draws: %u original compiler cases, %u decode/ABI/guard checks passed\n", cases, checks);
    return 0;
}
