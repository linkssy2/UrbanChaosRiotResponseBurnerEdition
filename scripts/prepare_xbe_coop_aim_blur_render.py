"""Keep retail depth-of-field source/filter/composite within one co-op camera."""
from pathlib import Path
import hashlib
import struct
from prepare_xbe_perf import once, function

INCLUDE = '#include "xbe_coop_aim_blur_render.h"\n'
SCOPE = '\tTrialCoopAimBlurRenderScope trial_coop_aim_blur_render(ctx,base);\n'
HOOKS = {
    14: ((0x923CC, 'TrialCoopAimBlurDownsample'), (0x927B9, 'TrialCoopAimBlurComposite')),
    17: ((0xA83A1, 'TrialCoopAimBlurVertical'),),
}
PINS = (
    (14,0x920b0,1986,'13e52d62b15fd1412fe37a8e53e0ec7f45ec05e1fc6e59d4ce2d8708566f9374',
     '462caf935d2dc54c0a487465a175ecdf0ea258e5ba72352e7b1ad5b121dfc340'),
    (17,0xa7710,3276,'9b3e910d84b1f37cb5777d3bd1a0d475f96d1157fdf2dfbb2bf76beb48342d34',
     '86b5a769137dbe645db2327a6de3b3930d15a12141c4f2f6923114330d71f379'),
)

SAMPLE = r'''#pragma once
#include <cstdint>
#include <cstring>
#include <string>

namespace gpu {
constexpr uint64_t kCoopAimBlurKey=0x554341494d424c52ull;
inline bool CoopAimBlurBounds(unsigned view,unsigned width,unsigned height,float out[4]) {
    if(view>1 || !width || height<2)return false;
    const unsigned split=height/2,first=view?split:0,last=view?height:split;
    out[0]=.5f/width;out[1]=(float(first)+.5f)/height;
    out[2]=(float(width)-.5f)/width;out[3]=(float(last)-.5f)/height;
    return true;
}
inline bool CoopAimBlurShader(std::string& source) {
    const char* sample="float4 t0 = tex0.Sample(smp0, (tc0.xy / tc0.w) / texsize[0].xy + texsize[0].zw);";
    const char* wide="float4 t0 = spreadtap(tex0, smp0, (tc0.xy / tc0.w) / texsize[0].xy + texsize[0].zw, spread[0], ";
    const char* wide_tap="t.SampleLevel(s, uv + w.xy * (float2(x, y) - (n - 1) * 0.5), 0)";
    const char* tail="    float4 lt[56];\n};";
    const auto at=source.find(sample),wide_at=source.find(wide),end=source.find(tail);
    if((at==std::string::npos)==(wide_at==std::string::npos) || end==std::string::npos ||
       (at!=std::string::npos && source.find(sample,at+1)!=std::string::npos) ||
       (wide_at!=std::string::npos && source.find(wide,wide_at+1)!=std::string::npos) ||
       source.find(tail,end+1)!=std::string::npos || source.find("coop_blur_crop")!=std::string::npos)return false;
    std::string filter;
    if(wide_at!=std::string::npos) {
        const auto first=source.find("float4 spreadtap(Texture2D ");
        if(first==std::string::npos)return false;
        const auto last=source.find("\n}",first);
        if(last==std::string::npos)return false;
        filter=source.substr(first,last+2-first);
        const auto tap=filter.find(wide_tap);
        if(tap==std::string::npos || filter.find(wide_tap,tap+1)!=std::string::npos)return false;
        filter.replace(tap,std::strlen(wide_tap),
            "t.SampleLevel(s, clamp(uv + w.xy * (float2(x, y) - (n - 1) * 0.5), coop_blur_crop.xy, coop_blur_crop.zw), 0)");
        filter.replace(filter.find("spreadtap"),9,"coop_blur_spreadtap");
        source.replace(wide_at+std::strlen("float4 t0 = "),9,"coop_blur_spreadtap");
        source.insert(source.find("float4 spreadtap(Texture2D "),filter+"\n");
    }else source.replace(at,std::strlen(sample),
        "float4 t0 = tex0.Sample(smp0, clamp((tc0.xy / tc0.w) / texsize[0].xy + texsize[0].zw, coop_blur_crop.xy, coop_blur_crop.zw));");
    // Existing shaders keep identical source/cache keys and their cbuffer
    // prefix. Only this exact sampling variant consumes the appended member.
    source.replace(source.find(tail),std::strlen(tail),"    float4 lt[56];\n    float4 coop_blur_crop;\n};");
    return true;
}
} // namespace gpu
'''


def unprepare_guest(area):
    for unit, hooks in HOOKS.items():
        path=Path(area)/f'generated/game/x86_recomp.{unit}.cpp'
        if not path.exists():
            continue
        source=path.read_text().replace(INCLUDE,'').replace(SCOPE,'')
        for _,name in hooks:
            source=source.replace(f'\t{name}(ctx,base);\n','')
        path.write_text(source)


