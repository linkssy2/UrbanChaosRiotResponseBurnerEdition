"""Adapt the verified Urban Chaos back-buffer scale entry to queued D3D12 presentation."""
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
AREA=ROOT/'build/xbe-full-integration-20261014'
SDK=ROOT/'build/upstream-review-xboxxbe-20261014'
def once(s,a,b):
 assert s.count(a)==1,a
 return s.replace(a,b,1)
def prepare():
 state=(SDK/'runtime/gpu/state.h').read_text()
 state=once(state,'struct VertexShader {','struct VertexShader {\n    bool immediate_pretransformed = false;')
 state=once(state,'uint32_t data = 0, format = 0, size = 0;','uint32_t data = 0, format = 0, size = 0;\n    float present_x = 1, present_y = 1; // owned by the queued Present command')
 (AREA/'src/gpu').mkdir(exist_ok=True)
 (AREA/'src/gpu/state.h').write_text(state)
 d=(SDK/'runtime/xdk/d3d8.cpp').read_text()
 d=once(d,'#include "xdk/hle.h"','#include "xdk/hle.h"\n#include "native/control_model_receiver.h"\n#include "scripts/xbe_input_memory.h"')
 d=once(d,'#include "scripts/xbe_input_memory.h"','''#include "scripts/xbe_input_memory.h"
// The scanner missed this title's stdcall SetPalette; use the existing host owner.
#undef SYM_D3DDevice_SetPalette
#define SYM_D3DDevice_SetPalette 003E39F0
#undef HAS_D3DDevice_SetPalette
#define HAS_D3DDevice_SetPalette 1''')
 # Reuse the existing qualified decoder without linking its unrelated skinning helpers.
 receiver=(ROOT/'native/control_model_receiver.c').read_text()
 receiver=receiver[:receiver.index('int uc_control_model_vertex(')]
 receiver=once(receiver,'#include "control_model_receiver.h"','#include "native/control_model_receiver.h"')
 receiver=once(receiver,'#include "point_light_vertex.h"\n','')
 (AREA/'src/compat/control_indexed.c').write_text(receiver)
 oracle=(AREA/'generated/game/x86_recomp.101.cpp').read_text()
 start=oracle.index('X86_FUNC(__imp__sub_0038590D) {')
 end=oracle.index('\nX86_WEAK_FUNC(sub_0038590D',start)
 (AREA/'src/compat/original_xg_compile.inc').write_text(oracle[start:end])
 d=once(d,'// drawing','// drawing\n'+(ROOT/'scripts/xbe_recorded_draws.inc').read_text())
 d=once(d,'#include <algorithm>','#include <algorithm>\n#include <cmath>\n#define SYM_D3DDevice_SetBackBufferScale 003E5230')
 d=once(d,'gpu::State g_state;','gpu::State g_state;\nfloat g_present_x = 1, g_present_y = 1;')
 d=once(d,'    g_state = gpu::State{};','    g_state = gpu::State{};\n    g_present_x = g_present_y = 1;')
 d=once(d,'if (!(handle & 1) && (handle & 0xE) == 4) LOG_FIRST(1, "d3d: pretransformed immediate mode vertices are not supported");','const bool screen = !(handle & 1) && (handle & 0xE) == 4;')
 d=once(d,'Made& m = made[{from.get(), g_im_mask}];','Made& m = made[{from.get(), g_im_mask | (screen ? 0x10000u : 0u)}];')
 d=once(d,'        m.vs = std::make_shared<gpu::VertexShader>();','        m.vs = std::make_shared<gpu::VertexShader>();\n        m.vs->immediate_pretransformed = screen;')
 d=once(d,'void Walk(uint32_t end) {','''void Walk(uint32_t end) {
    static unsigned traces=0;
    if(end != g_walk && traces++ < 16) LOGR("UC raw commands start=%08X end=%08X head=%08X",g_walk,end,g_walk ? R32(g_walk) : 0);''')
 d=once(d,'    auto next = clock::now();','    auto next = clock::now();\n    LOGR("UC vertical blank thread started");')
 d=once(d,'        if (uint32_t callback = Dev(XDK_CDevice_VBlankCallback)) {','''        if (count <= 4 || count == 60 || count == 180)
            LOGR("UC vblank count=%u callback=%08X owner=%08X", count, Dev(XDK_CDevice_VBlankCallback), R32(0x519F70));
        if (uint32_t callback = Dev(XDK_CDevice_VBlankCallback)) {''')
 d=once(d,'    if (g_backend) g_backend->Present(SurfaceOf(g_buffers[1]));','''    if (g_backend) {
        auto front = SurfaceOf(g_buffers[1]);
        front.present_x = g_present_x; front.present_y = g_present_y;
        g_backend->Present(front);
    }''')
 hook='''#define SYM_UrbanChaos_DeviceBusy 003E3A80
HLE_STDCALL(UrbanChaos_DeviceBusy, 0) {
    std::lock_guard<std::recursive_mutex> lock(g_lock);
    // shortcut: synchronous busy query; replace with nonblocking completion polling after boot parity.
    if (g_backend) g_backend->Flush();
    return 0;
}

// 003E5230 stores two floats at CDevice+96C/+970 and invokes NV2A scanout setup.
// Host presentation owns scanout; never enter the original hardware setup on a host device.
HLE_STDCALL(D3DDevice_SetBackBufferScale, 2) {
    std::lock_guard<std::recursive_mutex> lock(g_lock);
    float x = ArgF(a,0), y = ArgF(a,1);
    if (!std::isfinite(x) || !std::isfinite(y) || x <= 0 || y <= 0) return 0x80070057;
    g_present_x = x; g_present_y = y;
    float sx=x*GF(kDevice+kDevSuperX), sy=y*GF(kDevice+kDevSuperY);
    std::memcpy(HostPtr(kDevice+XDK_CDevice_BackBufferScale),&sx,4);
    std::memcpy(HostPtr(kDevice+XDK_CDevice_BackBufferScale+4),&sy,4);
    return 0;
}

'''
 d=once(d,'// targets and presentation',hook+'// targets and presentation')
 (AREA/'src/compat/d3d8.cpp').write_text(d)
 vertex=(SDK/'runtime/gpu/vertex.cpp').read_text()
 vertex=once(vertex,'return !(s.vs_handle & 1) && (s.vs_handle & 0xE) == 0x4;','return (s.vs && s.vs->immediate_pretransformed) || (!(s.vs_handle & 1) && (s.vs_handle & 0xE) == 0x4);')
 from prepare_xbe_vertex_costs import prepare as prepare_vertex_costs, packed_renderer
 vertex=prepare_vertex_costs(vertex,AREA,(SDK/'runtime/gpu/vertex.h').read_text())
 (AREA/'src/compat/vertex.cpp').write_text(vertex)
 d=(SDK/'runtime/gpu/d3d12.cpp').read_text()
 d=once(d,'bool UnitTexcoords(const Assembled& a) {','''bool UnitTexcoords(const State& s, const Assembled& a) {
    // Only wrapped U/V samplers use this vertex-dependent clamp decision.
    bool wrapped = false;
    for (const auto& t : s.tss)
        wrapped |= t[kTssAddressU] == kTaddressWrap || t[kTssAddressV] == kTaddressWrap;
    if (!wrapped) return false;''')
 d=once(d,'EnsureStageSamplers(s, stages, UnitTexcoords(a))','EnsureStageSamplers(s, stages, UnitTexcoords(s, a))')
 # F3-only state evidence; no per-frame logging in normal play.
 d=once(d,'    CaptureTextures(s, stages);','''    fprintf(cap_, " fog %u/%u rgba %08X params %.6g %.6g %.6g %.6g stencil %u func %X ref %X mask %X write %X ops %X/%X/%X",
        s.rs[kRsFogEnable], s.rs[kRsFogTableMode], s.rs[kRsFogColor],
        c.fog[0], c.fog[1], c.fog[2], c.fog[3], s.rs[kRsStencilEnable],
        SimpleState(s, kMStencilFunc), SimpleState(s, kMStencilRef), SimpleState(s, kMStencilMask), SimpleState(s, kMStencilWriteMask),
        s.rs[kRsStencilFail], SimpleState(s, kMStencilZFail), SimpleState(s, kMStencilPass));
    CaptureTextures(s, stages);''')
 d=once(d,'                res ? "" : " unbound");','''                res ? "" : " unbound");
        fprintf(cap_, " sign %08X hostfmt %u", s.tss[i][kTssColorSign], unsigned(stages.sv[i].Format));''')
 d=once(d,'    swap_->Present(vsync ? 1 : 0, !vsync && tearing_ ? DXGI_PRESENT_ALLOW_TEARING : 0);','''    HRESULT presented = swap_->Present(vsync ? 1 : 0, !vsync && tearing_ ? DXGI_PRESENT_ALLOW_TEARING : 0);
    if (presents_ < 3 || presents_ == 120) LOGR("UC present %llu result=%08X front=%08X size=%ux%u target=%u draws=%llu scale=%g,%g", (unsigned long long)presents_, unsigned(presented), front.header, bb_w_, bb_h_, t ? 1u : 0u, (unsigned long long)g_perf.draws, front.present_x, front.present_y);
    if (FAILED(presented)) LOGE("UC Present failed %08X", unsigned(presented));''')
 d=d.replace('DrawFrame(Target* t, uint32_t bi)','DrawFrame(Target* t, uint32_t bi, const Surface& front)')
 d=d.replace('BlitFrame(Target& t)','BlitFrame(Target& t, const Surface& front)')
 d=once(d,'if (t) BlitFrame(*t);','if (t) BlitFrame(*t, front);')
 d=once(d,'DrawFrame(t, bi);','DrawFrame(t, bi, front);')
 start=d.index('void D3D12Backend::BlitFrame(')
 end=d.index('void D3D12Backend::RenderUi()',start)
 d=d[:start]+once(d[start:end],'float uv[4] = {0, 0, 1, 1};','float uv[4] = {0, 0, front.present_x, front.present_y};')+d[end:]
 from prepare_xbe_native_lighting import patch_source, prepare_shaders
 d=patch_source(d)
 prepare_shaders(SDK/'runtime/gpu/shaders.cpp', AREA/'src/compat/shaders.cpp')
 (AREA/'src/compat/xbe_native_lighting.h').write_text((ROOT/'scripts/xbe_native_lighting.h').read_text())
 (AREA/'src/compat/native_lighting_pixel_source.h').write_text((ROOT/'src/native_lighting_pixel_source.h').read_text())
 (AREA/'src/compat/d3d12.cpp').write_text(packed_renderer(d))
if __name__=='__main__':prepare()
