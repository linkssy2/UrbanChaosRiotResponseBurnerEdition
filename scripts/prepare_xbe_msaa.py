"""Real 2x/4x primary scene MSAA, prepared after graphics settings/smooth fog.

Original guest surfaces and small shadow targets remain unchanged. Read images
resolve lazily; sample-count changes reuse the complete-clear transaction.
"""
from pathlib import Path


def once(s,old,new):
    assert s.count(old)==1,old
    return s.replace(old,new,1)


def function(s,name):
    start=s.index(name)
    start=s.rfind('\n',0,start)+1
    end=s.index('\n}',start)+2
    return s[start:end]


def patch(s):
    s=once(s,'#include "xbe_target_resize.h"','#include "xbe_target_resize.h"\n#include "xbe_msaa.h"')
    s=once(s,'    uint32_t rtfmt;','    uint32_t rtfmt;\n    uint32_t samples;')
    s=once(s,'    bool over = false;','    uint32_t samples = 1;\n    bool over = false;')
    s=once(s,'    std::unordered_map<uint64_t, Target> targets_;','''    std::unordered_map<uint64_t, Target> targets_;
    struct MsaaRead {
        ComPtr<ID3D12Resource> res;
        D3D12_RESOURCE_STATES state=D3D12_RESOURCE_STATE_RESOLVE_DEST;
        DXGI_FORMAT format=DXGI_FORMAT_UNKNOWN;
        uint32_t view=0;
        bool dirty=true,depth=false;
    };
    struct TargetRead { ID3D12Resource* res=nullptr;D3D12_RESOURCE_STATES* state=nullptr;DXGI_FORMAT format=DXGI_FORMAT_UNKNOWN; };
    std::unordered_map<ID3D12Resource*,MsaaRead> msaa_reads_;
    uint32_t msaa_support_=1u<<1,msaa_override_=0;
    uint32_t DesiredSamples(const Target&) const;
    TargetRead ReadTarget(Target&);
    void InvalidateMsaa(Target&);
    void ReleaseMsaa(ID3D12Resource*);
    bool CreateMatchedColor(Target&,uint32_t);
    bool MatchTargetSamples(Target&,const Target&);''')
    methods=Path(__file__).with_name('xbe_msaa_methods.inc').read_text()
    s=once(s,'void D3D12Backend::ReleaseTarget(',methods+'\nvoid D3D12Backend::ReleaseTarget(')
    s=once(s,'    LoadSettings();\n    Begin();','''    LoadSettings();
    msaa_support_=ext_?(1u<<1):uc_msaa::Support(dev_.Get());
    trial_graphics::PublishMsaaSupport(msaa_support_);
    Begin();''')
    part=function(s,'bool D3D12Backend::CreateTargetResource(')
    s=once(s,part,once(part,'    D3D12_CLEAR_VALUE cv{};','    td.SampleDesc.Count=DesiredSamples(t);\n    D3D12_CLEAR_VALUE cv{};'))
    s=once(s,'        dv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;',
           '        dv.ViewDimension = uc_msaa::Samples(&t)>1?D3D12_DSV_DIMENSION_TEXTURE2DMS:D3D12_DSV_DIMENSION_TEXTURE2D;')
    # A missing attachment inherits the retained companion, including during an
    # Apply before a complete paired clear. No mismatched RTV/DSV sample counts.
    s=once(s,'    const float wanted=scale_;','    const float wanted=scale_;\n    const auto previous_samples=msaa_override_;')
    s=once(s,'            scale_=float(companion->h)/gh;','''            { scale_=float(companion->h)/gh; msaa_override_=uc_msaa::Samples(companion); }''')
    s=once(s,'    scale_=wanted;\n    return created;','    scale_=wanted;msaa_override_=previous_samples;\n    return created;')
    s=once(s,'    if (rt && ds && (rt->w',
           '    if(rt&&ds&&(rt->w!=ds->w||rt->h!=ds->h||uc_msaa::Samples(rt)!=uc_msaa::Samples(ds))&&!EndSmoothFog(*rt))return false;\n'
           '    if(rt&&ds&&rt->w==ds->w&&rt->h==ds->h&&!MatchTargetSamples(*rt,*ds))return false;\n'
           '    if (rt && ds && (rt->w')
    s=once(s,'    k.rtfmt = bound_rt_ ? bound_rt_->fmt : 0;',
           '    k.rtfmt = bound_rt_ ? bound_rt_->fmt : 0;\n    k.samples=uc_msaa::Samples(bound_rt_?bound_rt_:bound_ds_);')
    part=function(s,'void D3D12Backend::FillPipelineDesc(')
    changed=once(part,'    d.SampleDesc.Count = 1;','    d.SampleDesc.Count = k.samples;\n    d.RasterizerState.MultisampleEnable=k.samples>1;')
    s=once(s,part,changed)
    # Every fullscreen destination explicitly states its sample count. Ordinary
    # swap-chain, lookup and postprocess targets keep the default of one.
    s=once(s,'EnsureBlitPipeline(DXGI_FORMAT rtv, bool over = false);','EnsureBlitPipeline(DXGI_FORMAT rtv, bool over = false, uint32_t samples = 1);')
    s=once(s,'FullScreenPipelineDesc(ID3DBlob* vs, ID3DBlob* ps, DXGI_FORMAT rtv) const;',
           'FullScreenPipelineDesc(ID3DBlob* vs, ID3DBlob* ps, DXGI_FORMAT rtv, uint32_t samples = 1) const;')
    s=once(s,'EnsurePassPipeline(const char* hlsl, const char* entry, DXGI_FORMAT rtv, float blend);',
           'EnsurePassPipeline(const char* hlsl, const char* entry, DXGI_FORMAT rtv, float blend, uint32_t samples = 1);')
    s=once(s,'                                                                        DXGI_FORMAT rtv) const {',
           '                                                                        DXGI_FORMAT rtv, uint32_t samples) const {')
    part=function(s,'D3D12_GRAPHICS_PIPELINE_STATE_DESC D3D12Backend::FullScreenPipelineDesc(')
    s=once(s,part,once(part,'    d.SampleDesc.Count = 1;','    d.SampleDesc.Count = samples;\n    d.RasterizerState.MultisampleEnable=samples>1;'))
    s=once(s,'D3D12Backend::EnsureBlitPipeline(DXGI_FORMAT rtv, bool over) {','D3D12Backend::EnsureBlitPipeline(DXGI_FORMAT rtv, bool over, uint32_t samples) {')
    s=once(s,'uint64_t(rtv) | uint64_t(over) << 32','uint64_t(rtv) | uint64_t(over) << 32 | uint64_t(samples) << 40')
    s=once(s,'                                                      float blend) {','                                                      float blend, uint32_t samples) {')
    s=once(s,'uint64_t seed = uint64_t(rtv) << 1 | (blend >= 0 ? 1 : 0);','uint64_t seed = uint64_t(rtv) << 1 | (blend >= 0 ? 1 : 0) | uint64_t(samples)<<40;')
    assert s.count('FullScreenPipelineDesc(vs, ps, rtv);')==2
    s=s.replace('FullScreenPipelineDesc(vs, ps, rtv);','FullScreenPipelineDesc(vs, ps, rtv, samples);')
    s=once(s,'EnsureBlitPipeline(o.rtv, o.over)','EnsureBlitPipeline(o.rtv, o.over, o.samples)')
    s=once(s,'.rtv = t.fmt, .over = over}', '.rtv = t.fmt, .samples = uc_msaa::Samples(&t), .over = over}')
    s=once(s,'.rtv = dst_t.fmt};','.rtv = dst_t.fmt, .samples = uc_msaa::Samples(&dst_t)};')
    # Resources read by translated Texture2D shaders are always single-sample.
    part=function(s,'ID3D12Resource* D3D12Backend::Feedback(')
    changed=once(part,'    if (!t.fb) {','    auto read=ReadTarget(t);if(!read.res)return nullptr;\n    if (!t.fb) {')
    changed=changed.replace('t.res->GetDesc()','read.res->GetDesc()').replace('Transition(t.res.Get(), t.state,','Transition(read.res, *read.state,').replace('t.fb.Get(), t.res.Get()','t.fb.Get(), read.res')
    s=once(s,part,changed)
    part=function(s,'ID3D12Resource* D3D12Backend::BindTargetTexture(')
    changed=once(part,'    Transition(t.res.Get(), t.state, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);',
                 '    auto read=ReadTarget(t);if(!read.res)return nullptr;\n    sv.Format=read.format;\n    Transition(read.res, *read.state, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);')
    changed=once(changed,'    return t.res.Get();','    return read.res;')
    s=once(s,part,changed)
    # Depth copies use the resolved float depth, then replicate into the actual
    # destination sample count. The pre-existing copy did not copy stencil.
    s=once(s,'    ID3D12PipelineState* EnsureDepthCopyPipeline();','    ID3D12PipelineState* EnsureDepthCopyPipeline(uint32_t samples = 1);')
    s=once(s,'D3D12Backend::EnsureDepthCopyPipeline() {','D3D12Backend::EnsureDepthCopyPipeline(uint32_t samples) {')
    s=once(s,'blit_psos_[kDepthCopyKey]','blit_psos_[kDepthCopyKey ^ uint64_t(samples)<<32]')
    part=function(s,'ID3D12PipelineState* D3D12Backend::EnsureDepthCopyPipeline(')
    s=once(s,part,once(part,'    p.SampleDesc.Count = 1;','    p.SampleDesc.Count = samples;\n    p.RasterizerState.MultisampleEnable=samples>1;'))
    s=once(s,'ID3D12PipelineState* pso = EnsureDepthCopyPipeline();','ID3D12PipelineState* pso = EnsureDepthCopyPipeline(uc_msaa::Samples(&dst));')
    part=function(s,'void D3D12Backend::DrawDepthCopy(')
    changed=once(part,'    Transition(src.res.Get(), src.state,','    auto read=ReadTarget(src);if(!read.res)return;\n    Transition(read.res, *read.state,')
    changed=once(changed,'sv.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;','sv.Format = read.format;')
    changed=once(changed,'CreateShaderResourceView(src.res.Get(),','CreateShaderResourceView(read.res,')
    changed=once(changed,'    dst.drawn = true;','    dst.drawn = true;\n    InvalidateMsaa(dst);')
    s=once(s,part,changed)
    # Packed fog aliases read the same sample's Z and stencil; the output is
    # ordinary BGRA for the retained dependent GB lookup.
    s=once(s,'auto* pso = EnsurePassPipeline(kDepthColorAliasHlsl, "psmain", kBgra, -1);',
           'auto* pso = EnsurePassPipeline(uc_msaa::Samples(&t)>1?uc_msaa::kDepthRead:kDepthColorAliasHlsl, uc_msaa::Samples(&t)>1?"packed":"psmain", kBgra, -1);')
    s=once(s,'auto sv = DepthAliasPlane(plane != 0);','auto sv = uc_msaa::Samples(&t)>1?uc_msaa::DepthView(plane!=0):DepthAliasPlane(plane != 0);')
    # Keep the scene's sample count throughout fog -> depth-tested transparents
    # -> tone. A depth rebind must not prematurely quantize the scene to RGB8.
    # Equal-count precision copies preserve each coverage sample; genuinely
    # changing counts still uses the existing resolve-and-broadcast path.
    part=function(s,'void D3D12Backend::CopySmoothFogImage(')
    changed=once(part,'    Transition(src.res.Get(),src.state,', '''    const uint32_t samples=uc_msaa::Samples(&dst);
    const bool per_sample=samples>1 && uc_msaa::Samples(&src)==samples;
    auto read=per_sample?TargetRead{src.res.Get(),&src.state,src.fmt}:ReadTarget(src);
    if(!read.res)return;
    Transition(read.res,*read.state,''')
    changed=once(changed,'sv.Format=src.fmt;','sv.Format=read.format;')
    changed=once(changed,'CreateShaderResourceView(src.res.Get(),','CreateShaderResourceView(read.res,')
    changed=once(changed,'sv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;',
                 'sv.ViewDimension=per_sample?D3D12_SRV_DIMENSION_TEXTURE2DMS:D3D12_SRV_DIMENSION_TEXTURE2D;')
    changed=once(changed,'sv.Texture2D.MipLevels=1;', 'if(!per_sample)sv.Texture2D.MipLevels=1;')
    changed=once(changed,'    bound_rt_=bound_ds_=nullptr;','    InvalidateMsaa(dst);\n    bound_rt_=bound_ds_=nullptr;')
    changed=changed.replace('void D3D12Backend::CopySmoothFogImage(', 'bool D3D12Backend::CopySmoothFogImage(')
    changed=once(changed,'if(!read.res)return;', 'if(!read.res)return false;')
    changed=once(changed,'    Transition(read.res,*read.state,',
                 '    auto* pso=EnsurePassPipeline(per_sample?kSmoothFogSampleCopyHlsl:kSmoothFogCopyHlsl,"psmain",dst.fmt,-1,samples);\n    if(!pso)return false;\n    Transition(read.res,*read.state,')
    changed=once(changed,'DrawFullScreen(EnsurePassPipeline(kSmoothFogCopyHlsl,"psmain",dst.fmt,-1),', 'DrawFullScreen(pso,')
    changed=once(changed,'    bound_rt_=bound_ds_=nullptr;', '    bound_rt_=bound_ds_=nullptr;\n    return true;')
    s=once(s,part,changed)
    s=once(s,'    void CopySmoothFogImage(Target& src, Target& dst);','    bool CopySmoothFogImage(Target& src, Target& dst);')
    s=once(s,'    void EndSmoothFog(Target& t);','    bool EndSmoothFog(Target& t);')
    part=function(s,'void D3D12Backend::EndSmoothFog(')
    changed=part.replace('void D3D12Backend::EndSmoothFog(', 'bool D3D12Backend::EndSmoothFog(')
    changed=once(changed,'return;', 'return true;')
    changed=once(changed,'    CopySmoothFogImage(t,f.other);','    if(!CopySmoothFogImage(t,f.other))return false;')
    changed=once(changed,'    f.active=false;', '    f.active=false;\n    return true;')
    s=once(s,part,changed)
    s=once(s,'    CopySmoothFogImage(t,f.other);','    if(!CopySmoothFogImage(t,f.other))return false;')
    for name in ('void D3D12Backend::UploadTarget(','void D3D12Backend::ClearTarget('):
        part=function(s,name)
        s=once(s,part,once(part,'    EndSmoothFog(t);','    if(!EndSmoothFog(t))return;'))
    s=once(s,'    // Compile both directions before changing ownership: failure leaves ordinary rendering intact.',
           '    const uint32_t samples=uc_msaa::Samples(&t);\n'
           '    const char* copy_shader=samples>1?kSmoothFogSampleCopyHlsl:kSmoothFogCopyHlsl;\n'
           '    // Compile both directions before changing ownership: failure leaves ordinary rendering intact.')
    s=once(s,'EnsurePassPipeline(kSmoothFogCopyHlsl,"psmain",kSmoothFogFormat,-1)',
           'EnsurePassPipeline(copy_shader,"psmain",kSmoothFogFormat,-1,samples)')
    s=once(s,'EnsurePassPipeline(kSmoothFogCopyHlsl,"psmain",t.fmt,-1)',
           'EnsurePassPipeline(copy_shader,"psmain",t.fmt,-1,samples)')
    s=once(s,'auto td=Texture2DDesc(kSmoothFogFormat,t.w,t.h); td.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;',
           'auto td=Texture2DDesc(kSmoothFogFormat,t.w,t.h); td.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET; td.SampleDesc.Count=samples;')
    # Color copies cannot CopyTextureRegion to multisampled destinations or
    # directly from a multisampled source. The draw path preserves rect/alpha.
    part=function(s,'void D3D12Backend::Copy(const Surface&')
    changed=once(part,'    CopyList l{','    auto read=ReadTarget(*src_t);if(!read.res)return;\n    CopyList l{')
    changed=once(changed,'uc_target_resize::DrawCopy(*src_t,*dst_t)','uc_target_resize::DrawCopy(*src_t,*dst_t) || uc_msaa::Samples(dst_t)>1')
    changed=changed.replace('src_t->res.Get(), src_t->state','read.res, *read.state')
    s=once(s,part,changed)
    # Present, synchronous/late CPU readback and target captures share the same
    # lazy read image; no read path can bind an MS resource as Texture2D.
    part=function(s,'void D3D12Backend::BlitFrame(')
    changed=once(part,'    // 16:9','    auto read=ReadTarget(t);if(!read.res)return;\n    // 16:9')
    changed=changed.replace('Transition(t.res.Get(), t.state,','Transition(read.res, *read.state,').replace('Blit(t.res.Get(), t.fmt,','Blit(read.res, read.format,')
    s=once(s,part,changed)
    part=function(s,'D3D12Backend::CopySource D3D12Backend::PrepareCopySource(')
    changed=once(part,'    CopySource out{t.res.Get(), nullptr};','    auto read=ReadTarget(t);if(!read.res)return {};\n    CopySource out{read.res, nullptr};')
    changed=changed.replace('Shrink(t.res.Get(), t.state, t.fmt,','Shrink(read.res, *read.state, read.format,').replace('Transition(t.res.Get(), t.state,','Transition(read.res, *read.state,')
    s=once(s,part,changed)
    for name in ('void D3D12Backend::CapSave(Target&','void D3D12Backend::Capture(Target&'):
        part=function(s,name)
        changed=once(part,'    CapSave({.res = t.res.Get(), .state = &t.state,',
                     '    auto read=ReadTarget(t);if(!read.res)return;\n    CapSave({.res = read.res, .state = read.state,')
        changed=changed.replace('.fmt = fmt,','.fmt = read.format,').replace('.fmt = t.fmt,','.fmt = read.format,')
        s=once(s,part,changed)
    # Invalidate exactly after GPU writes. Reads/rebinding alone keep the cache.
    s=once(s,'    DrawGeometry(a);','    DrawGeometry(a);\n    if(bound_rt_)InvalidateMsaa(*bound_rt_);\n    if(bound_ds_)InvalidateMsaa(*bound_ds_);')
    for name,target in (('void D3D12Backend::ClearTarget(','t'),('void D3D12Backend::CopyByDraw(','dst_t'),('void D3D12Backend::CopyByRegion(','dst_t')):
        part=function(s,name)
        changed=once(part,f'    {target}.drawn = true;',f'    {target}.drawn = true;\n    InvalidateMsaa({target});')
        s=once(s,part,changed)
    s=once(s,'    t.epoch = DirtyEpoch(t.data);\n    t.sig = MemSig(t.data, t.bytes);','    InvalidateMsaa(t);\n    t.epoch = DirtyEpoch(t.data);\n    t.sig = MemSig(t.data, t.bytes);')
    # All image-retirement sites release the cached resolve through the frame
    # garbage owner. Smooth-fog swaps themselves do not invalidate map identity.
    s=once(s,'    ReleaseDepthAlias(it->second);','    ReleaseDepthAlias(it->second);\n    ReleaseMsaa(it->second.res.Get());')
    s=once(s,'        free_rtv_.push_back(f.other.view);','        ReleaseMsaa(f.other.res.Get());\n        free_rtv_.push_back(f.other.view);')
    s=once(s,'        targets_.clear();','        for(auto& [key,tg]:targets_)ReleaseMsaa(tg.res.Get());\n        targets_.clear();')
    # The existing scale transaction also covers sample count. Finish a promoted
    # image before inspection, preserving it even if a replacement cannot fit.
    s=once(s,'    Target* old[2]={color,depth};','    if(color&&!EndSmoothFog(*color))return;\n    Target* old[2]={color,depth};')
    s=once(s,'change[i]=t.w!=w||t.h!=h||t.fmt!=fmt;','change[i]=t.w!=w||t.h!=h||t.fmt!=fmt||uc_msaa::Samples(&t)!=DesiredSamples(t);')
    s=once(s,'        ReleaseSmoothFog(*old[i]);','')
    s=once(s,'        auto& t=*old[i];\n        CurrentFrame().garbage.push_back(t.res);',
           '        auto& t=*old[i];\n        ReleaseSmoothFog(t);\n        ReleaseMsaa(t.res.Get());\n        CurrentFrame().garbage.push_back(t.res);')
    return s


