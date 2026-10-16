"""Private graphics settings and lossless scale changes at authored clear boundaries."""
from pathlib import Path


def once(text, old, new):
    assert text.count(old) == 1, old
    return text.replace(old, new, 1)


def between(text, begin, end):
    start = text.index(begin)
    return text[start:text.index(end, start)]


SAVE = r'''void Save() {
    auto target=File(),temp=target;
    temp+=L".tmp-"+std::to_wstring(GetCurrentProcessId());
    try {
        std::ofstream f(temp,std::ios::out|std::ios::trunc);
        f.exceptions(std::ios::badbit|std::ios::failbit);
        f << Table() << "\n";f.flush();f.close();
        if(!MoveFileExW(temp.c_str(),target.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))
            throw std::system_error(int(GetLastError()),std::system_category(),"saving settings");
    } catch(...) {
        std::error_code ignored;std::filesystem::remove(temp,ignored);throw;
    }
}'''


PAIRED = r'''
// A newly encountered attachment must match a retained companion. Otherwise a
// scale Apply followed by a partial clear would make BindTargets discard depth.
Target* D3D12Backend::EnsurePairedTarget(const Surface& s, bool depth, const Surface& peer) {
    if(Target* found=FindTarget(s,depth)) return found;
    Target* companion=FindTarget(peer,!depth);
    const float wanted=scale_;
    if(companion) {
        uint32_t gw=0,gh=0;SurfaceSize(s,gw,gh);
        if(gw==companion->gw&&gh==companion->gh&&gh)
            scale_=float(companion->h)/gh;
    }
    Target* created=EnsureTarget(s,depth);
    scale_=wanted;
    return created;
}
'''


RESIZE = r'''
// Existing GPU-only targets retain their contents until the title overwrites
// every plane. Resource allocation is transactional across the active pair.
void D3D12Backend::ResizeClearedTargets(const State& s, const ClearArgs& c, Target* color, Target* depth) {
    Target* old[2]={color,depth};
    const Surface* surfaces[2]={&s.rt,&s.zs};
    bool change[2]={},complete[2]={};
    for(int i=0;i<2;++i) if(old[i]) {
        auto& t=*old[i];float sc=scale_;auto fmt=t.fmt;
        if(ext_) ext_->MakeTarget(*surfaces[i],t.depth,t.gw,t.gh,sc,fmt);
        uint32_t w=std::max(1u,uint32_t(std::lround(t.gw*sc)));
        uint32_t h=std::max(1u,uint32_t(std::lround(t.gh*sc)));
        change[i]=t.w!=w||t.h!=h||t.fmt!=fmt;
        complete[i]=uc_target_resize::Complete(t,s.viewport,c);
    }
    if(!change[0]&&!change[1]) {
        if(color&&color->gw==guest_w_&&color->gh==guest_h_) trial_graphics::SetScalePending(false);
        return;
    }
    if(!uc_target_resize::Pair(change[0],complete[0],change[1],complete[1])) return;
    if((change[0]&&free_rtv_.empty())||(change[1]&&free_dsv_.empty())) return;
    Target replacements[2];
    for(int i=0;i<2;++i) if(change[i]) {
        // The opt-in fog pass may have a promoted alternate for this target.
        // SMOOTH_FOG_RELEASE
        replacements[i]=*old[i];replacements[i].fb.Reset();
        if(!CreateTargetResource(replacements[i],*surfaces[i])) return;
    }
    // View pools were checked before either clear command is recorded.
    for(int i=0;i<2;++i) if(change[i]) {
        bool made=CreateTargetView(replacements[i]);
        if(!made) {
            // Keep resources referenced by any already-recorded clear alive.
            for(int j=0;j<i;++j) if(change[j]) {
                CurrentFrame().garbage.push_back(replacements[j].res);
                (replacements[j].depth?free_dsv_:free_rtv_).push_back(replacements[j].view);
            }
            return;
        }
    }
    for(int i=0;i<2;++i) if(change[i]) {
        auto& t=*old[i];
        CurrentFrame().garbage.push_back(t.res);
        if(t.fb) CurrentFrame().garbage.push_back(t.fb);
        (t.depth?free_dsv_:free_rtv_).push_back(t.view);
        ReleaseDepthAlias(t);
        t=std::move(replacements[i]);
    }
    bound_rt_=bound_ds_=nullptr;
    if(color&&color->gw==guest_w_&&color->gh==guest_h_) trial_graphics::SetScalePending(false);
}
'''


