/* Qualified native decals and ordinary textured quads (including body blobs
 * and authored sprites). Included after the shared hybrid/particle owners.
 * submit_draw_body calls this BEFORE the generic immediate-GPU expansion.
 * Original SDK emission, source lifecycle and input latches have already run. */
#include "control_quad_bridge.h"
#include "hybrid_quads_pgraph_contract.h"
static struct {UCNativeControlQuads *renderer;unsigned initialized,disabled;uint64_t decal_draws,textured_draws,vertices,rejected,fatal;char error[512];} hybrid_quads;
static void uc_hybrid_quads_shutdown(void){uc_native_control_quads_destroy(hybrid_quads.renderer);hybrid_quads.renderer=NULL;}
static int uc_hybrid_quads_state(IDirect3DDevice8 *dev,unsigned kind){
    const uint32_t *m=g_pg.trace_methods;float slope;int units;
    if(!nv_depth_offset(m,8,g_pg.depth_test,&slope,&units)||
       (g_pg.alpha_test&&(kind==1?(g_pg.alpha_func!=0x206&&g_pg.alpha_func!=0x204):
          (g_pg.alpha_func!=0x206||(g_pg.alpha_ref&255)!=1)))||
       (kind==2&&(g_pg.cull_enable||slope||units||((g_pg.tex[0].control0&4u)&&!g_pg.alpha_test))))return 0;
    NvDrawStateKey key={0};D3D8DrawStateRecord record;
    key.depth=g_pg.depth_test;key.depth_func=m[NV097_SET_DEPTH_FUNC/4];
    key.depth_mask=m[NV097_SET_DEPTH_MASK/4];key.stencil_enabled=nv_option_stencil_state();
    const unsigned offsets[8]={0x32C,0x364,0x368,0x36C,0x360,0x370,0x374,0x378};
    for(unsigned i=0;i<8;i++)key.stencil[i]=m[offsets[i]/4];
    key.cull_enabled=g_pg.cull_enable;key.cull_face=m[NV097_SET_CULL_FACE/4];key.front_face=m[NV097_SET_FRONT_FACE/4];
    key.fog=m[NV097_SET_FOG_COLOR/4];key.blend=g_pg.blend_enable;
    key.src=g_pg.blend_sfactor;key.dst=g_pg.blend_dfactor;
    key.alpha=g_pg.alpha_test;key.alpha_func=g_pg.alpha_func;key.alpha_ref=g_pg.alpha_ref;
    key.color_mask=g_pg.color_mask;nv_draw_state_compile(&record,&key);
    if(record.culled||record.unsupported_cull)return 0;
    d3d8_apply_draw_state_record(&record);d3d8_states_set_depth_offset(slope,units);
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

static int uc_hybrid_quads_enabled(void){static int enabled=-1;if(enabled<0){const char*s=getenv("UCRR_HYBRID_EXTENDED");enabled=s&&!strcmp(s,"1");}return enabled;}
/* Only these two fixed shaders capture extra raw attributes. Rejected draws
 * materialize back to the original CPU path unless its GPU experiment was
 * independently enabled. Never enable the broad experiment as a side effect. */
static int uc_hybrid_quads_capture_ready(void){return uc_hybrid_quads_enabled()&&!hybrid_quads.disabled&&
 (g_pg.transform_execution_mode&3)==2&&g_pg.draw_mode==8&&g_pg.shader_stage_program==1&&
 uc_quad_kind(g_pg.vertex_program,g_pg.transform_program_start,g_pg.trace_methods)!=0;}
static int uc_hybrid_quads_submit(void){
 int enabled=uc_hybrid_quads_enabled();
 const unsigned count=immediate_gpu.count;
 if(!enabled||hybrid_quads.disabled||!count||count>512||count%4||immediate_gpu.disabled||
    !g_pg.initialized||g_pg.draw_mode!=8||g_pg.indexed_draw||g_pg.indexed_failed||
    g_pg.inline_count!=count*INLINE_VERT_DWORDS||g_pg.inline_array_draw||
    !nv_immediate_same(&immediate_gpu,g_pg.vertex_program,g_pg.transform_program_start,g_pg.vertex_constants,
        g_pg.transform_execution_mode,g_pg.trace_methods[NV097_SET_CLIP_MAX/4])||
    !surface_path_enabled()||!nv_option_lighting_textures()||!nv_option_multi_textures()||
    raw_draw_retain_enabled()||nv_removal.enabled||nv_deep_trace.active||g_pg.chyron_scroll_offset||
    g_pg.reports.active||g_pg.reports.failed||(g_pg.transform_execution_mode&3)!=2||
    !g_pg.tex[0].enabled||g_pg.shader_stage_program!=1)return 0;
 const uint32_t *m=g_pg.trace_methods;unsigned kind=uc_quad_kind(g_pg.vertex_program,g_pg.transform_program_start,m);
 if(!kind)return 0;
 ID3D11DeviceContext *ctx=d3d8_GetD3D11Context();IDirect3DDevice8 *dev=xbox_GetD3DDevice();if(!ctx||!dev)return 0;
 ID3D11Predicate *predicate=NULL;BOOL value=FALSE;ID3D11DeviceContext_GetPredication(ctx,&predicate,&value);
 if(predicate){ID3D11Predicate_Release(predicate);return 0;}
 LiveTextureLayout layout;if(!live_texture_layout((g_pg.tex[0].format>>8)&255,&layout)||(!layout.swizzled&&!layout.compressed))return 0;
 unsigned w=g_pg.surface_clip_h>>16,h=g_pg.surface_clip_v>>16;uint32_t format=m[0x208/4],pitch=m[0x20c/4];
 if((format&0xffff)!=0x128||!m[0x214/4]||!w||!h||(g_pg.surface_clip_h&0xffff)||(g_pg.surface_clip_v&0xffff))return 0;
 NvSurface *color=nv_surface_find_cached(&g_pg.surfaces,0,m[0x210/4],pitch&0xffff,w,h);
 NvSurface *depth=nv_surface_find_cached(&g_pg.surfaces,1,m[0x214/4],pitch>>16,w,h);
 if(!color||!depth||!color->rtv||!depth->dsv||color->samples!=1||depth->samples!=1||
    color->physical_width!=depth->physical_width||color->physical_height!=depth->physical_height)return 0;
 UCNativeControlDecalView decal={0};UCNativeControlTexturedQuadView textured={0};
 if(!uc_hybrid_particle_projection(decal.clip_rows,g_pg.vertex_constants,(float)w,(float)h,u2f(m[NV097_SET_CLIP_MAX/4])))return 0;
 memcpy(textured.clip_rows,decal.clip_rows,64);memcpy(decal.tint,g_pg.vertex_constants[125],16);memcpy(decal.exposure,g_pg.vertex_constants[119],16);
 decal.alpha_test=!!g_pg.alpha_test;decal.alpha_greater=g_pg.alpha_func==0x204;decal.alpha_reference=g_pg.alpha_ref&255;
 textured.alpha_test=!!g_pg.alpha_test;textured.texture_alpha_kill=!!(g_pg.tex[0].control0&4u);
 for(unsigned i=0;i<3;i++)textured.fog[i]=(float)((m[NV097_SET_FOG_COLOR/4]>>(8*i))&255)/255.f;
 textured.fog[3]=1;
 if(kind==2&&m[NV097_SET_FOG_ENABLE/4]){
    if(m[NV097_SET_FOG_MODE/4]!=0x2601||m[NV097_SET_FOG_GEN_MODE/4]!=2)return 0;
    /* This exact VP never writes oFog: its initialized x is zero. */
    float factor=u2f(m[NV097_SET_FOG_PARAMS/4])-1.f;
    if(!isfinite(factor))return 0;textured.fog[3]=fmaxf(0,fminf(1,factor));
 }
 UCNativeControlDecalVertex dv[512];UCParticleQuadVertex tv[512];
 for(unsigned i=0;i<count;i++){
    const float (*a)[4]=immediate_gpu.attrs[i];
    if(a[0][3]!=1)return 0;
    for(unsigned k=0;k<4;k++)if(!isfinite(a[2][k]))return 0;
    if(kind==1){memcpy(dv[i].position,a[0],12);memcpy(dv[i].color,a[2],16);memcpy(dv[i].uv,a[3],8);}
    else{memcpy(tv[i].position,a[0],12);tv[i].color=nv2a_pack_argb(a[2]);memcpy(tv[i].uv,a[3],8);}
 }
 if(!hybrid_quads.initialized){
    hybrid_quads.initialized=1;wchar_t shaders[4096];DWORD n=GetEnvironmentVariableW(L"UCRR_HYBRID_SHADERS",shaders,4096);
    if(n&&n<4096)hybrid_quads.renderer=uc_native_control_quads_create(d3d8_GetD3D11Device(),ctx,shaders,hybrid_quads.error,sizeof hybrid_quads.error);
    if(!hybrid_quads.renderer){hybrid_quads.disabled=1;return 0;}
 }
 /* Earlier retained draws are already flushed by submit_draw. Materialize
  * current resources and state before borrowing the actual parent targets. */
 D3D8Texture *texture=(D3D8Texture*)update_live_texture(0);
 if(!texture||nv_effect_should_skip(nv_effect_stage[0],g_pg.blend_enable,0,m[NV097_SET_DEPTH_MASK/4]!=0,m[0x32c/4]!=0,0,g_pg.shader_stage_program)||!uc_hybrid_quads_state(dev,kind))return 0;
 UCNativeControlParticleDraw draw={0};draw.texture=texture->surface_srv?texture->surface_srv:texture->srv;
 ID3D11DeviceContext_OMGetRenderTargets(ctx,1,&draw.target,&draw.depth);int same=draw.target==color->rtv&&draw.depth==depth->dsv;
 if(draw.target)ID3D11RenderTargetView_Release(draw.target);if(draw.depth)ID3D11DepthStencilView_Release(draw.depth);
 UINT viewports=1;ID3D11DeviceContext_RSGetViewports(ctx,&viewports,&draw.viewport);
 if(!same||viewports!=1||draw.viewport.TopLeftX!=0||draw.viewport.TopLeftY!=0||draw.viewport.MinDepth!=0||draw.viewport.MaxDepth!=1||
    draw.viewport.Width!=(float)color->physical_width||draw.viewport.Height!=(float)color->physical_height)return 0;
 ID3D11RasterizerState *raster=NULL;ID3D11DeviceContext_RSGetState(ctx,&raster);if(!raster)return 0;
 ID3D11DeviceContext_OMGetBlendState(ctx,&draw.blend,draw.blend_factor,&draw.sample_mask);
 ID3D11DeviceContext_OMGetDepthStencilState(ctx,&draw.depth_state,&draw.stencil_ref);ID3D11DeviceContext_PSGetSamplers(ctx,0,1,&draw.sampler);
 int result=kind==1?uc_native_control_quads_decal(hybrid_quads.renderer,dv,count/4,&decal,&draw,raster,hybrid_quads.error,sizeof hybrid_quads.error):
    uc_native_control_quads_textured(hybrid_quads.renderer,tv,count/4,&textured,&draw,raster,hybrid_quads.error,sizeof hybrid_quads.error);
 ID3D11RasterizerState_Release(raster);if(draw.blend)ID3D11BlendState_Release(draw.blend);if(draw.depth_state)ID3D11DepthStencilState_Release(draw.depth_state);if(draw.sampler)ID3D11SamplerState_Release(draw.sampler);
 d3d8_vertex_binding_invalidate();d3d8_target_binding_invalidate();d3d8_viewport_binding_invalidate();
 if(!result){++hybrid_quads.rejected;return 0;}
 if(result<0){hybrid_quads.disabled=1;++hybrid_quads.fatal;return 1;}
 if(kind==1)++hybrid_quads.decal_draws;else ++hybrid_quads.textured_draws;
 hybrid_quads.vertices+=count;++g_pg.stats.draw_calls;g_pg.stats.vertices_submitted+=count;d3d8_CustomLightingSurfaceWritten();return 1;
}
