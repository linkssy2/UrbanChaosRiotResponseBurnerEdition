"""Distance-fog visibility and opt-in precision at verified retail draw sites.

No global framebuffer format change: retain RGBA16 UNORM only from 91EC2's
fog blend to A2FEC's tone mapping, then return to the original target. The
previous alpha-only experiment failed because the intervening RGB8 blend
introduced replacement contours. See docs/WALL-BANDS-2026-10-10.md.
"""
from pathlib import Path
import hashlib
import struct
import re


STATE_HEADER = r'''#pragma once
#include "gpu/state.h"
#include <cstdint>
#include <cstring>

namespace gpu {
// ReadSpan validates/copies guest bytes on the title thread. The queued State
// owns values, never a pointer into a subsequently reused guest stack/object.
template<class ReadSpan>
void CaptureSmoothFogPass(State& s, uint32_t caller, uint32_t owner,
                         uint32_t prim, uint32_t count, uint32_t stride, ReadSpan read) {
    s.uc_fog_pass = 0;
    s.uc_fog_params_valid = 0;
    if (prim != 8 || count != 4) return;
    if (caller == 0xA2FEC && stride == 20) { s.uc_fog_pass = 2; return; }
    if (caller != 0x91EC2 || stride != 32) return;
    uint32_t p = 0, textures[2]{};
    // Identity is independent of the optional higher-precision curve. An
    // unreadable/unsupported curve must not silently defeat Distance fog Off.
    if (!read(owner, &p, sizeof(p)) || p > UINT32_MAX-0x27u ||
        !read(p+0x1c, textures, sizeof(textures)) ||
        textures[0] != s.textures[1] || textures[1] != s.textures[0]) return;
    s.uc_fog_pass = 1;
    s.uc_fog_params_valid = read(p, s.uc_fog_params, sizeof(s.uc_fog_params)) ? 1 : 0;
}
} // namespace gpu
'''


