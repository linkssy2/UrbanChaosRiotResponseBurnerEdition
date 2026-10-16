#pragma once
#include <cstdint>
#include <string>
#include <functional>
#include <d3dcommon.h>
namespace uc_xbe_shader_cache {
using WarmSink=std::function<void(const std::string&,const std::string&,const std::string&,ID3DBlob*)>;
void Warm(const WarmSink& sink) noexcept;
bool Load(const std::string& source, const char* entry, const char* profile,
          uint32_t flags, uint32_t effect_flags, ID3DBlob** code) noexcept;
void Save(const std::string& source, const char* entry, const char* profile,
          uint32_t flags, uint32_t effect_flags, ID3DBlob* code) noexcept;
}