def patch_backend(source):
    if '#include "xbe_coop_aim_blur_sample.h"' in source:
        return source
    source=once(source,'#include "xbe_smooth_fog.h"\n','#include "xbe_smooth_fog.h"\n#include "xbe_coop_aim_blur_sample.h"\n')
    # All variant identities, including qualification, enter the ordinary
    # shader/PSO caches. Owner bounds are dynamic, copied per queued draw.
    source=source.replace('bool smooth_tone, bool analytic_fog)', 'bool smooth_tone, bool analytic_fog, bool coop_aim_blur)')
    source=once(source,'    if (analytic_fog) AnalyticFogShader(source);',
                '    if (analytic_fog) AnalyticFogShader(source);\n    if (coop_aim_blur) CoopAimBlurShader(source);')
    source=once(source,'    if(analytic_fog)AnalyticFogShader(pixel);',
                '    if(analytic_fog)AnalyticFogShader(pixel);\n    if(coop_aim_blur)CoopAimBlurShader(pixel);')
    source=once(source,'    unsigned fog_bits=0;', '''    bool coop_aim_blur=s.uc_coop_blur_view>=1 && s.uc_coop_blur_view<=2 &&
        stages.res[0] && stages.rt[0] && stages.info[0].kind==kTex2D && stages.info[0].rect;
    unsigned fog_bits=0;''')
    source=once(source,'if(smooth_tone || analytic_fog)fog_bits=', 'if(smooth_tone || analytic_fog || coop_aim_blur)fog_bits=')
    source=once(source,'        return bits|(AnalyticFogShader(check)?2u:0u);', '''        bits|=AnalyticFogShader(check)?2u:0u;
        check=PixelHlsl(pk);
        return bits|(CoopAimBlurShader(check)?4u:0u);''')
    source=once(source,'    EnsurePixelShader(pk, sh, smooth_tone, analytic_fog);', '''    coop_aim_blur=coop_aim_blur && (fog_bits&4)!=0;
    if(coop_aim_blur)sh.pkey^=kCoopAimBlurKey;
    EnsurePixelShader(pk, sh, smooth_tone, analytic_fog, coop_aim_blur);''')
    source=once(source,'        CaptureShaders(s, a, sh, pk, smooth_tone, analytic_fog);',
                '        CaptureShaders(s, a, sh, pk, smooth_tone, analytic_fog, coop_aim_blur);')
    # Fill after the ordinary constants; actual retained resource extent is
    # authoritative while a render-scale Apply waits for the next full clear.
    source=once(source,'    if(analytic_fog)AnalyticFogConstants(s,c);', '''    if(analytic_fog)AnalyticFogConstants(s,c);
    if(s.uc_coop_blur_view>=1 && s.uc_coop_blur_view<=2 && stages.res[0]) {
        const auto source=stages.res[0]->GetDesc();
        CoopAimBlurBounds(s.uc_coop_blur_view-1,unsigned(source.Width),source.Height,c.coop_blur_crop);
    }''')
    return source


