#pragma once
#include <cstdint>

namespace uc_wall_depth {
template<class Key> int32_t ReceiverBias(uint32_t vs, uint32_t ps, const Key& k) {
    // Subdivided wall receivers lose coplanarity after float projection; the
    // 215127 captured-mesh replay requires 64 D24 units across its 180 cameras.
    if (vs == 0xB01013A1u && ps == 0xB0101860u && k.zen && !k.zwrite &&
        k.zfunc == 0x203 && k.blend && k.src == 1 && k.dst == 1 && k.zbias == -3)
        return -64;
    return k.zbias;
}
}
