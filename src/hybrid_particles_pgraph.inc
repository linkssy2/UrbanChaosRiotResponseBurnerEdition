/* Ordinary prepared class4 particles; included in the real ordered PGRAPH owner. */
#include "hybrid_particles_guest.h"
#include "hybrid_particles_pgraph_contract.h"
#include "control_particle_bridge.h"
static struct {
    UCNativeControlParticles *renderer;
    unsigned initialized,disabled,shown;
    uint64_t calls,rejected,submitted,records,fatal;
    char error[512];
} hybrid_particles;
static void uc_hybrid_particles_shutdown(void){
    uc_native_control_particles_destroy(hybrid_particles.renderer);
    hybrid_particles.renderer=NULL;
}
static int uc_hybrid_particles_state(IDirect3DDevice8 *dev){
    const uint32_t *m=g_pg.trace_methods;float slope;int units;
    if(g_pg.cull_enable||!nv_depth_offset(m,8,g_pg.depth_test,&slope,&units)||slope||units||
       (g_pg.alpha_test&&(g_pg.alpha_func!=0x206||(g_pg.alpha_ref&255)!=1))||
       ((g_pg.tex[0].control0&4u)&&!g_pg.alpha_test))return 0;
    NvDrawStateKey key={0};D3D8DrawStateRecord record;
    key.depth=g_pg.depth_test;key.depth_func=m[NV097_SET_DEPTH_FUNC/4];
    key.depth_mask=m[NV097_SET_DEPTH_MASK/4];key.stencil_enabled=nv_option_stencil_state();
    const unsigned offsets[8]={0x32C,0x364,0x368,0x36C,0x360,0x370,0x374,0x378};
    for(unsigned i=0;i<8;i++)key.stencil[i]=m[offsets[i]/4];
    key.cull_face=m[NV097_SET_CULL_FACE/4];key.front_face=m[NV097_SET_FRONT_FACE/4];
    key.fog=m[NV097_SET_FOG_COLOR/4];key.blend=g_pg.blend_enable;
    key.src=g_pg.blend_sfactor;key.dst=g_pg.blend_dfactor;
    key.alpha=g_pg.alpha_test;key.alpha_func=g_pg.alpha_func;key.alpha_ref=g_pg.alpha_ref;
    key.color_mask=g_pg.color_mask;nv_draw_state_compile(&record,&key);
    if(record.culled||record.unsupported_cull)return 0;
    d3d8_apply_draw_state_record(&record);d3d8_states_set_depth_offset(0,0);
    unsigned u=nv_texture_address(g_pg.tex[0].address,0),v=nv_texture_address(g_pg.tex[0].address,1);
    unsigned w=nv_texture_address(g_pg.tex[0].address,2);
    if(!u||!v)return 0;
    dev->lpVtbl->SetTextureStageState(dev,0,D3DTSS_ADDRESSU,u);
    dev->lpVtbl->SetTextureStageState(dev,0,D3DTSS_ADDRESSV,v);
    d3d8_states_set_address_w(0,w?w:D3DTADDRESS_WRAP);
    apply_texture_spatial_filter(dev,0,g_pg.tex[0].filter,g_pg.tex[0].enabled,
        g_pg.tex[0].host_levels,g_pg.tex[0].control0,NULL);
    d3d8_states_apply();return 1;
}
static int uc_hybrid_particles_draw(const UcHybridParticleBatch *b){
    if(hybrid_particles.disabled||!b||!b->count||b->count>39||!g_pg.initialized||
       !uc_hybrid_drain())return 0;
    ID3D11DeviceContext *ctx=d3d8_GetD3D11Context();IDirect3DDevice8 *dev=xbox_GetD3DDevice();
    if(!ctx||!dev||g_pg.reports.active||g_pg.reports.failed||nv_removal.enabled||nv_deep_trace.active)return 0;
    ID3D11Predicate *predicate=NULL;BOOL predicate_value=FALSE;
    ID3D11DeviceContext_GetPredication(ctx,&predicate,&predicate_value);
    if(predicate){ID3D11Predicate_Release(predicate);return 0;}
    const uint32_t *m=g_pg.trace_methods;
    if((g_pg.transform_execution_mode&3)!=2||!g_pg.tex[0].enabled||g_pg.shader_stage_program!=1||
       !uc_hybrid_particle_program(g_pg.vertex_program,g_pg.transform_program_start,
           g_pg.vertex_constants,g_pg.array_format,m)||
       memcmp(b->clip_rows,g_pg.vertex_constants[104],64)||
       memcmp(b->up,g_pg.vertex_constants[120],16)||memcmp(b->right,g_pg.vertex_constants[121],16)||
       (m[NV097_SET_FOG_ENABLE/4]&&(m[NV097_SET_FOG_MODE/4]!=0x2601||m[NV097_SET_FOG_GEN_MODE/4]!=2)))return 0;
    LiveTextureLayout layout;
    if(!live_texture_layout((g_pg.tex[0].format>>8)&255,&layout)||(!layout.swizzled&&!layout.compressed))return 0;
    UCNativeControlParticleDraw draw={0};NvSurface *color=NULL,*depth=NULL;
    uint32_t format=m[0x208/4],pitch=m[0x20C/4];unsigned w=g_pg.surface_clip_h>>16,h=g_pg.surface_clip_v>>16;
    if(surface_path_enabled()&&(format&0xFFFF)==0x128&&m[0x214/4]&&w&&h&&
       !(g_pg.surface_clip_h&0xFFFF)&&!(g_pg.surface_clip_v&0xFFFF)){
        color=nv_surface_find_cached(&g_pg.surfaces,0,m[0x210/4],pitch&0xFFFF,w,h);
        depth=nv_surface_find_cached(&g_pg.surfaces,1,m[0x214/4],pitch>>16,w,h);
    }
    if(!color||!depth||!color->rtv||!depth->dsv||color->samples!=1||depth->samples!=1||
       color->physical_width!=depth->physical_width||color->physical_height!=depth->physical_height)return 0;
    UCNativeControlParticleView view={0};float attrs[16][4];
    if(!uc_hybrid_particle_projection(view.clip_rows,g_pg.vertex_constants,(float)w,(float)h,u2f(m[NV097_SET_CLIP_MAX/4]))||
       !uc_hybrid_particle_latch(attrs,g_pg.immediate_attr,g_pg.array_format,b->last_raw))return 0;
    memcpy(view.up,b->up,16);memcpy(view.right,b->right,16);
    for(unsigned i=0;i<4;i++)view.fog_color[i]=(float)((m[NV097_SET_FOG_COLOR/4]>>(8*i))&255)/255.f;
    view.fog_enabled=!!m[NV097_SET_FOG_ENABLE/4];
    if(view.fog_enabled){view.fog_bias=u2f(m[NV097_SET_FOG_PARAMS/4]);view.fog_slope=u2f(m[NV097_SET_FOG_PARAMS/4+1]);}
    view.alpha_test=!!g_pg.alpha_test;view.texture_alpha_kill=!!(g_pg.tex[0].control0&4u);
    if(!hybrid_particles.initialized){
        hybrid_particles.initialized=1;wchar_t shaders[4096];
        DWORD n=GetEnvironmentVariableW(L"UCRR_HYBRID_SHADERS",shaders,4096);
        if(n&&n<4096)hybrid_particles.renderer=uc_native_control_particles_create(d3d8_GetD3D11Device(),ctx,
            shaders,hybrid_particles.error,sizeof hybrid_particles.error);
        if(!hybrid_particles.renderer){hybrid_particles.disabled=1;return 0;}
    }
    /* Reuse the original live texture owner; no duplicate asset load or upload.
     * Rejection below has submitted no pixels, so the original emission is safe. */
    D3D8Texture *texture=(D3D8Texture*)update_live_texture(0);
    if(!texture||nv_effect_should_skip(nv_effect_stage[0],g_pg.blend_enable,0,
        m[NV097_SET_DEPTH_MASK/4]!=0,m[0x32C/4]!=0,0,g_pg.shader_stage_program)||
       !uc_hybrid_particles_state(dev))return 0;
    draw.texture=texture->surface_srv?texture->surface_srv:texture->srv;
    /* Texture preparation can retire surface work; inspect the actual parent
     * target/viewport only after it and render-state materialization complete. */
    ID3D11DeviceContext_OMGetRenderTargets(ctx,1,&draw.target,&draw.depth);
    int same=draw.target==color->rtv&&draw.depth==depth->dsv;
    if(draw.target)ID3D11RenderTargetView_Release(draw.target);
    if(draw.depth)ID3D11DepthStencilView_Release(draw.depth);
    UINT viewports=1;ID3D11DeviceContext_RSGetViewports(ctx,&viewports,&draw.viewport);
    if(!same||viewports!=1||draw.viewport.TopLeftX!=0||draw.viewport.TopLeftY!=0||
       draw.viewport.MinDepth!=0||draw.viewport.MaxDepth!=1||
       draw.viewport.Width!=(float)color->physical_width||draw.viewport.Height!=(float)color->physical_height)return 0;
    ID3D11RasterizerState *raster=NULL;ID3D11DeviceContext_RSGetState(ctx,&raster);
    if(!raster)return 0;
    D3D11_RASTERIZER_DESC rd;ID3D11RasterizerState_GetDesc(raster,&rd);ID3D11RasterizerState_Release(raster);
    if(rd.FillMode!=D3D11_FILL_SOLID||rd.CullMode!=D3D11_CULL_NONE||rd.DepthBias||rd.DepthBiasClamp||
       rd.SlopeScaledDepthBias||!rd.DepthClipEnable||rd.ScissorEnable||rd.MultisampleEnable)return 0;
    ID3D11DeviceContext_OMGetBlendState(ctx,&draw.blend,draw.blend_factor,&draw.sample_mask);
    ID3D11DeviceContext_OMGetDepthStencilState(ctx,&draw.depth_state,&draw.stencil_ref);
    ID3D11DeviceContext_PSGetSamplers(ctx,0,1,&draw.sampler);
    int result=uc_native_control_particles_draw(hybrid_particles.renderer,b->billboards,b->count,&view,&draw,
        hybrid_particles.error,sizeof hybrid_particles.error);
    if(draw.blend)ID3D11BlendState_Release(draw.blend);
    if(draw.depth_state)ID3D11DepthStencilState_Release(draw.depth_state);
    if(draw.sampler)ID3D11SamplerState_Release(draw.sampler);
    d3d8_vertex_binding_invalidate();d3d8_target_binding_invalidate();d3d8_viewport_binding_invalidate();
    if(!result)return 0;
    /* Preserve the last original vertex's attribute latch and readable method
     * values, while explicitly leaving NO fake/stale inline payload to replay. */
    memcpy(g_pg.immediate_attr,attrs,sizeof attrs);nv_immediate_begin(&immediate_gpu);raw_packet_active=0;
    g_pg.in_draw=0;g_pg.draw_mode=8;g_pg.d3d_prim_type=nv2a_draw_mode_to_d3d(8);g_pg.inline_count=0;
    g_pg.immediate_draw=g_pg.inline_array_draw=g_pg.indexed_draw=g_pg.indexed_failed=0;
    vertex_dma_begin();vertex_fetch_plan.valid=0;nv_attribute_cache_advance(&attribute_cache,52*b->count+2);
    g_pg.trace_methods[NV097_SET_BEGIN_END/4]=0;g_pg.trace_methods[NV097_INLINE_ARRAY/4]=b->last_raw[12];
    ++g_pg.batches_started;
    if(result<0){hybrid_particles.disabled=1;return -1;}
    ++g_pg.stats.draw_calls;g_pg.stats.vertices_submitted+=4*b->count;
    d3d8_CustomLightingSurfaceWritten();return 1;
}
int uc_hybrid_particles_host_draw(const UcHybridParticleBatch *b){
    ++hybrid_particles.calls;int result=uc_hybrid_particles_draw(b);
    if(result>0){++hybrid_particles.submitted;hybrid_particles.records+=b->count;}
    else if(result<0)++hybrid_particles.fatal;else ++hybrid_particles.rejected;
    if(result>0&&!hybrid_particles.shown){
        hybrid_particles.shown=1;
        SetWindowTextA(d3d8_GetHWND(),hybrid_host.batches?
            "Urban Chaos Hybrid - Native scenery lighting and particles ACTIVE":
            "Urban Chaos Hybrid - Native particles ACTIVE");
    }
    return result;
}
/* Only the existing explicitly requested capture writes this; normal play
 * updates bounded counters without logs, readbacks or background probes. */
static void uc_hybrid_particles_capture(FILE *f){
    fprintf(f,"{\"calls\":%llu,\"rejected\":%llu,\"submitted\":%llu,\"records\":%llu,\"fatal\":%llu,\"disabled\":%u,\"error\":\"",
        hybrid_particles.calls,hybrid_particles.rejected,hybrid_particles.submitted,
        hybrid_particles.records,hybrid_particles.fatal,hybrid_particles.disabled);
    for(const char *p=hybrid_particles.error;*p;p++){
        if(*p=='"'||*p=='\\')fputc('\\',f);if((unsigned char)*p>=32)fputc(*p,f);
    }
    fputs("\"}",f);
}
