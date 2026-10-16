#pragma once
// Exact authored combiner math only. The new renderer continues to own texture
// fetches, shadow comparison, color selection, alpha testing, and output state.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <bcrypt.h>
#include <cstring>
#include <cstdlib>
#include <string>
#include "gpu/shaders.h"
#include "native_lighting_pixel_source.h"

namespace uc_xbe_lighting {

inline bool Enabled() {
    // Process-scoped: changing this inside a session would require shader-cache
    // invalidation. It is an isolated opt-in until the new-backend proof passes.
    static const bool enabled = [] {
        const char* p = std::getenv("UCRR_XBE_NATIVE_LIGHTING_PIXEL");
        return p && !std::strcmp(p, "1");
    }();
    return enabled;
}

inline bool SpecialInput(uint32_t inputs, unsigned bytes = 4) {
    for (unsigned i = 0; i < bytes; ++i, inputs >>= 8)
        if ((inputs & 15) >= 14) return true;
    return false;
}

inline const UcNativeLightPixel* Qualify(const gpu::PixelKey& k) {
    const unsigned stages = k.count & 255;
    if (!k.programmable || stages > 8 || gpu::g_shader_hooks) return nullptr;
    // Exact signatures include the verified final SUM/EF families. The private
    // generic compiler also needs the per-operand unsigned SUM correction.
    uint32_t key[39] = {stages, k.count >> 8, k.final_efg & 255, 0, 0};
    unsigned count = 5;
    for (unsigned i = 0; i < 4; ++i) {
        unsigned mode = (k.tex_modes >> (i * 5)) & 31;
        if (mode > 3 && mode != 15 && mode != 16) return nullptr;
        unsigned source = i < 2 ? 0 : (k.input_texture >> (i == 2 ? 16 : 20)) & 15;
        if ((mode == 15 || mode == 16) && (!i || source >= i)) return nullptr;
        key[3] |= (mode == 0 ? 0 : mode == 15 ? 2 : mode == 16 ? 3 : 1) << (i * 2);
        if (mode == 15 || mode == 16) key[4] |= source << (i * 4);
    }
    for (unsigned i = 0; i < stages; ++i) {
        if (SpecialInput(k.rgb_in[i]) || SpecialInput(k.a_in[i])) return nullptr;
        key[count++] = k.rgb_in[i]; key[count++] = k.a_in[i];
        key[count++] = k.rgb_out[i] & 0xfffff; key[count++] = k.a_out[i] & 0xfffff;
    }
    key[count++] = k.final_abcd; key[count++] = k.final_efg >> 8;
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    unsigned char digest[32];
    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0) return nullptr;
    NTSTATUS status = BCryptHash(algorithm, nullptr, 0, reinterpret_cast<PUCHAR>(key), count * 4,
                                 digest, sizeof(digest));
    BCryptCloseAlgorithmProvider(algorithm, 0);
    if (status < 0) return nullptr;
    for (const auto& entry : uc_native_light_pixels)
        if (entry.stages == stages && !std::memcmp(entry.digest, digest, sizeof(digest))) return &entry;
    return nullptr;
}

inline void Replace(std::string& text, const std::string& from, const std::string& to) {
    size_t at = 0;
    while ((at = text.find(from, at)) != std::string::npos) {
        text.replace(at, from.size(), to); at += to.size();
    }
}

inline std::string Pixel(const gpu::PixelKey& k, std::string source, bool enabled) {
    if (!enabled) return source;
    const auto* entry = Qualify(k);
    if (!entry) return source;
    constexpr const char* begin_marker = "float4 r0 = float4(0, 0, 0, ";
    constexpr const char* end_marker = "alphatest(outc.a);\n";
    const size_t begin = source.find(begin_marker), end = source.find(end_marker);
    if (begin == std::string::npos || end == std::string::npos || end <= begin ||
        source.find(begin_marker, begin + 1) != std::string::npos ||
        source.find(end_marker, end + 1) != std::string::npos) return source;
    std::string body = entry->source;
    Replace(body, "input.color0", "v0"); Replace(body, "input.color1", "v1");
    Replace(body, "input.fog", "i.fogamt"); Replace(body, "fog_color", "fogcolor");
    Replace(body, "fc0", "pc[16]"); Replace(body, "fc1", "pc[17]");
    for (unsigned i = 0; i < 8; ++i) {
        Replace(body, "c0[" + std::to_string(i) + "]", "pc[" + std::to_string((k.count & 0x1000) ? i * 2 : 0) + "]");
        Replace(body, "c1[" + std::to_string(i) + "]", "pc[" + std::to_string((k.count & 0x10000) ? i * 2 + 1 : 1) + "]");
    }
    source.replace(begin, end - begin, "// UC native lighting combiner: " + std::string(entry->name) +
                   "\n" + body + "float4 outc = result;\n");
    return source;
}
} // namespace uc_xbe_lighting
