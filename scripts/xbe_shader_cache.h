#pragma once
#include <cstdint>
#include <string>
#include <d3dcommon.h>
namespace uc_xbe_shader_cache {
bool Load(const std::string& source, const char* entry, const char* profile,
          uint32_t flags, uint32_t effect_flags, ID3DBlob** code) noexcept;
void Save(const std::string& source, const char* entry, const char* profile,
          uint32_t flags, uint32_t effect_flags, ID3DBlob* code) noexcept;
}
