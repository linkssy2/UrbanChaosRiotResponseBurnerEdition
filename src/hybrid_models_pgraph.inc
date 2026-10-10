/* Actual synchronous source light traversal, before its later albedo passes. */
#include "hybrid_models_guest.h"
#include "control_model_bridge.h"
#include "hybrid_models_texture.h"
#include "../third_party/xboxrecomp/src/d3d/d3d8_host_anisotropy.h"
static struct {UCNativeControlModels *renderer;ID3D11SamplerState *sampler;D3D11_SAMPLER_DESC sampler_desc;unsigned initialized,disabled;uint64_t calls,submitted,vertices,rejected,fatal,base_calls,base_submitted,base_rejected,base_fatal;char error[512];} hybrid_models;
static void uc_hybrid_models_shutdown(void){uc_native_control_models_destroy(hybrid_models.renderer);hybrid_models.renderer=NULL;if(hybrid_models.sampler)ID3D11SamplerState_Release(hybrid_models.sampler);hybrid_models.sampler=NULL;}
static int uc_hybrid_models_texture(const UcHybridModelBatch *b,UCNativeControlParticleDraw *draw){
    /* This texture is selected by the source material/IFL, not by whichever
     * earlier draw happened to publish stage 0. Reuse its ordinary cache and
     * revision validation, restoring all stage/admission metadata afterward. */
    LiveTextureLayout layout;uint32_t format=b->texture_header[3];
    if(!b->texture_header_va||(format&12)!=8||((format>>4)&15)!=2||
       !live_texture_layout((format>>8)&255,&layout)||!layout.compressed)return 0;
    unsigned u=nv_texture_address(b->texture_address,0),v=nv_texture_address(b->texture_address,1),w=nv_texture_address(b->texture_address,2);
    if(!u||!v||!w||(b->texture_filter&0xF0000000u))return 0;
    unsigned char old[sizeof g_pg.tex[0]];memcpy(old,&g_pg.tex[0],sizeof old);
    unsigned effect=nv_effect_stage[0],hud=pc_hud_stage[0];NvRetainedResource retained=raw_retained_resources.prepared[0];
    uc_model_texture_registers(b->texture_header,&g_pg.tex[0].offset,&g_pg.tex[0].format,&g_pg.tex[0].control1,&g_pg.tex[0].image_rect);
    g_pg.tex[0].enabled=1;g_pg.tex[0].address=b->texture_address;g_pg.tex[0].filter=b->texture_filter;g_pg.tex[0].control0=b->texture_control0;
    D3D8Texture *texture=(D3D8Texture*)update_live_texture_impl(0);
    unsigned levels=texture?texture->levels:0;
    if(texture){draw->texture=texture->surface_srv?texture->surface_srv:texture->srv;if(draw->texture)ID3D11ShaderResourceView_AddRef(draw->texture);}
    memcpy(&g_pg.tex[0],old,sizeof old);nv_effect_stage[0]=effect;pc_hud_stage[0]=hud;raw_retained_resources.prepared[0]=retained;
    if(!draw->texture)return 0;
    /* The owned sampler can represent the source LOD clamp directly; do not
     * trigger the ordinary TSS adapter's base-level fallback for nonzero MinLOD. */
    D3D8SamplerInputs input={0};apply_texture_spatial_filter(NULL,0,b->texture_filter,1,levels,b->texture_control0&~0x3FFC0000u,&input);
    D3D11_SAMPLER_DESC desc={0};
    desc.Filter=(D3D11_FILTER)((input.min_filter==D3DTEXF_LINEAR?D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT:0)|
        (input.mag_filter==D3DTEXF_LINEAR?D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT:0)|
        (input.mip_filter==D3DTEXF_LINEAR?D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR:0));
    desc.AddressU=(D3D11_TEXTURE_ADDRESS_MODE)u;desc.AddressV=(D3D11_TEXTURE_ADDRESS_MODE)v;desc.AddressW=(D3D11_TEXTURE_ADDRESS_MODE)w;
    memcpy(&desc.MipLODBias,&input.lod_bits,4);desc.MaxAnisotropy=1;desc.ComparisonFunc=D3D11_COMPARISON_NEVER;
    if(desc.MipLODBias<D3D11_MIP_LOD_BIAS_MIN)desc.MipLODBias=D3D11_MIP_LOD_BIAS_MIN;
    if(desc.MipLODBias>D3D11_MIP_LOD_BIAS_MAX)desc.MipLODBias=D3D11_MIP_LOD_BIAS_MAX;
    if(input.mip_filter!=D3DTEXF_NONE){desc.MinLOD=(float)((b->texture_control0>>18)&4095)/256.f;desc.MaxLOD=(float)((b->texture_control0>>6)&4095)/256.f;}
    d3d8_host_anisotropy(&desc,d3d8_GetHostAnisotropy());
    if(!hybrid_models.sampler||memcmp(&desc,&hybrid_models.sampler_desc,sizeof desc)){
        ID3D11SamplerState *next=NULL;
        if(FAILED(ID3D11Device_CreateSamplerState(d3d8_GetD3D11Device(),&desc,&next))){ID3D11ShaderResourceView_Release(draw->texture);draw->texture=NULL;return 0;}
        if(hybrid_models.sampler)ID3D11SamplerState_Release(hybrid_models.sampler);hybrid_models.sampler=next;hybrid_models.sampler_desc=desc;
    }
    draw->sampler=hybrid_models.sampler;ID3D11SamplerState_AddRef(draw->sampler);return 1;
}
int uc_hybrid_models_host_palette(UCModelRenderPalette *p){
    if(!p||!uc_hybrid_drain())return 0;
    memcpy(p->rows,g_pg.vertex_constants,sizeof p->rows);
    for(unsigned i=0;i<192;i++){
        p->valid[i]=1;for(unsigned j=0;j<4;j++)if(!isfinite(p->rows[i][j]))p->valid[i]=0;
    }
    return 1;
}
static int uc_hybrid_models_state(const UcHybridModelBatch *b){
    const uint32_t*m=g_pg.trace_methods;
    /* A2B20 publishes the original ONE/ONE light phase. Never add native
     * contribution after the later receiver base/material composition. */
    if(!g_pg.blend_enable||g_pg.blend_sfactor!=1||g_pg.blend_dfactor!=1||
       m[NV097_SET_DEPTH_MASK/4]||!g_pg.depth_test||g_pg.alpha_test)return 0;
    NvDrawStateKey key={0};D3D8DrawStateRecord record;
    key.depth=g_pg.depth_test;key.depth_func=m[NV097_SET_DEPTH_FUNC/4];key.depth_mask=m[NV097_SET_DEPTH_MASK/4];
    key.stencil_enabled=nv_option_stencil_state();const unsigned offsets[8]={0x32C,0x364,0x368,0x36C,0x360,0x370,0x374,0x378};
    for(unsigned i=0;i<8;i++)key.stencil[i]=m[offsets[i]/4];
    key.cull_enabled=g_pg.cull_enable;key.cull_face=m[NV097_SET_CULL_FACE/4];key.front_face=m[NV097_SET_FRONT_FACE/4];
    key.fog=m[NV097_SET_FOG_COLOR/4];key.blend=g_pg.blend_enable;key.src=g_pg.blend_sfactor;key.dst=g_pg.blend_dfactor;key.color_mask=g_pg.color_mask;
    nv_draw_state_compile(&record,&key);if(record.culled||record.unsupported_cull)return 0;
    float slope;int units;if(!nv_depth_offset(m,5,g_pg.depth_test,&slope,&units))return 0;
    if(b->flags&0x400){slope=-.75f;units=-3;}
    d3d8_apply_draw_state_record(&record);d3d8_states_set_depth_offset(slope,units);d3d8_states_apply();return 1;
}
static int uc_hybrid_models_draw(const UcHybridLightingScene*s,const UcHybridLightingRecord*l,const UcHybridModelBatch*b){
    if(hybrid_models.disabled||!s||!l||!b||!b->vertex_count||nv_removal.enabled||nv_deep_trace.active)return 0;
    UcHybridModelLightView prepared;
    if(!uc_hybrid_models_prepare(s,l,b,&prepared))return 0;
    UcHybridTarget target;if(!uc_hybrid_target(&target)||!uc_hybrid_models_state(b))return 0;
    typedef char SameView[(sizeof(UcHybridModelLightView)==sizeof(UCNativeControlModelView))?1:-1];
    typedef char SameVertex[(sizeof(UcHybridModelVertex)==sizeof(UCNativeControlModelVertex))?1:-1];
    typedef char SameIndexedVertex[(sizeof(UcHybridModelIndexedVertex)==sizeof(UCNativeControlModelIndexedVertex))?1:-1];
    UCNativeControlModelView view;memcpy(&view,&prepared,sizeof view);
    float matrix[16],vp[16];for(unsigned i=0;i<4;i++)for(unsigned j=0;j<4;j++)matrix[i*4+j]=view.clip_rows[j][i];
    if(!uc_hybrid_projection(vp,matrix,(float)target.width,(float)target.height))return 0;
    for(unsigned i=0;i<4;i++)for(unsigned j=0;j<4;j++)view.clip_rows[i][j]=vp[j*4+i];
    ID3D11DeviceContext *ctx=d3d8_GetD3D11Context();
    if(!hybrid_models.initialized){
        hybrid_models.initialized=1;wchar_t directory[4096];DWORD n=GetEnvironmentVariableW(L"UCRR_HYBRID_SHADERS",directory,4096);
        if(n&&n<4096)hybrid_models.renderer=uc_native_control_models_create(d3d8_GetD3D11Device(),ctx,directory,hybrid_models.error,sizeof hybrid_models.error);
        if(!hybrid_models.renderer){hybrid_models.disabled=1;return 0;}
    }
    UCNativeControlParticleDraw draw={0};draw.target=target.color;draw.depth=target.depth;draw.viewport=target.viewport;
    if(view.material_kind&&!uc_hybrid_models_texture(b,&draw))return 0;
    int result=0;ID3D11RasterizerState *raster=NULL;ID3D11DeviceContext_RSGetState(ctx,&raster);if(!raster)goto release;
    ID3D11DeviceContext_OMGetBlendState(ctx,&draw.blend,draw.blend_factor,&draw.sample_mask);
    ID3D11DeviceContext_OMGetDepthStencilState(ctx,&draw.depth_state,&draw.stencil_ref);
    if(b->indexed_vertices&&b->palette&&!view.material_kind&&view.kind%3!=2&&nv_gpu_first_vertex_enabled()){
        /* Retain the source viewport/RCC sequence; prepared rows are still raw. */
        UCNativeControlModelSourceClip source_clip={0};
        memcpy(source_clip.rows,prepared.clip_rows,sizeof source_clip.rows);
        memcpy(source_clip.scale,g_pg.vertex_constants[58],sizeof source_clip.scale);
        memcpy(source_clip.bias,g_pg.vertex_constants[59],sizeof source_clip.bias);
        source_clip.screen[0]=(float)target.width;source_clip.screen[1]=(float)target.height;
        source_clip.screen[2]=u2f(g_pg.trace_methods[NV097_SET_CLIP_MAX/4]);
        result=uc_native_control_models_draw_indexed_source(hybrid_models.renderer,
            (const UCNativeControlModelIndexedVertex*)b->indexed_vertices,b->vertex_count,
            b->indices,b->index_count,b->primitive,b->palette->rows,b->skinned,
            &source_clip,&view,&draw,raster,hybrid_models.error,sizeof hybrid_models.error);
    }
    else if(b->indexed_vertices&&b->palette)
        result=uc_native_control_models_draw_indexed(hybrid_models.renderer,
            (const UCNativeControlModelIndexedVertex*)b->indexed_vertices,b->vertex_count,
            b->indices,b->index_count,b->primitive,b->palette->rows,b->skinned,
            &view,&draw,raster,hybrid_models.error,sizeof hybrid_models.error);
    else
        result=uc_native_control_models_draw(hybrid_models.renderer,(const UCNativeControlModelVertex*)b->vertices,b->vertex_count,
            &view,&draw,raster,hybrid_models.error,sizeof hybrid_models.error);
    ID3D11RasterizerState_Release(raster);if(draw.blend)ID3D11BlendState_Release(draw.blend);if(draw.depth_state)ID3D11DepthStencilState_Release(draw.depth_state);
release:
    if(draw.texture)ID3D11ShaderResourceView_Release(draw.texture);if(draw.sampler)ID3D11SamplerState_Release(draw.sampler);
    d3d8_vertex_binding_invalidate();d3d8_target_binding_invalidate();d3d8_viewport_binding_invalidate();
    if(result>0){++g_pg.stats.draw_calls;g_pg.stats.vertices_submitted+=b->indexed_vertices?b->index_count:b->vertex_count;d3d8_CustomLightingSurfaceWritten();}
    if(result<0)hybrid_models.disabled=1;
    return result;
}
int uc_hybrid_models_host_draw(const UcHybridLightingScene*s,const UcHybridLightingRecord*l,const UcHybridModelBatch*b){
    ++hybrid_models.calls;int result=uc_hybrid_models_draw(s,l,b);
    if(result>0){++hybrid_models.submitted;hybrid_models.vertices+=b->vertex_count;}
    else if(result<0)++hybrid_models.fatal;else ++hybrid_models.rejected;
    return result;
}
/* Existing explicit diagnostic capture only. Normal play never writes these. */
static void uc_hybrid_extended_capture(FILE *f){
    uint64_t models[12]={0},casters[8]={0};uc_hybrid_models_read_counts(models,12);uc_hybrid_casters_read_counts(casters,8);
    fprintf(f,"{\"model_draws\":%llu,\"model_vertices\":%llu,\"model_rejected\":%llu,\"model_fatal\":%llu,\"decal_draws\":%llu,\"textured_quad_draws\":%llu,\"quad_rejected\":%llu,\"quad_fatal\":%llu,\"spot_batches\":%llu,\"caster_frames\":%llu,\"caster_pairs\":%llu,\"caster_failed\":%llu,\"model_guest\":[",
        hybrid_models.submitted,hybrid_models.vertices,hybrid_models.rejected,hybrid_models.fatal,
        hybrid_quads.decal_draws,hybrid_quads.textured_draws,hybrid_quads.rejected,hybrid_quads.fatal,
        hybrid_host.spot_batches,hybrid_host.caster_frames,hybrid_host.caster_pairs,hybrid_host.caster_failed);
    for(unsigned i=0;i<12;i++)fprintf(f,"%s%llu",i?",":"",models[i]);
    fprintf(f,"],\"base_calls\":%llu,\"base_submitted\":%llu,\"base_rejected\":%llu,\"base_fatal\":%llu,\"caster_guest\":[",hybrid_models.base_calls,hybrid_models.base_submitted,hybrid_models.base_rejected,hybrid_models.base_fatal);for(unsigned i=0;i<8;i++)fprintf(f,"%s%llu",i?",":"",casters[i]);fputs("]}",f);
}

