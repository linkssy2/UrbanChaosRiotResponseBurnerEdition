#pragma once
#include <cassert>
#include <cstdint>
#include <cstring>
#include <functional>
#include <vector>
#include "gpu/texture.h"

namespace uc_xbe_signed {
// B1A20 selects RGB-signed A8R8G8B8 normalization cubes. Other formats keep
// their existing decoder until their signed component contracts are verified.
inline uint32_t CubeSigns(const gpu::TextureDesc& d, uint32_t state) {
    return d.cube && d.format == 6 ? (state >> 28) & 15 : 0;
}

struct TextureKey {
    uint32_t data, palette, signs;
    bool operator==(const TextureKey&) const = default;
};
struct TextureKeyHash {
    size_t operator()(const TextureKey& k) const {
        return std::hash<uint64_t>{}((uint64_t(k.palette) << 32) | k.data) ^
               (size_t(k.signs) * size_t(0x9e3779b9));
    }
};

// NVIDIA NV_texture_shader's signed eight-bit texture domain is [-128,127]/128.
// Decode BEFORE filtering: interpolating 0x7f/0x80 as UNORM loses the sign edge.
// This is the same component math as the accepted nv2a_light_textures.h owner.
inline float Component(uint8_t byte, bool signed_channel) {
    return signed_channel ? float(int(byte) - (byte >= 128 ? 256 : 0)) / 128.0f
                          : float(byte) / 255.0f;
}
inline void Expand(std::vector<uint8_t>& bgra, uint32_t& pitch, uint32_t signs) {
    if (!signs) return;
    assert(!(bgra.size() % 4) && !(pitch % 4));
    std::vector<uint8_t> rgba(bgra.size() * 4);
    for (size_t n = 0; n < bgra.size(); n += 4) {
        float value[] = {Component(bgra[n + 2], signs & 2),
                         Component(bgra[n + 1], signs & 4),
                         Component(bgra[n], signs & 8),
                         Component(bgra[n + 3], signs & 1)};
        std::memcpy(rgba.data() + n * 4, value, sizeof(value));
    }
    bgra.swap(rgba);
    pitch *= 4;
}
} // namespace uc_xbe_signed
