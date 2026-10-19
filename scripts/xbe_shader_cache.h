#pragma once
#include <cstdint>
#include <string>
#include <functional>
#include <d3dcommon.h>
#include <d3d12.h>
namespace uc_xbe_shader_cache {
using WarmSink=std::function<void(const std::string&,const std::string&,const std::string&,ID3DBlob*)>;
void Warm(const WarmSink& sink) noexcept;
bool Load(const std::string& source, const char* entry, const char* profile,
          uint32_t flags, uint32_t effect_flags, ID3DBlob** code) noexcept;
void Save(const std::string& source, const char* entry, const char* profile,
          uint32_t flags, uint32_t effect_flags, ID3DBlob* code) noexcept;
HRESULT CreatePipeline(ID3D12Device* device,const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc,
                       ID3D12PipelineState** result) noexcept;
void FlushWrites(); // Offline qualification/shutdown only; never call from a draw.
}