HEADER = r'''#pragma once
#include "gpu/ext.h"
#include "xbe_smooth_fog_state.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <string>
#include <cfenv>
#include <xmmintrin.h>

namespace gpu {
constexpr DXGI_FORMAT kSmoothFogFormat = DXGI_FORMAT_R16G16B16A16_UNORM;
constexpr uint64_t kSmoothToneKey = 0x5543464F47544F4Eull;
constexpr uint64_t kAnalyticFogKey = 0x5543464F475A3234ull;

inline bool FogPrecisionRequired(bool smooth, uint32_t mode) { return smooth || mode==1; }

// These fields belong to the GPU image, not the guest surface identity. In
// particular a feedback snapshot must change format with its source image.
inline void SwapSmoothFogImage(Target& a, Target& b) {
    using std::swap;
    swap(a.res,b.res); swap(a.view,b.view); swap(a.state,b.state);
    swap(a.fb,b.fb); swap(a.fb_state,b.fb_state); swap(a.fmt,b.fmt);
}

inline bool SmoothFogTarget(const State& s, const Target& t) {
    // 91C30 disables Z at 91D7E but retains the scene's depth-surface header.
    // Match BindTargets' active attachment policy, not the retained reference.
    return !t.depth && t.range == 1 && t.fmt == DXGI_FORMAT_B8G8R8A8_UNORM &&
        !s.rs[kRsZEnable] && !s.rs[kRsStencilEnable] && !s.scissor_count &&
        s.viewport.x == 0 && s.viewport.y == 0 &&
        s.viewport.w == t.gw && s.viewport.h == t.gh;
}
inline bool SmoothFogTextures(const TextureDesc& depth, const TextureDesc& table, const Target& t) {
    return depth.format == 0x40 && depth.linear && !depth.cube && depth.depth == 1 && depth.levels == 1 &&
        depth.width == t.gw && depth.height == t.gh && depth.pitch == t.gw * 4 &&
        table.format == 0x19 && !table.cube && table.depth == 1 && table.levels == 1 &&
        table.width == 256 && table.height == 256;
}
inline bool AnalyticFogPixels(const State& s,const Target& color,const Target& depth) {
    return SmoothFogTarget(s,color) && depth.w==color.w && depth.h==color.h;
}
inline bool SmoothToneTextures(const TextureDesc (&d)[4], const Target& t) {
    if (d[0].data != t.data || d[0].width != t.gw || d[0].height != t.gh) return false;
    for (unsigned i=0;i<4;++i) if (d[i].cube || d[i].depth != 1 || d[i].levels != 1) return false;
    return d[1].format == 6 && d[2].format == 6 && d[3].format == 6 &&
        d[1].width == 1 && d[1].height == 256 && d[2].width == 256 && d[2].height == 1 &&
        d[3].width == 1 && d[3].height == 256;
}

// Same-size exact conversion, including alpha. Load avoids the generic blit's
// filtering and alpha-forcing options at the two precision boundaries.
inline constexpr char kSmoothFogCopyHlsl[] = R"(
Texture2D image : register(t0);
float4 vs(uint id : SV_VertexID) : SV_Position {
    float2 uv=float2((id<<1)&2,id&2);
    return float4(uv*float2(2,-2)+float2(-1,1),0,1);
}
float4 psmain(float4 p : SV_Position) : SV_Target { return image.Load(int3(int2(p.xy),0)); }
)";

// Keep edge samples distinct when the precision image and its original share
// an MSAA count. Resolving here would lose coverage before later depth draws.
inline constexpr char kSmoothFogSampleCopyHlsl[] = R"(
Texture2DMS<float4> image : register(t0);
float4 vs(uint id : SV_VertexID) : SV_Position {
    float2 uv=float2((id<<1)&2,id&2);
    return float4(uv*float2(2,-2)+float2(-1,1),0,1);
}
float4 psmain(float4 p : SV_Position,uint sample : SV_SampleIndex) : SV_Target {
    return image.Load(int2(p.xy),sample);
}
)";

// Point sampling the original 256-entry tables quantizes the high precision
// fog again. Interpolate adjacent ORIGINAL bytes; byte inputs remain exact.
inline bool SmoothToneShader(std::string& source) {
    const char* old[] = {
        "float4 t1 = tex1.Sample(smp1, (t0.ar) + texsize[1].zw);",
        "float4 t2 = tex2.Sample(smp2, (t0.gb) + texsize[2].zw);",
        "float4 t3 = tex3.Sample(smp3, (t0.gb) + texsize[3].zw);"};
    const char* replacement[] = {
        "float uc_r=saturate(t0.r)*255; int uc_ri=(int)floor(uc_r); float4 t1=lerp(tex1.Load(int3(0,uc_ri,0)),tex1.Load(int3(0,min(uc_ri+1,255),0)),frac(uc_r));",
        "float uc_g=saturate(t0.g)*255; int uc_gi=(int)floor(uc_g); float4 t2=lerp(tex2.Load(int3(uc_gi,0,0)),tex2.Load(int3(min(uc_gi+1,255),0,0)),frac(uc_g));",
        "float uc_b=saturate(t0.b)*255; int uc_bi=(int)floor(uc_b); float4 t3=lerp(tex3.Load(int3(0,uc_bi,0)),tex3.Load(int3(0,min(uc_bi+1,255),0)),frac(uc_b));"};
    for (const auto* text : old) {
        auto at=source.find(text);
        if (at==std::string::npos || source.find(text,at+1)!=std::string::npos) return false;
    }
    for (unsigned i=0;i<3;++i) source.replace(source.find(old[i]),std::strlen(old[i]),replacement[i]);
    return true;
}

// The original dependent-GB fetch discards Z24's low byte. Moving fog to the
// final quarter of Westside's range leaves only about two dozen lookup entries.
// This separate mode consumes the actual full-resolution packed depth image.
// Its generated combiner still supplies the original exposure-adjusted colour.
inline constexpr char kAnalyticFogHlsl[] = R"(
float uc_distance_fog(float3 packed, float4 projection_range, float maximum) {
    uint3 bytes=(uint3)round(saturate(packed)*255.0);
    uint bits=bytes.x+(bytes.y<<8)+(bytes.z<<16);
    if(bits==0xffffffu) return maximum;
    float z=float(bits)/16777215.0;
    float distance=projection_range.y/(z-projection_range.x);
    return maximum*saturate((distance-projection_range.z)/(projection_range.w-projection_range.z));
}
)";
inline bool AnalyticFogShader(std::string& source) {
    const char* depth="float4 t0 = tex0.Sample(smp0, (tc0.xy / tc0.w) / texsize[0].xy + texsize[0].zw);";
    const char* table="float4 t1 = tex1.Sample(smp1, (t0.gb) + texsize[1].zw);";
    for(const auto* text:{depth,table}) {
        auto at=source.find(text);
        if(at==std::string::npos || source.find(text,at+1)!=std::string::npos) return false;
    }
    // These two originally-unused constant registers are private to this exact
    // composite. Reject a different combiner that actually reads vertex fog.
    const auto uses=[&](const char* name) {
        unsigned count=0;size_t at=0;
        while((at=source.find(name,at))!=std::string::npos){++count;at+=std::strlen(name);}return count;
    };
    if(uses("fogin")!=1 || uses("fogp")!=1 || uses("fogcolor")!=2) return false;
    source.replace(source.find(depth),std::strlen(depth),"float4 t0 = tex0.Load(int3(int2(i.pos.xy),0));");
    source.replace(source.find(table),std::strlen(table),
        "float4 t1 = float4(1,1,1,uc_distance_fog(t0.rgb,fogp,fogcolor.x));");
    source.insert(0,kAnalyticFogHlsl); return true;
}

// Optional visibility policy expressed in the scene's original world distances.
inline bool FogDistanceParams(const float original[5], uint32_t mode, float out[5]) {
    if (!original || !out || mode>1) return false;
    float p[5]; std::memcpy(p,original,sizeof(p));
    if (mode==1) {
        for (float v:p) if (!std::isfinite(v)) return false;
        // Retail perspective depth is z=m22+m32/distance. Keep the authored
        // far endpoint; move onset to its last quarter, not an inferred map edge.
        const float start=std::fabs(p[1])>p[2] ? std::fabs(p[1])+1.0f : p[2];
        if (!std::isfinite(start) || start<=0 || p[3]<=start) return false;
        if(p[0]<1 || p[1]>=0 || p[4]<0 || p[4]>1) return false;
        p[2]=float(double(start)+(double(p[3])-double(start))*.75);
        if (!(p[2]<p[3])) return false;
    }
    std::memcpy(out,p,sizeof(p)); return true;
}

template<class Constants>
bool AnalyticFogConstants(const State& s, Constants& c) {
    float p[5];
    if(s.uc_fog_pass!=1 || !s.uc_fog_params_valid || !FogDistanceParams(s.uc_fog_params,1,p)) return false;
    std::memcpy(c.fog,p,4*sizeof(float));c.fog_color[0]=p[4];return true;
}

template<class Lookup>
bool FogLookupMatches(const State& s, const Lookup& f, uint32_t mode, bool precise) {
    return f.distance_mode==mode && f.precise==precise &&
        !std::memcmp(f.params.data(),s.uc_fog_params,sizeof(s.uc_fog_params));
}

// Pre-truncation A6F00, including its sequential float accumulation and A7126
// subtraction. With precise=false, retain the original byte truncation exactly.
inline bool SmoothFogTable(const float p[5], uint16_t* rgba, bool precise=true) {
    if (!p || !rgba || std::fegetround()!=FE_TONEAREST || (_mm_getcsr()&0x6000)) return false;
    const float m22=p[0],m32=p[1],end=p[3],maximum=p[4]; float start=p[2];
    for (unsigned i=0;i<5;++i) if (!std::isfinite(p[i])) return false;
    if (maximum<0 || maximum>1 || m22<=0 || m32>=0) return false;
    start=std::fabs(m32)>start ? std::fabs(m32)+1.0f : start;
    if (!std::isfinite(start) || start<=0 || end<=start) return false;
    const float z0=float(double(m32)/double(start)+double(m22));
    volatile float z1=m32/end; z1=z1+m22;
    if (!std::isfinite(z0) || !std::isfinite(z1) || z0<0 || z1>1 || z1<z0) return false;
    uint32_t first=uint32_t(int64_t(double(z0)*65536.0-1.0));
    uint32_t last=uint32_t(int64_t(double(z1)*65536.0-1.0));
    first=std::clamp(first,1u,65535u); last=std::clamp(last,1u,65535u);
    if (last<first) return false;
    const uint16_t tail=uint16_t(uint32_t(double(maximum)*255.0)*257);
    for (uint32_t i=0;i<65536;++i) {
        rgba[i*4]=rgba[i*4+1]=rgba[i*4+2]=65535;
        rgba[i*4+3]=i<last?0:tail;
    }
    if (first<last) {
        const float step=float((double(z1)-double(z0))/double(last-first));
        volatile float scale=maximum/(end-start); scale=scale*256.0f;
        volatile float z=z0;
        for (uint32_t i=first;i<last;++i) {
            volatile float v=z-m22; v=m32/v; v=v-z0; v=v-start; v=v*scale;
            rgba[i*4+3]=!(v<255)?65535:v<=0?0:
                precise?uint16_t(double(v)*257.0+0.5):uint16_t(uint32_t(v)*257u);
            z=step+z;
        }
    }
    return true;
}
} // namespace gpu
'''


