"""Bind the title's LIN_B8G8R8A8 view to live GPU depth, not stale guest bytes.

The 083305 capture's fog draw samples 83C6C000/format40 while the rendered
Z24S8 surface is 03C6C000. See native/environment_fog.h for the original byte
contract. This adapter preserves that shader and its dependent GB lookup.
"""
from pathlib import Path


HEADER = r'''#pragma once
#include "gpu/ext.h"

namespace gpu {
// Only the authored, same-sized linear Z24S8-as-colour view is qualified.
inline bool IsDepthColorAlias(const TextureDesc& d, const Target& t) {
    return d.format == 0x40 && d.linear && !d.cube && d.depth == 1 && d.levels == 1 &&
           t.depth && d.data == t.data && d.width == t.gw && d.height == t.gh &&
           d.pitch == t.gw * 4;
}

inline D3D12_SHADER_RESOURCE_VIEW_DESC DepthAliasPlane(bool stencil) {
    D3D12_SHADER_RESOURCE_VIEW_DESC sv{};
    sv.Format = stencil ? DXGI_FORMAT_X24_TYPELESS_G8_UINT : DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    sv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    sv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    sv.Texture2D.MipLevels = 1;
    sv.Texture2D.PlaneSlice = stencil ? 1 : 0;
    return sv;
}

inline constexpr char kDepthColorAliasHlsl[] = R"(
Texture2D<float> depth : register(t0);
Texture2D<uint2> stencil : register(t1);
float4 vs(uint id : SV_VertexID) : SV_Position {
    float2 uv = float2((id << 1) & 2, id & 2);
    return float4(uv * float2(2, -2) + float2(-1, 1), 0, 1);
}
float4 psmain(float4 pos : SV_Position) : SV_Target {
    int3 p = int3(int2(pos.xy), 0);
    uint z = (uint)round(saturate(depth.Load(p)) * 16777215.0);
    // Xbox memory is stencil, Zlow, Zmiddle, Zhigh; format40 maps it to A,R,G,B.
    return float4(z & 255, (z >> 8) & 255, z >> 16, stencil.Load(p).g & 255) / 255.0;
}
)";
} // namespace gpu
'''


METHODS = r'''
void D3D12Backend::InvalidateDepthAlias(Target& t) {
    auto it = depth_aliases_.find(t.res.Get());
    if (it != depth_aliases_.end()) it->second.dirty = true;
}

void D3D12Backend::ReleaseDepthAlias(Target& t) {
    auto it = depth_aliases_.find(t.res.Get());
    if (it == depth_aliases_.end()) return;
    free_rtv_.push_back(it->second.view);
    CurrentFrame().garbage.push_back(it->second.res);
    depth_aliases_.erase(it);
}

ID3D12Resource* D3D12Backend::BindDepthColorAlias(Target& t, int stage, DrawTextures& stages) {
    auto it = depth_aliases_.find(t.res.Get());
    if (it == depth_aliases_.end()) {
        if (free_rtv_.empty()) return nullptr;
        DepthAlias alias;
        D3D12_HEAP_PROPERTIES hp{D3D12_HEAP_TYPE_DEFAULT};
        D3D12_RESOURCE_DESC td = Texture2DDesc(kBgra, t.w, t.h);
        td.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        if (FAILED(dev_->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &td, alias.state,
                                                 nullptr, IID_PPV_ARGS(&alias.res)))) return nullptr;
        alias.view = free_rtv_.back();
        free_rtv_.pop_back();
        dev_->CreateRenderTargetView(alias.res.Get(), nullptr, Cpu(rtv_heap_.Get(), alias.view, rtv_inc_));
        it = depth_aliases_.emplace(t.res.Get(), std::move(alias)).first;
    }
    DepthAlias& alias = it->second;
    if (alias.dirty) {
        auto* pso = EnsurePassPipeline(kDepthColorAliasHlsl, "psmain", kBgra, -1);
        if (!pso) return nullptr;
        // Same command queue, no CPU readback or fence. A later write invalidates this snapshot.
        Transition(t.res.Get(), t.state, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        Transition(alias.res.Get(), alias.state, D3D12_RESOURCE_STATE_RENDER_TARGET);
        SetBlitTarget(Cpu(rtv_heap_.Get(), alias.view, rtv_inc_), t.w, t.h);
        uint32_t slot = AllocSrvSlots(2);
        for (uint32_t plane = 0; plane != 2; ++plane) {
            auto sv = DepthAliasPlane(plane != 0);
            dev_->CreateShaderResourceView(t.res.Get(), &sv, Cpu(srv_heap_.Get(), slot + plane, srv_inc_));
        }
        Alloc cb = Upload(256, 256);
        DrawFullScreen(pso, cb.gpu, slot);
        alias.dirty = false;
        if (cap_) fprintf(cap_, "%04u depth colour alias %08X %ux%u\n", cap_i_++, t.data, t.w, t.h);
    }
    Transition(alias.res.Get(), alias.state, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    target_used_[t.data] = frame_;
    stages.rt[stage] = true;
    stages.aligned_depth_alias[stage] = true;
    stages.info[stage].kind = kTex2D;
    stages.info[stage].shadow = 0;
    stages.sv[stage].Format = kBgra;
    return alias.res.Get();
}
'''


