#pragma once
#include <cstdint>

namespace uc_stream {
// Retail 5849 CHardwareStream::GetStatus (4293B8). The high word is part
// of the XMO contract: title251E10 tests PLAYING before starting lip sync.
constexpr uint32_t Status(uint16_t flags, bool accepts_input) {
    uint32_t result = accepts_input ? 1u : 0u;
    if (flags & 0x2000) result |= 0x80000; // completed release envelope
    if ((flags & 3) == 3) {
        if (flags & 0x444) {
            result |= 0x20000;           // paused, including sync/underrun holds
            if (flags & 0x400) result |= 0x40000;
        } else result |= 0x10000;        // playing
    } else if ((flags & 0x8001) == 0x8001) result |= 0x10000;
    return result;
}

template<class Voice> uint32_t HostStatus(const Voice& v) {
    uint16_t flags = v.active ? 3 : 0;
    if (v.paused) flags |= 4;
    if (v.sync) flags |= 0x40;
    // An exhausted stream remains active awaiting another packet. Retail
    // 429BF9 suspends it with flag400; a Discontinuity instead deactivates it.
    if (v.active && v.packets.empty()) flags |= 0x400;
    // Do not claim an envelope completion: the host currently has no release
    // envelope state. A normal end-of-stream is not that event.
    return Status(flags, v.packets.size() < v.max_packets);
}
}