METHODS = r'''
// Keep a second GPU image only for targets that actually execute authored fog.
// Both resources/views survive frame fences through the ordinary garbage owner.
void D3D12Backend::ReleaseSmoothFog(Target& t) {
    auto it=smooth_fog_.find(&t);
    if (it==smooth_fog_.end()) return;
    auto& f=it->second;
    if (f.active) SwapSmoothFogImage(t,f.other);
    if (f.other.res) {
        free_rtv_.push_back(f.other.view);
        CurrentFrame().garbage.push_back(f.other.res);
    }
    if (f.other.fb) CurrentFrame().garbage.push_back(f.other.fb);
    if (f.lookup) CurrentFrame().garbage.push_back(f.lookup);
    smooth_fog_.erase(it);
    bound_rt_=bound_ds_=nullptr;
}

void D3D12Backend::CopySmoothFogImage(Target& src, Target& dst) {
    Transition(src.res.Get(),src.state,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    Transition(dst.res.Get(),dst.state,D3D12_RESOURCE_STATE_RENDER_TARGET);
    SetBlitTarget(Cpu(rtv_heap_.Get(),dst.view,rtv_inc_),dst.w,dst.h);
    uint32_t slot=AllocSrvSlots(1);
    D3D12_SHADER_RESOURCE_VIEW_DESC sv{};
    sv.Format=src.fmt; sv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
    sv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; sv.Texture2D.MipLevels=1;
    dev_->CreateShaderResourceView(src.res.Get(),&sv,Cpu(srv_heap_.Get(),slot,srv_inc_));
    Alloc cb=Upload(256,256);
    DrawFullScreen(EnsurePassPipeline(kSmoothFogCopyHlsl,"psmain",dst.fmt,-1),cb.gpu,slot);
    bound_rt_=bound_ds_=nullptr;
}

void D3D12Backend::EndSmoothFog(Target& t) {
    auto it=smooth_fog_.find(&t);
    if (it==smooth_fog_.end() || !it->second.active) return;
    auto& f=it->second;
    CopySmoothFogImage(t,f.other);
    SwapSmoothFogImage(t,f.other);
    f.active=false;
}

void D3D12Backend::EndAllSmoothFog() {
    for (auto& [target,f] : smooth_fog_) if (f.active) EndSmoothFog(*target);
}

bool D3D12Backend::SkipDistanceFog(const State& s) {
    // Apply runs on this render thread. Finish an already-active precision
    // chain before changing visibility, including a toggle between fog/tone.
    if (!FogPrecisionRequired(TrialSmoothFog,TrialFogDistanceMode) || !TrialDistanceFog) EndAllSmoothFog();
    // Only 91C30's qualified composite is omitted. Original guest cleanup,
    // depth copies, particles and the subsequent A2FEC tone pass still run.
    return !TrialDistanceFog && s.uc_fog_pass==1;
}

bool D3D12Backend::PrepareFogLookup(const State& s) {
    // F3-only evidence: ordinary play does not emit or format rejection logs.
    const auto reject=[&](const char* reason) {
        if(cap_ && s.uc_fog_pass==1)fprintf(cap_,
            "distance fog rejected=%s valid=%u zs=%08X z=%u stencil=%u scissor=%u viewport=%u,%u,%u,%u target=%08X fmt=%u range=%.9g guest=%ux%u host=%ux%u\n",
            reason,s.uc_fog_params_valid,s.zs.header,s.rs[kRsZEnable],s.rs[kRsStencilEnable],s.scissor_count,
            s.viewport.x,s.viewport.y,s.viewport.w,s.viewport.h,bound_rt_?bound_rt_->data:0,
            bound_rt_?unsigned(bound_rt_->fmt):0,bound_rt_?bound_rt_->range:0,
            bound_rt_?bound_rt_->gw:0,bound_rt_?bound_rt_->gh:0,bound_rt_?bound_rt_->w:0,bound_rt_?bound_rt_->h:0);
        return false;
    };
    if (s.uc_fog_pass!=1 || !s.uc_fog_params_valid) return reject("identity/curve");
    if (!bound_rt_ || !SmoothFogTarget(s,*bound_rt_)) return reject("target");
    Target& t=*bound_rt_;
    TextureDesc depth,table;
    if (!Describe(s.textures[0],depth) || !Describe(s.textures[1],table) || !SmoothFogTextures(depth,table,t)) return reject("textures");
    auto di=targets_.find(SizedTargetKey(depth.data,true,depth.width,depth.height));
    if (di==targets_.end() || !IsDepthColorAlias(depth,di->second)) return reject("depth-alias");
    // The analytic shader loads by physical pixel, never filtering packed bytes.
    if(!AnalyticFogPixels(s,t,di->second)) return reject("physical-pixels");
    // Exact colour-blend contract at 91EC2, not an arbitrary use of the same LUT.
    if (!s.simple[kMBlendEnable>>2] || s.simple[kMSrcBlend>>2]!=0x302 ||
        s.simple[kMDestBlend>>2]!=0x303 || BlendOp(s.simple[kMBlendOp>>2])!=D3D12_BLEND_OP_ADD ||
        s.simple[kMColorWriteEnable>>2]!=0x10101) return reject("blend");
    float params[5];
    if (!FogDistanceParams(s.uc_fog_params,TrialFogDistanceMode,params)) return reject("parameters");
    auto& f=smooth_fog_[&t];
    if(TrialFogDistanceMode==1) {
        // No table build/upload in the new distance mode. Retain any prior
        // Original lookup until its normal owner retires/replaces it.
        std::memcpy(f.params.data(),s.uc_fog_params,sizeof(s.uc_fog_params));
        f.distance_mode=1;f.precise=true;return true;
    }
    if (!f.lookup || !FogLookupMatches(s,f,TrialFogDistanceMode,TrialSmoothFog)) {
        std::vector<uint16_t> pixels(65536*4);
        if (!SmoothFogTable(params,pixels.data(),TrialSmoothFog)) return false;
        D3D12_HEAP_PROPERTIES hp{D3D12_HEAP_TYPE_DEFAULT};
        auto td=Texture2DDesc(kSmoothFogFormat,256,256);
        ComPtr<ID3D12Resource> lookup;
        if (FAILED(dev_->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&td,D3D12_RESOURCE_STATE_COPY_DEST,
                                                 nullptr,IID_PPV_ARGS(&lookup)))) return false;
        Alloc upload=Upload(pixels.size()*sizeof(uint16_t),D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
        std::memcpy(upload.cpu,pixels.data(),pixels.size()*sizeof(uint16_t));
        D3D12_TEXTURE_COPY_LOCATION dst{lookup.Get(),D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX};
        D3D12_TEXTURE_COPY_LOCATION src{upload.res,D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT};
        src.PlacedFootprint.Offset=upload.off;
        src.PlacedFootprint.Footprint={kSmoothFogFormat,256,256,1,256*8};
        cl_->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
        auto state=D3D12_RESOURCE_STATE_COPY_DEST;
        Transition(lookup.Get(),state,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        if (f.lookup) CurrentFrame().garbage.push_back(f.lookup);
        f.lookup=std::move(lookup);
        std::memcpy(f.params.data(),s.uc_fog_params,sizeof(s.uc_fog_params));
        f.distance_mode=TrialFogDistanceMode; f.precise=TrialSmoothFog;
    }
    return true;
}

bool D3D12Backend::BeginSmoothFog(const State& s) {
    if (!bound_rt_ || s.uc_fog_pass!=1 || !s.uc_fog_params_valid) return false;
    Target& t=*bound_rt_;
    auto it=smooth_fog_.find(&t);
    if (it==smooth_fog_.end() || (TrialFogDistanceMode!=1 && !it->second.lookup)) return false;
    auto& f=it->second;
    // Compile both directions before changing ownership: failure leaves ordinary rendering intact.
    if (!EnsurePassPipeline(kSmoothFogCopyHlsl,"psmain",kSmoothFogFormat,-1) ||
        !EnsurePassPipeline(kSmoothFogCopyHlsl,"psmain",t.fmt,-1)) return false;
    if (!f.other.res) {
        if (free_rtv_.empty()) return false;
        D3D12_HEAP_PROPERTIES hp{D3D12_HEAP_TYPE_DEFAULT};
        auto td=Texture2DDesc(kSmoothFogFormat,t.w,t.h); td.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        if (FAILED(dev_->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&td,D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                 nullptr,IID_PPV_ARGS(&f.other.res)))) return false;
        f.other.view=free_rtv_.back(); free_rtv_.pop_back();
        f.other.fmt=kSmoothFogFormat; f.other.state=D3D12_RESOURCE_STATE_RENDER_TARGET;
        f.other.w=t.w; f.other.h=t.h;
        dev_->CreateRenderTargetView(f.other.res.Get(),nullptr,Cpu(rtv_heap_.Get(),f.other.view,rtv_inc_));
    }
    CopySmoothFogImage(t,f.other);
    SwapSmoothFogImage(t,f.other);
    f.active=true;
    if (cap_) fprintf(cap_,"smooth fog begin %08X %ux%u\n",t.data,t.w,t.h);
    return true;
}

bool D3D12Backend::SmoothToneDraw(const State& s) {
    if (s.uc_fog_pass!=2 || !bound_rt_) return false;
    auto it=smooth_fog_.find(bound_rt_);
    if (it==smooth_fog_.end() || !it->second.active) return false;
    TextureDesc d[4];
    for (int i=0;i<4;++i) if (!Describe(s.textures[i],d[i])) return false;
    return SmoothToneTextures(d,*bound_rt_);
}

void D3D12Backend::SmoothFogLookup(const State& s, DrawTextures& stages) {
    if(TrialFogDistanceMode==1)return;
    if (s.uc_fog_pass!=1 || !s.uc_fog_params_valid || !bound_rt_) return;
    auto it=smooth_fog_.find(bound_rt_);
    if (it==smooth_fog_.end() || (!it->second.active && TrialFogDistanceMode!=1) ||
        !FogLookupMatches(s,it->second,TrialFogDistanceMode,TrialSmoothFog)) return;
    stages.res[1]=it->second.lookup.Get();
    stages.sv[1].Format=kSmoothFogFormat;
}
'''