/* Qualified static/posed base and exposure materials only. Original 8FD80/8EC20 setup is retained;
 * the executed program and combiner are requalified after draining that setup. */
#include "hybrid_base_pgraph_contract.h"
static int uc_hybrid_models_draw_base(const UcHybridModelBatch*b){
    if(hybrid_models.disabled||!b||!b->vertices||!b->vertex_count||nv_removal.enabled||nv_deep_trace.active)return 0;
    UcHybridTarget target;if(!uc_hybrid_target(&target))return 0;
    int kind=uc_hybrid_base_program(g_pg.vertex_program,g_pg.transform_program_start,g_pg.trace_methods,b->skinned);
    if(!kind||
       (g_pg.transform_execution_mode&3)!=2||g_pg.shader_stage_program!=1||!g_pg.tex[0].enabled||
       (g_pg.tex[0].control0&4u)||g_pg.chyron_scroll_offset||!nv_option_lighting_textures()||!nv_option_multi_textures())return 0;
    ID3D11DeviceContext*ctx=d3d8_GetD3D11Context();IDirect3DDevice8*dev=xbox_GetD3DDevice();if(!ctx||!dev)return 0;
    UCNativeControlModelSourceClip clip={0};memcpy(clip.rows,g_pg.vertex_constants[104],sizeof clip.rows);
    memcpy(clip.scale,g_pg.vertex_constants[58],sizeof clip.scale);memcpy(clip.bias,g_pg.vertex_constants[59],sizeof clip.bias);
    clip.screen[0]=(float)target.width;clip.screen[1]=(float)target.height;clip.screen[2]=u2f(g_pg.trace_methods[NV097_SET_CLIP_MAX/4]);
    D3D8Texture*texture=(D3D8Texture*)update_live_texture(0);
    if(!texture||!uc_hybrid_quads_state(dev,0))return 0;
    if(!hybrid_models.initialized){
        hybrid_models.initialized=1;wchar_t directory[4096];DWORD n=GetEnvironmentVariableW(L"UCRR_HYBRID_SHADERS",directory,4096);
        if(n&&n<4096)hybrid_models.renderer=uc_native_control_models_create(d3d8_GetD3D11Device(),ctx,directory,hybrid_models.error,sizeof hybrid_models.error);
        if(!hybrid_models.renderer){hybrid_models.disabled=1;return 0;}
    }
    UCNativeControlParticleDraw draw={0};draw.target=target.color;draw.depth=target.depth;draw.viewport=target.viewport;
    draw.texture=texture->surface_srv?texture->surface_srv:texture->srv;
    ID3D11RasterizerState*raster=NULL;ID3D11DeviceContext_RSGetState(ctx,&raster);
    ID3D11DeviceContext_OMGetBlendState(ctx,&draw.blend,draw.blend_factor,&draw.sample_mask);
    ID3D11DeviceContext_OMGetDepthStencilState(ctx,&draw.depth_state,&draw.stencil_ref);ID3D11DeviceContext_PSGetSamplers(ctx,0,1,&draw.sampler);
    uint32_t argb=g_pg.trace_methods[NV097_SET_COMBINER_FACTOR0/4];float constant[4]={(float)((argb>>16)&255)/255.f,(float)((argb>>8)&255)/255.f,(float)(argb&255)/255.f,(kind&16)?1.f:(float)(argb>>24)/255.f};
    float alpha[2]={(kind&4)?(float)(g_pg.trace_methods[NV097_SET_COMBINER_FACTOR0/4]>>24)/255.f:-1.f,g_pg.alpha_test?1.f/255.f:-1.f};
    int result=b->index_count?
        uc_native_control_models_base_indexed(hybrid_models.renderer,(const UCNativeControlModelVertex*)b->vertices,b->vertex_count,b->indices,b->index_count,
            &clip,g_pg.vertex_constants[125],(kind&3)==2?g_pg.vertex_constants[119]:NULL,alpha,(kind&8)?constant:NULL,&draw,raster,hybrid_models.error,sizeof hybrid_models.error):
        uc_native_control_models_base(hybrid_models.renderer,(const UCNativeControlModelVertex*)b->vertices,b->vertex_count,
            &clip,g_pg.vertex_constants[125],(kind&3)==2?g_pg.vertex_constants[119]:NULL,alpha,(kind&8)?constant:NULL,&draw,raster,hybrid_models.error,sizeof hybrid_models.error);
    if(raster)ID3D11RasterizerState_Release(raster);if(draw.blend)ID3D11BlendState_Release(draw.blend);
    if(draw.depth_state)ID3D11DepthStencilState_Release(draw.depth_state);if(draw.sampler)ID3D11SamplerState_Release(draw.sampler);
    d3d8_vertex_binding_invalidate();d3d8_target_binding_invalidate();d3d8_viewport_binding_invalidate();
    if(result>0){++g_pg.stats.draw_calls;g_pg.stats.vertices_submitted+=b->index_count?b->index_count:b->vertex_count;d3d8_CustomLightingSurfaceWritten();}
    if(result<0)hybrid_models.disabled=1;
    return result;
}

int uc_hybrid_models_host_base(const UcHybridModelBatch*b){
    ++hybrid_models.base_calls;int result=uc_hybrid_models_draw_base(b);
    if(result>0)++hybrid_models.base_submitted;else if(result<0)++hybrid_models.base_fatal;else ++hybrid_models.base_rejected;
    return result;
}