def prepare(sdk,area):
    area=Path(area);sdk=Path(sdk);local=Path(__file__).parent
    unprepare_guest(area)
    xbe=(area/'assets/Default.xbe').read_bytes()
    base=struct.unpack_from('<I',xbe,0x104)[0]
    count,table=struct.unpack_from('<II',xbe,0x11c)
    sections=[struct.unpack_from('<6I',xbe,table-base+i*56) for i in range(count)]
    for unit,va,size,digest,translated in PINS:
        section=next(s for s in sections if s[1]<=va and va+size<=s[1]+s[4])
        offset=section[3]+va-section[1]
        assert hashlib.sha256(xbe[offset:offset+size]).hexdigest()==digest,hex(va)
        source=(area/f'generated/game/x86_recomp.{unit}.cpp').read_text()
        start=source.index(f'X86_FUNC(__imp__sub_{va:08X}) {{\n')
        end=source.index(f'\nX86_WEAK_FUNC(sub_{va:08X}',start)
        assert hashlib.sha256(source[start:end].encode()).hexdigest()==translated,hex(va)
    originals={}
    for unit,hooks in HOOKS.items():
        path=area/f'generated/game/x86_recomp.{unit}.cpp'
        source=path.read_text();originals[unit]=source
        for address,name in hooks:
            marker=f'\t// {address:08X} call '
            assert source.count(marker)==1,marker
            source=source.replace(marker,f'\t{name}(ctx,base);\n'+marker,1)
        if unit==14:
            source=once(source,'X86_FUNC(__imp__sub_000920B0) {\n','X86_FUNC(__imp__sub_000920B0) {\n'+SCOPE)
        source=once(source,'#include "x86_recomp_shared.h"\n','#include "x86_recomp_shared.h"\n'+INCLUDE)
        path.write_text(source)
    for name in ('xbe_coop_aim_blur_render.h','xbe_coop_aim_blur_render.cpp'):
        (area/'src/common'/name).write_text((local/name).read_text())
    compat=area/'src/compat'
    (compat/'xbe_coop_aim_blur_sample.h').write_text(SAMPLE)
    state=area/'src/gpu/state.h'
    source=state.read_text()
    if 'uc_coop_blur_view' not in source:
        source=once(source,'    uint32_t uc_fog_pass = 0;', '    uint32_t uc_coop_blur_view = 0; // queued crop owner + 1, only retail 923D1\n    uint32_t uc_fog_pass = 0;')
        state.write_text(source)
    shader_header=area/'src/gpu/shaders.h'
    source=(shader_header if shader_header.exists() else sdk/'runtime/gpu/shaders.h').read_text()
    if 'coop_blur_crop' not in source:
        marker='    float lt[8][7][4];'
        start=source.index(marker);end=source.index('\n',start)
        source=source[:end]+'\n    float coop_blur_crop[4]; // private appended constants for the isolated source-copy variant'+source[end:]
    shader_header.write_text(source)
    bridge=compat/'d3d8.cpp';source=bridge.read_text()
    if INCLUDE not in source:
        source=once(source,'#include "xbe_smooth_fog_state.h"\n','#include "xbe_smooth_fog_state.h"\n'+INCLUDE)
        source=once(source,'    DrawUp(a[0], a[1], HostPtr(a[2]), a[3]);', '''    g_state.uc_coop_blur_view=a[0]==6 && a[1]==4 && a[3]==20 ?
        TrialCoopAimBlurSourceView(TrialGuestReadable(ctx.esp,4)?R32(ctx.esp):0) : 0;
    DrawUp(a[0], a[1], HostPtr(a[2]), a[3]);
    g_state.uc_coop_blur_view=0;''')
    source=source.replace('    g_state.uc_coop_blur_view=0;\n','')
    if 'ResetCoopAimBlurView' not in source:
        source=once(source,'    g_state.uc_coop_blur_view=a[0]==6',
            '    struct ResetCoopAimBlurView { uint32_t& value; ~ResetCoopAimBlurView(){value=0;} } reset_coop_blur{g_state.uc_coop_blur_view};\n'
            '    g_state.uc_coop_blur_view=a[0]==6')
    bridge.write_text(source)
    backend=compat/'d3d12.cpp';source=patch_backend(backend.read_text());backend.write_text(source)
    gpu_fixture=local/'test_xbe_coop_aim_blur_render_gpu.cpp'
    if gpu_fixture.exists():
        textures=source[source.index('struct DrawTextures {'):source.index('\n};',source.index('struct DrawTextures {'))+3]
        fill=function(source,'void D3D12Backend::FillTexelConstants(').replace('D3D12Backend::','')
        # Use the final maintained constant mapper, including the later dirty-state patch.
        from prepare_xbe_simple_state_costs import rewrite_writes, TEXTURE_WRITES
        texture=rewrite_writes((compat/'texture.cpp').read_text(),TEXTURE_WRITES,(1,1,1,1))
        feedback='\n'.join(function(text,signature) for text,signature in (
            (texture,'uint32_t PackColor('),(texture,'void ApplyPixelConstants('),
            (source,'void Color('),(source,'uint32_t SimpleState('),(source,'void FillCombinerConstants(')))
        for address,size,digest in ((0x4f8550,16,'92f8e4ae71e30802d288bb46ee3369521f1a5ac5fd75816e40a31f22f56971ee'),
                                    (0x4f6cc0,240,'506eacb1ebc775bd5b21d147195daba354abaadbc033688c2efa8a5adc0f55d7')):
            section=next(s for s in sections if s[1]<=address and address+size<=s[1]+s[4])
            offset=section[3]+address-section[1];data=xbe[offset:offset+size]
            assert hashlib.sha256(data).hexdigest()==digest,hex(address)
            if address==0x4f8550:assert struct.unpack('<4I',data)==(0x4f6cc0,1,0x51a238,0)
            else:feedback+='\nconstexpr uint32_t kFeedbackShader[]={'+','.join(f'0x{v:x}u' for v in struct.unpack('<60I',data))+'};\n'
        fixture=gpu_fixture.read_text()
        fixture=once(fixture,'// PRODUCTION_COOP_AIM_BLUR_RENDER_GPU',textures+'\nconstexpr float kGlowBoxTexels=3.0f;\n'+fill+'\n'+feedback)
        (compat/'test_coop_aim_blur_render_gpu.cpp').write_text(fixture)
    # Run the original emitted quad builders in the contract fixture, not a
    # second implementation of their topology, UV packing or stack offsets.
    fixture=(local/'test_xbe_coop_aim_blur_render.cpp').read_text()
    blocks=[]
    for name,first,last in (('OriginalDownsample',0x921A4,0x923CC),('OriginalComposite',0x9241C,0x927B9)):
        text=originals[14];start=text.index(f'\t// {first:08X} ');stop=text.index(f'\t// {last:08X} ',start)
        blocks.append(f'X86_FUNC({name}) {{\n\tX86_LOAD_FLAGS();\n\tuint32_t ca=0,cb=0;\n'+text[start:stop]+'\n}\n')
    fixture=once(fixture,'// ORIGINAL_AIM_BLUR_QUADS','\n'.join(blocks))
    (compat/'test_coop_aim_blur_render.cpp').write_text(fixture)