def once(source, old, new):
    assert source.count(old) == 1, old
    return source.replace(old, new, 1)


def patch(source):
    source = once(source, '#include "xbe_depth_alias.h"',
                  '#include "xbe_depth_alias.h"\n#include "xbe_smooth_fog.h"\nextern bool TrialSmoothFog;\nextern bool TrialDistanceFog;\nextern uint32_t TrialFogDistanceMode;')
    source = once(source, '    void ReleaseDepthAlias(Target& t);', '''    void ReleaseDepthAlias(Target& t);
    void ReleaseSmoothFog(Target& t);
    void CopySmoothFogImage(Target& src, Target& dst);
    void EndSmoothFog(Target& t);
    void EndAllSmoothFog();
    bool SkipDistanceFog(const State& s);
    bool PrepareFogLookup(const State& s);
    bool BeginSmoothFog(const State& s);
    bool SmoothToneDraw(const State& s);
    void SmoothFogLookup(const State& s, DrawTextures& stages);''')
    source = once(source, '    std::unordered_map<ID3D12Resource*, DepthAlias> depth_aliases_;', '''    std::unordered_map<ID3D12Resource*, DepthAlias> depth_aliases_;
    struct SmoothFog {
        Target other; // same-size image; swapped with the live target while active
        bool active=false;
        ComPtr<ID3D12Resource> lookup;
        std::array<float,5> params{};
        uint32_t distance_mode=0;
        bool precise=true;
    };
    std::unordered_map<Target*,SmoothFog> smooth_fog_;''')
    source = once(source, 'void D3D12Backend::ReleaseTarget(std::unordered_map<uint64_t, Target>::iterator it) {',
                  'void D3D12Backend::ReleaseTarget(std::unordered_map<uint64_t, Target>::iterator it) {\n    ReleaseSmoothFog(it->second);')
    source = once(source, '        for (auto& [key, tg] : targets_) (tg.depth ? free_dsv_ : free_rtv_).push_back(tg.view);',
                  '        for (auto& [key, tg] : targets_) ReleaseSmoothFog(tg);\n        for (auto& [key, tg] : targets_) (tg.depth ? free_dsv_ : free_rtv_).push_back(tg.view);')
    for anchor in ['void D3D12Backend::UploadTarget(Target& t, const Surface& s) {',
                   'void D3D12Backend::ClearTarget(Target& t, const State& s, const ClearArgs& c, std::vector<D3D12_RECT>& rs) {']:
        source = once(source, anchor, anchor + '\n    EndSmoothFog(t);')
    source = once(source, 'void D3D12Backend::Present(const Surface& front) {',
                  'void D3D12Backend::Present(const Surface& front) {\n    EndAllSmoothFog(); // a missing/disabled tone pass cannot leak the temporary format')
    source = once(source, '    if (DepthCopy(s)) return;',
                  '    if (SkipDistanceFog(s)) return;\n    if (DepthCopy(s)) return;')
    anchor = '    if (ext_ && ext_->Bound(*this, s, bound_rt_, bound_ds_, a.pretransformed) && !BindTargets(s)) return;'
    source = once(source, anchor, anchor + '''
    bool fog_lookup=FogPrecisionRequired(TrialSmoothFog,TrialFogDistanceMode) && PrepareFogLookup(s);
    bool analytic_fog=fog_lookup && TrialFogDistanceMode==1;
    // The analytic variant qualifies its shader before promoting below.
    if (fog_lookup && !analytic_fog && BeginSmoothFog(s) && !BindTargets(s)) return;
    bool smooth_tone=SmoothToneDraw(s);
    Target* tone_target=smooth_tone?bound_rt_:nullptr;''')
    source = once(source, '    PixelKey pk = MakePixelKey(s, stages.info);',
                  '    if (fog_lookup) SmoothFogLookup(s,stages);\n    PixelKey pk = MakePixelKey(s, stages.info);')
    source = once(source, '    sh.pkey = Hash(&pk, sizeof(pk));\n    EnsurePixelShader(pk, sh);', '''    sh.pkey = Hash(&pk, sizeof(pk));
    if (smooth_tone) {
        auto check=PixelHlsl(pk);
        smooth_tone=SmoothToneShader(check);
        if (smooth_tone) sh.pkey ^= kSmoothToneKey;
    }
    if(analytic_fog) {
        auto check=PixelHlsl(pk);analytic_fog=AnalyticFogShader(check);
        if(!analytic_fog && cap_)fprintf(cap_,"distance fog rejected=shader\\n");
        // Failed qualification/allocation leaves the original table and target
        // intact. Never run the new mode into an unpromoted RGB8 intermediate.
        if(analytic_fog) {
            analytic_fog=BeginSmoothFog(s);
            if(!analytic_fog && cap_)fprintf(cap_,"distance fog rejected=promotion\\n");
        }
        if(analytic_fog) {
            if(!BindTargets(s))return;
            sh.pkey^=kAnalyticFogKey;
        }
    }
    EnsurePixelShader(pk, sh, smooth_tone, analytic_fog);''')
    source = source.replace('EnsurePixelShader(const PixelKey& pk, DrawShaders& sh)',
                            'EnsurePixelShader(const PixelKey& pk, DrawShaders& sh, bool smooth_tone, bool analytic_fog)')
    anchor = '    auto source = uc_xbe_lighting::Pixel(pk, PixelHlsl(pk), uc_xbe_lighting::Enabled());'
    source = once(source, anchor, anchor + '\n    if (smooth_tone) SmoothToneShader(source);\n    if (analytic_fog) AnalyticFogShader(source);')
    source = source.replace('CaptureShaders(const State& s, const Assembled& a, const DrawShaders& sh, const PixelKey& pk)',
                            'CaptureShaders(const State& s, const Assembled& a, const DrawShaders& sh, const PixelKey& pk, bool smooth_tone, bool analytic_fog)')
    source = once(source, '        CaptureShaders(s, a, sh, pk);',
                  '        CaptureShaders(s, a, sh, pk, smooth_tone, analytic_fog);')
    source = once(source, '    CaptureShader("ps", sh.pkey, PixelHlsl(pk));', '''    auto pixel=uc_xbe_lighting::Pixel(pk,PixelHlsl(pk),uc_xbe_lighting::Enabled());
    if(smooth_tone)SmoothToneShader(pixel);
    if(analytic_fog)AnalyticFogShader(pixel);
    CaptureShader("ps",sh.pkey,pixel);''')
    source, count = re.subn(r'(UploadConstants\([^;{]*?DrawConstants& c)\)',
                            r'\1, bool analytic_fog)', source)
    assert count == 2
    source = once(source, 'UploadConstants(s, sh, k, stages, size, c);',
                  'UploadConstants(s, sh, k, stages, size, c, analytic_fog);')
    source = once(source, '    std::memcpy(dc.cpu, &c, sizeof(c));',
                  '    if(analytic_fog)AnalyticFogConstants(s,c);\n    std::memcpy(dc.cpu, &c, sizeof(c));')
    source = once(source, '        CaptureDraw(s, prim, count, k, c, stages);', '''        if(s.uc_fog_pass==1)fprintf(cap_,"distance fog mode=%u analytic_z24=%u target16=%u params=%.9g,%.9g,%.9g,%.9g,%.9g\\n",
            TrialFogDistanceMode,unsigned(analytic_fog),unsigned(bound_rt_&&bound_rt_->fmt==kSmoothFogFormat),
            s.uc_fog_params[0],s.uc_fog_params[1],s.uc_fog_params[2],s.uc_fog_params[3],s.uc_fog_params[4]);
        CaptureDraw(s, prim, count, k, c, stages);''')
    source = once(source, '    EndCountedDraw(query);\n}', '''    EndCountedDraw(query);
    if (tone_target) {
        if (cap_) fprintf(cap_,"smooth fog tone %08X interpolation=%u\\n",tone_target->data,unsigned(smooth_tone));
        EndSmoothFog(*tone_target);
    }
}''')
    source = once(source, 'void D3D12Backend::InvalidateDepthAlias(Target& t) {',
                  METHODS + '\nvoid D3D12Backend::InvalidateDepthAlias(Target& t) {')
    return source


