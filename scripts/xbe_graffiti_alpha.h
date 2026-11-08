#pragma once
#include "xbe_hd_ui_restored.h"
#include <string_view>

// Exact original mip identities also qualify their optional HD replacements.
namespace uc_graffiti_alpha {
struct Entry { uint32_t width, height, format, bytes; const char* sha256; };
inline constexpr Entry kEntries[] = {
    {128, 128, 12, 10920, "7d101a8592c6cfe74a36c3f9ab6641b6afa2506ae9d08df54d9762dd0e2238ee"},
    {32, 64, 15, 2736, "8a6d35e44b071ab3feea6706e99f66646ec1b711d050254a6b6b8507b3200ede"},
    {128, 64, 15, 10928, "e74c63e5ac0ea908c7078260de43fcb4f4b926fd250800db8335e0250849b677"},
    {128, 64, 12, 5464, "52016c1191e393484a550b818d5b9c188c7360c1152aeb792c568425c2d1e91b"},
    {64, 64, 15, 5456, "fc90e6e378ae562693f0cf8caa9a0894a5273edcbf66bcf9c2676307dcc67457"},
    {128, 64, 15, 10928, "509e36e8dc19e50a825992e4ffb8fe8159e3a18a4c979e5a72558e197527ddbe"},
    {128, 128, 12, 10920, "77a344e98ecd24b4824076b5f2559191ce6d2f9cbf2320bd4cfd584f36fa59a0"},
    {128, 64, 15, 10928, "86abcc932a96f6ee563243b0a22f2fd44a3adb680bf135943f0e89a4ff4dbd40"},
    {128, 64, 15, 10928, "c5e070c3585f5fe4364066e8491df563056c96626af464dcc4e16c818daa2178"},
    {128, 128, 15, 21840, "a37c1d8baedf638f9745754c5651d8f6185b92078079c9f0e5d8e7b7a6d6eed0"},
    {128, 64, 15, 10928, "beafaf78a3c35a7f4692591bac222fd86b27fe9737bf1b9b720d732157c9cad8"},
    {256, 128, 15, 43696, "16e505b44307af7a6f0ac81ac392f6e2234556ded27e064245f85d5fe3e19da1"},
    {128, 128, 15, 21840, "1795e1f33faab159a76d7f0be0e41795d5742957f37cdd4f7239fe1baec63380"},
    {128, 128, 15, 21840, "086f3da9f3e24e8d24a102bf9b59cb0db7f90102ede919219e3f7ccf459b0f0f"},
    {64, 32, 12, 1368, "13b31d1289e84ef26e799d54a1443cc65ab1f16051f464c085650edf6206665f"},
    {128, 32, 12, 2744, "91850c5d09de755304d58e657d24fd73373903050625a1eb476bd03db31f005c"},
    {256, 128, 12, 21848, "7cf4c14deb10cfacc732512d71424ee4365e6f0a1bc957dde134098da29aef2b"},
    {256, 64, 15, 21872, "e890273083b1422dacf076bb31361cc6a9d122883c622876e0e66716961c395f"},
    {128, 64, 15, 10928, "7d72420a9015e4a35dabdfef5eb7d65ecc78b904e0b180c5e11ae657cf4b1d8a"},
    {256, 128, 15, 43696, "c029610f7ca4dba61ced14a48fb76fa1c42765995cf9970bdd1a6a3e186ff638"},
    {128, 32, 12, 2744, "988b7e9969eb1bca6a406693e04c19ec2a69f53d3728c8b2100838e790e7dc56"},
    {256, 64, 15, 21872, "bc232b76c82cc8f4c1e303fe16410409c6a22fbb0d0f7341a1e7446de5b0e79d"},
    {128, 64, 15, 10928, "68fb8112651341d2337eb1bb98bb6c2570a373ea22ad6271d1866d7810d2518f"},
    {128, 64, 15, 10928, "cd78632472bdd608a360e39aed33e85a7aaae165fe3441a7bfaa57b4c2895313"},
    {128, 128, 15, 21840, "5bdd997393594ed155e6bd518bc519d2b77fd05a6a515ce37295ed3cc326dd75"},
};

// Called only when guest texture contents change, never for each draw or mip.
inline bool Match(uint32_t width, uint32_t height, uint32_t format,
                  const uint8_t* data, uint32_t bytes) {
    if (!data || !bytes) return false;
    bool hashed = false;
    std::array<char, 64> hex{};
    for (const auto& entry : kEntries) {
        if (entry.width != width || entry.height != height ||
            entry.format != format || entry.bytes != bytes) continue;
        if (!hashed) {
            std::array<uint8_t, 32> digest{};
            if (!uc_hd_ui::restored::Digest({data, bytes}, digest)) return false;
            constexpr char digits[] = "0123456789abcdef";
            for (size_t i = 0; i < digest.size(); ++i) {
                hex[i * 2] = digits[digest[i] >> 4];
                hex[i * 2 + 1] = digits[digest[i] & 15];
            }
            hashed = true;
        }
        if (std::string_view(hex.data(), hex.size()) == entry.sha256) return true;
    }
    return false;
}

// Preserve authored blend operation, depth writes and alpha-test coverage.
inline bool Eligible(bool matched, int stage, uint32_t blend, uint32_t source,
                     uint32_t destination, uint32_t operation, uint32_t color_write) {
    if (!matched || stage != 0 || !blend || source != 0x302 ||
        !(color_write & 0x00010101)) return false;
    return (operation == 0x8006 && (destination == 0x303 || destination == 1)) ||
           (operation == 0x800b && destination == 1);
}
}