COPY_ROUTE = r'''    if (uc_target_resize::DrawCopy(*src_t,*dst_t)) {
        // Differing formats or retained render scales require a scaled draw.
        CopyByDraw(*src_t, src_t->res.Get(), src_t->state, *dst_t, l);
        return;
    }
    CopyByRegion(*src_t, src_t->res.Get(), src_t->state, *dst_t, l);
'''


def prepare(settings, main, graphics, area):
    """After depth-alias and smooth-fog preparation; return three private sources."""
    area=Path(area);local=Path(__file__).parent
    for name in ('xbe_graphics_settings.h','xbe_graphics_settings.cpp','xbe_target_resize.h'):
        (area/'src/compat'/name).write_text((local/name).read_text())
    settings=once(settings,'#include "app/app.h"',
                  '#include "app/app.h"\n#include "xbe_graphics_settings.h"\n#include <system_error>')
    save=between(settings,'void Save() {','\nvoid LogSettings()')
    settings=settings.replace(save,SAVE+'\n',1)
    wide=between(settings,'bool Widescreen() {','\nfloat RenderScale(')
    settings=settings.replace(wide,'bool Widescreen() { return trial_graphics::Widescreen(); }\n',1)
    main=once(main,'#include "app/app.h"','#include "app/app.h"\n#include "xbe_graphics_settings.h"\n#include "xbe_trial_graphics_ui.h"')
    main=once(main,'    ImGui_ImplWin32_Init(app::g_hwnd);',
              '    TrialGraphicsFonts(game.dir);\n    ImGui_ImplWin32_Init(app::g_hwnd);')
    main=once(main,'    const Settings& s = Get();','    const Settings s = trial_graphics::Snapshot().settings;')
    main=once(main,'    Mode out = Output();','    Mode out = s.width && s.height ? Mode{s.width,s.height} : Desktop();')
    main=once(main,'Get().display_mode == DisplayMode::Fullscreen && Get().width && Get().height',
              'trial_graphics::FullscreenRequested()')
    graphics=once(graphics,'#include "app/app.h"',
                  '#include "app/app.h"\n#include "xbe_graphics_settings.h"\n#include "xbe_target_resize.h"')
    graphics=once(graphics,'    Target* EnsureTarget(const Surface& s, bool depth);',
                  '    Target* EnsureTarget(const Surface& s, bool depth);\n'
                  '    Target* EnsurePairedTarget(const Surface&, bool, const Surface&);')
    clear_decl='    void ClearTarget(Target& t, const State& s, const ClearArgs& c, std::vector<D3D12_RECT>& rs);'
    graphics=once(graphics,clear_decl,
                  clear_decl+'\n    void ResizeClearedTargets(const State&, const ClearArgs&, Target*, Target*);')
    resize=RESIZE
    if 'void D3D12Backend::ReleaseSmoothFog(' in graphics:
        resize=once(resize,'        // SMOOTH_FOG_RELEASE','        ReleaseSmoothFog(*old[i]);')
    else:
        resize=once(resize,'        // SMOOTH_FOG_RELEASE','')
    graphics=once(graphics,'bool D3D12Backend::BindTargets(const State& s) {',PAIRED+'\nbool D3D12Backend::BindTargets(const State& s) {')
    graphics=once(graphics,'    Target* rt = EnsureTarget(s.rt, false);',
                  '    Target* rt = EnsurePairedTarget(s.rt, false, s.zs);')
    graphics=once(graphics,'    Target* ds = s.rs[kRsZEnable] || s.rs[kRsStencilEnable] ? EnsureTarget(s.zs, true) : nullptr;',
                  '    Target* ds = s.rs[kRsZEnable] || s.rs[kRsStencilEnable] ? EnsurePairedTarget(s.zs, true, s.rt) : nullptr;')
    graphics=once(graphics,'    Target* rt = (flags & kClearTarget) ? EnsureTarget(s.rt, false) : nullptr;',
                  '    Target* rt = (flags & kClearTarget) ? EnsurePairedTarget(s.rt, false, s.zs) : nullptr;')
    graphics=once(graphics,'    Target* ds = (flags & (kClearZ | kClearStencil)) ? EnsureTarget(s.zs, true) : nullptr;',
                  '    Target* ds = (flags & (kClearZ | kClearStencil)) ? EnsurePairedTarget(s.zs, true, s.rt) : nullptr;')
    graphics=once(graphics,'void D3D12Backend::Clear(const State& s,',resize+'\nvoid D3D12Backend::Clear(const State& s,')
    graphics=once(graphics,
                  '    for (Target* t : {rt, ds}) {\n        if (t) ClearTarget(*t, s, c, rs);',
                  '''    // Include unchanged bound companions so a partial clear cannot detach depth.
    Target* color_target=rt ? rt : FindTarget(s.rt,false);
    Target* depth_target=ds ? ds : FindTarget(s.zs,true);
    ResizeClearedTargets(s,c,color_target,depth_target);
    for (Target* t : {rt, ds}) {
        if (t) ClearTarget(*t, s, c, rs);''')
    graphics=once(graphics,
                  '    if (std::fabs(want - scale_) > 1e-4f || remake) {',
                  '''    if (std::fabs(want - scale_) > 1e-4f) {
        scale_=want;
        trial_graphics::SetScalePending(!targets_.empty());
    }
    if (remake) {''')
    copy_begin=graphics.index('void D3D12Backend::Copy(const Surface&')
    copy_route=between(graphics[copy_begin:],'    if (src_t->fmt != dst_t->fmt) {','    if (cap_) {')
    graphics=once(graphics,copy_route,COPY_ROUTE)
    draw=between(graphics,'void D3D12Backend::CopyByDraw(', '\n// one CopyTextureRegion')
    scaled=once(draw,'    float src_scale = float(src_t.w) / src_t.gw;\n    float dst_scale = float(dst_t.w) / dst_t.gw;',
                '    const auto scale=uc_target_resize::DrawScale(src_t,dst_t);\n'
                '    float dst_scale=scale.x, dst_scale_y=scale.y;')
    scaled=once(scaled,'D3D12_VIEWPORT vp{r.dx * dst_scale, r.dy * dst_scale, (r.x1 - r.x0) * dst_scale, (r.y1 - r.y0) * dst_scale,',
                'D3D12_VIEWPORT vp{r.dx * dst_scale, r.dy * dst_scale_y, (r.x1 - r.x0) * dst_scale, (r.y1 - r.y0) * dst_scale_y,')
    scaled=once(scaled,'uint32_t taps = uint32_t(std::ceil(std::max(src_scale / dst_scale, 1.0f) - kTapsSlack));',
                'uint32_t taps = scale.taps;')
    graphics=once(graphics,draw,scaled)
    # Existing extension-requested remakes retain their explicit behavior; only
    # an ordinary output/internal-scale change takes the content-preserving path.
    fixture=(local/'test_xbe_graphics_settings.cpp').read_text()
    host=(local/'xbe_graphics_settings.cpp').read_text()
    start=host.index('uint32_t TrialAspectRatio')
    host=host[start:]
    display=between(host,'static bool DisplaySupported(', '\nbool Apply(')
    host=host.replace(display,'static bool DisplaySupported(const app::Settings&,std::string& message) {\n'
                      '    if(platform_supported)return true;message="unsupported display";return false;\n}\n',1)
    fixture=once(fixture,'// GENERATED_SETTINGS',host)
    fixture=once(fixture,'// GENERATED_SAVE',SAVE)
    (area/'src/compat/test_graphics_settings.cpp').write_text(fixture)
    fixture=(local/'test_xbe_target_resize.cpp').read_text()
    fixture=once(fixture,'// GENERATED_RESIZE',PAIRED+resize)
    fixture=once(fixture,'// GENERATED_COPY_ROUTE',COPY_ROUTE)
    (area/'src/compat/test_target_resize.cpp').write_text(fixture)
    return settings,main,graphics