def prepare(sdk, area):
    """Run after depth_alias, before display/render-scale preparation."""
    area = Path(area)
    compat = area / 'src/compat'
    # The visibility option is qualified by the original distance-fog function,
    # its sole scene caller, and the separate tone-mapping draw site. No generic
    # fog render state or arbitrary fullscreen shader is disabled.
    xbe = (area / 'assets/Default.xbe').read_bytes()
    base = struct.unpack_from('<I', xbe, 0x104)[0]
    count, table = struct.unpack_from('<II', xbe, 0x11c)
    sections = [struct.unpack_from('<6I', xbe, table-base+i*56) for i in range(count)]
    def image(va, size):
        section = next(s for s in sections if s[1] <= va and va+size <= s[1]+s[4])
        at = section[3]+va-section[1]
        return xbe[at:at+size]
    assert hashlib.sha256(image(0x91c30, 777)).hexdigest() == '1c8be0578109aaae6e8c1a47018d57a734f2a40ad80bb53525b661bbf3710d6e'
    assert image(0x97b3a, 5) == bytes.fromhex('e8f1a0ffff')
    assert image(0xa2fe7, 5) == bytes.fromhex('e804c23400')
    state_path = area / 'src/gpu/state.h'
    state = state_path.read_text()
    state = once(state, '    float zscale = 16777215.0f;  // depth buffer units per 1.0', '''    float zscale = 16777215.0f;  // depth buffer units per 1.0
    // Trailing POD is part of render_thread.cpp StateRanges' material-to-end range.
    uint32_t uc_fog_pass = 0;
    uint32_t uc_fog_params_valid = 0;
    float uc_fog_params[5] = {};''')
    rt = (Path(sdk) / 'runtime/gpu/render_thread.cpp').read_text()
    assert '{At(s, &s.material), sizeof(State) - At(s, &s.material)}' in rt
    # Vertex shaders use these shared constants solely for an output that the
    # qualified fog combiner never consumes. They cannot affect its position.
    shaders = (compat / 'shaders.cpp').read_text()
    start = shaders.index('float fogfactor(float d) {')
    end = shaders.index('\n}', start) + 2
    assert 'fogp.' not in shaders[:start] + shaders[end:]
    for chunk, caller, stride in [(14, '91EC2', '20'), (16, 'A2FEC', '14')]:
        generated = (area / f'generated/game/x86_recomp.{chunk}.cpp').read_text()
        call = f'X86_PUSH32(0x{caller}u); sub_003EF1F0(ctx, base);'
        assert generated.count(call) == 1
        before = generated[:generated.index(call)][-6000:]
        assert f'uint32_t v = (uint32_t)(0x{stride}u); X86_PUSH32(v);' in before
    d3d8_path = compat / 'd3d8.cpp'
    d3d8 = once(d3d8_path.read_text(), '#include "gpu/backend.h"',
                '#include "gpu/backend.h"\n#include "xbe_smooth_fog_state.h"')
    anchor = '''    Walk(Dev(kDevPush));
    DrawUp(a[0], a[1], HostPtr(a[2]), a[3]);
    return 0;
}'''
    d3d8 = once(d3d8, anchor, '''    Walk(Dev(kDevPush));
    gpu::CaptureSmoothFogPass(g_state,TrialGuestReadable(ctx.esp,4)?R32(ctx.esp):0,ctx.esi,a[0],a[1],a[3],
        [](uint32_t p,void* out,uint32_t size) {
            if (!TrialGuestReadable(p,size)) return false;
            std::memcpy(out,HostPtr(p),size); return true;
        });
    DrawUp(a[0], a[1], HostPtr(a[2]), a[3]);
    g_state.uc_fog_pass=0;
    g_state.uc_fog_params_valid=0;
    return 0;
}''')
    d3d12_path = compat / 'd3d12.cpp'
    d3d12 = patch(d3d12_path.read_text())
    (compat / 'xbe_smooth_fog_state.h').write_text(STATE_HEADER)
    (compat / 'xbe_smooth_fog.h').write_text(HEADER)
    fixture = Path(__file__).with_name('test_xbe_smooth_fog.cpp').read_text()
    # Execute the actual retail draw owner with an existing depth attachment.
    # Its 91D7E/91ECD Z-enable calls bracket DrawUP; it never detaches that header.
    generated=(area/'generated/game/x86_recomp.14.cpp').read_text()
    start=generated.index('X86_FUNC(__imp__sub_00091C30) {')
    end=generated.index('\nX86_WEAK_FUNC(sub_00091C30,',start)
    original=generated[start:end].replace('__imp__sub_00091C30','OriginalFogDraw',1)
    fixture=once(fixture,'// ORIGINAL_FOG_DRAW',original)
    constant=struct.unpack('<I',image(0x44bf94,4))[0]
    fixture=once(fixture,'// ORIGINAL_FOG_CONSTANT',f'write(0x44bf94,0x{constant:08x});')
    ownership = ''
    for result, name in [('void', 'ReleaseSmoothFog'), ('void', 'EndSmoothFog'),
                         ('void', 'EndAllSmoothFog'), ('bool', 'SkipDistanceFog')]:
        start = METHODS.index(f'{result} D3D12Backend::{name}(')
        end = METHODS.index('\n}\n', start) + 3
        ownership += METHODS[start:end] + '\n'
    fixture = once(fixture, '// PRODUCTION_OWNERSHIP', ownership)
    (compat / 'test_smooth_fog.cpp').write_text(fixture)
    state_path.write_text(state)
    d3d8_path.write_text(d3d8)
    d3d12_path.write_text(d3d12)