def prepare(source,area):
    area=Path(area);local=Path(__file__).parent
    source=patch(source)
    (area/'src/compat/xbe_msaa.h').write_text((local/'xbe_msaa.h').read_text())
    fixture=(local/'test_xbe_msaa.cpp').read_text()
    start=source.index('    struct MsaaRead {')
    fields=source[start:source.index('    struct DepthAlias {',start)]
    fixture=once(fixture,'    // PRODUCTION_MSAA_FIELDS',fields)
    methods=(local/'xbe_msaa_methods.inc').read_text()
    methods=once(methods,'bool D3D12Backend::CreateMatchedColor(', 'bool D3D12Backend::CreateMatchedColorReal(')
    fixture=once(fixture,'// PRODUCTION_MSAA_METHODS',methods)
    names=('D3D12_RESOURCE_DESC BufferDesc(', 'D3D12_RESOURCE_DESC Texture2DDesc(')
    fixture=once(fixture,'// PRODUCTION_RESOURCE_HELPERS','\n'.join(function(source,n) for n in names))
    names=('void D3D12Backend::Transition(', 'void D3D12Backend::SetFullViewport(',
           'void D3D12Backend::SetBlitTarget(', 'void D3D12Backend::DrawFullScreen(',
           'D3D12_GRAPHICS_PIPELINE_STATE_DESC D3D12Backend::FullScreenPipelineDesc(',
           'bool D3D12Backend::CreateTargetResource(', 'bool D3D12Backend::CreateTargetView(',
           'ID3D12Resource* D3D12Backend::Feedback(', 'bool D3D12Backend::CopySmoothFogImage(',
           'bool D3D12Backend::BindTargets(', 'bool D3D12Backend::BeginSmoothFog(',
           'bool D3D12Backend::SmoothToneDraw(',
           'bool D3D12Backend::EndSmoothFog(', 'void D3D12Backend::EndAllSmoothFog(',
           'void D3D12Backend::ReleaseSmoothFog(', 'void D3D12Backend::ResizeClearedTargets(',
           'float D3D12Backend::SamplesPerPixel(')
    methods='\n'.join(function(source,n) for n in names)
    methods=once(methods,'bool D3D12Backend::CreateTargetResource(', 'bool D3D12Backend::CreateTargetResourceReal(')
    fixture=once(fixture,'// PRODUCTION_BACKEND_METHODS',methods)
    # Negative witness: the exact current body with the previous unconditional
    # precision-close seam restored, rather than a second hand-written binder.
    before=function(source,'bool D3D12Backend::BindTargets(')
    before=once(before,'bool D3D12Backend::BindTargets(', 'bool D3D12Backend::BindTargetsBefore(')
    before=once(before,'    if(rt&&ds&&(', '    if(rt&&ds&&!EndSmoothFog(*rt))return false;\n    if(rt&&ds&&(')
    fixture=once(fixture,'// PRODUCTION_BEFORE_BIND',before)
    (area/'src/compat/test_msaa.cpp').write_text(fixture)
    return source