def once(source, old, new):
    assert source.count(old) == 1, old
    return source.replace(old, new, 1)


def patch(source):
    source = once(source, '#include "gpu/ext.h"',
                  '#include "gpu/ext.h"\n#include "xbe_depth_alias.h"')
    source = once(source, '    bool DepthCopy(const State& s);', '''    bool DepthCopy(const State& s);
    void InvalidateDepthAlias(Target& t);
    void ReleaseDepthAlias(Target& t);
    ID3D12Resource* BindDepthColorAlias(Target& t, int stage, DrawTextures& stages);''')
    source = once(source, '    bool rt[4] = {};',
                  '    bool aligned_depth_alias[4] = {};\n    bool rt[4] = {};')
    source = once(source, '    if (!stages.rt[i]) return;', '''    // 91C30's fog quad has identical zero-origin positions and texcoords, already at host pixel centres.
    // Offsetting packed depth would blend unrelated bytes across Z24 carries at fractional render scales.
    if (!stages.rt[i] || stages.aligned_depth_alias[i]) return;''')
    source = once(source, '    std::unordered_map<uint64_t, Target> targets_;', '''    std::unordered_map<uint64_t, Target> targets_;
    struct DepthAlias {
        ComPtr<ID3D12Resource> res;
        uint32_t view = 0;
        D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_RENDER_TARGET;
        bool dirty = true;
    };
    std::unordered_map<ID3D12Resource*, DepthAlias> depth_aliases_;''')
    source = once(source, '    targets_.erase(it);',
                  '    ReleaseDepthAlias(it->second);\n    targets_.erase(it);')
    source = once(source, '        targets_.clear();',
                  '        for (auto& [key, tg] : targets_) ReleaseDepthAlias(tg);\n        targets_.clear();')
    source = once(source, '    DrawGeometry(a);\n    EndCountedDraw(query);', '''    DrawGeometry(a);
    if (bound_ds_) InvalidateDepthAlias(*bound_ds_);
    EndCountedDraw(query);''')
    source = once(source, '    dst.drawn = true;\n    bound_rt_ = bound_ds_ = nullptr;',
                  '    dst.drawn = true;\n    InvalidateDepthAlias(dst);\n    bound_rt_ = bound_ds_ = nullptr;')
    source = once(source, '    if (t.depth) {\n        Transition(t.res.Get(), t.state, D3D12_RESOURCE_STATE_DEPTH_WRITE);',
                  '    if (t.depth) {\n        InvalidateDepthAlias(t);\n        Transition(t.res.Get(), t.state, D3D12_RESOURCE_STATE_DEPTH_WRITE);')
    source = once(source, '    return BindGuestTexture(s, stage, d, fi, stages);', '''    if (d.format == 0x40) {
        auto depth = targets_.find(SizedTargetKey(d.data, true, d.width, d.height));
        if (depth != targets_.end() && IsDepthColorAlias(d, depth->second))
            return BindDepthColorAlias(depth->second, stage, stages);
    }
    return BindGuestTexture(s, stage, d, fi, stages);''')
    source = once(source, 'ID3D12Resource* D3D12Backend::BindTexture(',
                  METHODS + '\nID3D12Resource* D3D12Backend::BindTexture(')
    return source


def prepare(sdk, area):
    """After graphics compatibility preparation; changes only its private copy."""
    del sdk
    compat = Path(area) / 'src/compat'
    path = compat / 'd3d12.cpp'
    source = patch(path.read_text())
    # The production shader and descriptor helper also drive the independent GPU fixture.
    (compat / 'xbe_depth_alias.h').write_text(HEADER)
    fixture = Path(__file__).with_name('test_xbe_depth_alias.cpp').read_text()
    start = source.index('struct DrawTextures {')
    stages = source[start:source.index('\n};', start) + 3]
    start = source.index('void D3D12Backend::FillTexelConstants(')
    texel = source[start:source.index('\nvoid D3D12Backend::FillTextureConstants(', start)]
    texel = texel.replace('D3D12Backend::', '')
    start = METHODS.index('    stages.rt[stage] = true;')
    metadata = METHODS[start:METHODS.index('    return alias.res.Get();', start)]
    fixture = once(fixture, '// PRODUCTION_TEXEL_CONSTANTS',
        'namespace gpu::fixture {\n' + stages +
        '\nconstexpr float kGlowBoxTexels = 3.0f;\nconstexpr DXGI_FORMAT kBgra = DXGI_FORMAT_B8G8R8A8_UNORM;\n' + texel +
        '\nvoid SetAliasMetadata(int stage, DrawTextures& stages) {\n' + metadata + '}\n}\n')
    (compat / 'test_depth_alias.cpp').write_text(fixture)
    path.write_text(source)
