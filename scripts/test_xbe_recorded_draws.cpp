// The current translator's original XG compiler supplies packets, not a hand-made decoder fixture.
#undef NDEBUG
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include "cpu/x86_ops.h"
#include "control_model_receiver.h"

[[noreturn]] void x86_trap(X86Context&, uint32_t address, const char* reason) {
    std::fprintf(stderr, "Original XG compiler trapped at %08X: %s\n", address, reason);
    std::abort();
}

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

void Rejected(const std::vector<uint8_t>& packet, size_t bytes, uint32_t vertices, size_t capacity) {
    std::vector<uint32_t> out(capacity + 2, 0xA55AA55A);
    size_t count = 17;
    uint32_t topology = 99;
    assert(!uc_control_model_indexed(packet.data(), bytes, vertices, out.data(), capacity, &count, &topology));
    assert(count == 17 && topology == 99);
    assert(std::all_of(out.begin(), out.end(), [](uint32_t value) { return value == 0xA55AA55A; }));
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
        ++checks;
    }
    Rejected(packet, bytes, 65536, count - 1);
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
}
}

int main() {
    unsigned cases = 0;
    for (uint32_t topology : {5u, 6u}) {
        for (uint32_t count : {3u, 4u, 6u, 30u, 31u, 32u, 33u, 315u, 316u, 317u, 318u,
                               4092u, 4093u, 4094u, 4095u, 8190u, 65534u}) {
            if (topology == 5 && count % 3) continue;
            Check(topology, count);
            ++cases;
        }
    }
    std::printf("Recorded draws: %u original compiler cases, %u decode/ABI/guard checks passed\n", cases, checks);
}
