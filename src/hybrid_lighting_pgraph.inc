/* Separate native-lighting experiment, at the actual ordered PGRAPH owner. */
#include "hybrid_lighting_host.h"
#include "control_lighting_bridge.h"
#if defined(UCRR_HYBRID_EXTENDED)
#include "control_spotlight_bridge.h"
#include "hybrid_casters_guest.h"
#endif
#include "nv2a_pb_live.h"
#include "hybrid_lighting_projection.h"
#include <wchar.h>
static struct {
    UCNativeControlLighting *renderer;
    unsigned initialized;
    uint64_t calls,drain_failed,target_failed,query_failed,projection_failed;
    uint64_t create_failed,draw_failed,draw_ok,batches,lights,spot_batches,caster_frames,caster_pairs,caster_failed;
    UCNativeControlLightingCounts last;
    char error[512];
} hybrid_host;
static unsigned hybrid_shown;
static LONG hybrid_scenery_visible=1;
int uc_hybrid_scenery_lighting_visible(void){return InterlockedCompareExchange(&hybrid_scenery_visible,0,0)!=0;}
void uc_hybrid_scenery_lighting_show(int visible){InterlockedExchange(&hybrid_scenery_visible,!!visible);}
static void uc_hybrid_title(unsigned status) {
    if(status==hybrid_shown)return;
    hybrid_shown=status;
    SetWindowTextA(d3d8_GetHWND(),status==4?
        "Urban Chaos Hybrid - Native scenery lighting OFF":status==3?
        "Urban Chaos Hybrid - Native scenery lighting unavailable":status==2?
        "Urban Chaos Hybrid - Native scenery lighting ACTIVE":
        "Urban Chaos Hybrid - Waiting for native scenery lighting");
}
void uc_hybrid_lighting_host_reject(const char *reason) {
    snprintf(hybrid_host.error,sizeof(hybrid_host.error),"%s",reason);
    uc_hybrid_title(3);
}
static void uc_hybrid_host_shutdown(void) {
    uc_native_control_lighting_destroy(hybrid_host.renderer);
    hybrid_host.renderer=NULL;
}
static int uc_hybrid_drain(void) {
    uint32_t device,flags,port;uint8_t *ram=g_pg.guest_memory;size_t size=g_pg.guest_memory_size;
    if(!ram||size<0x3F423Cu)return 0;
    memcpy(&device,ram+0x3F4238,4);
    if(device<0x10000u||(uint64_t)device+0x1C24u>size)return 0;
    memcpy(&flags,ram+device+8,4);memcpy(&port,ram+device+0x1C20,4);
    if((flags&4u)||port!=0xFD800000u||!nv2a_pb_live_drain_reason(ram,size,0x003F4238u,NV_WAIT_OTHER)||g_pg.in_draw)return 0;
    gameplay_batch_flush();d3d8_BeforeHostMutation();return 1;
}
#if defined(UCRR_HYBRID_PC_FRONTEND)
/* The original movie render call remains between its guest predecessors and
 * later fades/title draws. Borrow the resolved current target after draining. */
int uc_hybrid_frontend_draw_movie(void (*render)(ID3D11Device*,ID3D11DeviceContext*,ID3D11RenderTargetView*,const D3D11_VIEWPORT*)){
    if(!render||!uc_hybrid_drain()||!bind_guest_surface())return 0;
    ID3D11DeviceContext *context=d3d8_GetD3D11Context();ID3D11RenderTargetView *view=NULL;
    ID3D11DeviceContext_OMGetRenderTargets(context,1,&view,NULL);
    D3D11_VIEWPORT viewport;UINT count=1;ID3D11DeviceContext_RSGetViewports(context,&count,&viewport);
    if(view&&count==1){render(d3d8_GetD3D11Device(),context,view,&viewport);d3d8_CustomLightingSurfaceWritten();}
    int ok=view&&count==1;if(view)ID3D11RenderTargetView_Release(view);return ok;
}
#endif
static int uc_hybrid_projection(float out[16],const float guest[16],float width,float height) {
    const float *scale=g_pg.vertex_constants[58],*bias=g_pg.vertex_constants[59];
    float depth=u2f(g_pg.trace_methods[NV097_SET_CLIP_MAX/4]);
    return uc_hybrid_compose_projection(out,guest,scale,bias,width,height,depth);
}
/* One target contract for all native light receivers. The logical Xbox extent
 * and actual PC viewport are deliberately kept distinct. */
typedef struct UcHybridTarget {
    ID3D11RenderTargetView *color;
    ID3D11DepthStencilView *depth;
    D3D11_VIEWPORT viewport;
    unsigned width,height;
} UcHybridTarget;
static int uc_hybrid_target(UcHybridTarget *out) {
    if(!uc_hybrid_drain()){++hybrid_host.drain_failed;return 0;}
    ID3D11DeviceContext *ctx=d3d8_GetD3D11Context();
    if(!ctx)return 0;
    ID3D11Predicate *predicate=NULL;BOOL predicate_value=FALSE;
    ID3D11DeviceContext_GetPredication(ctx,&predicate,&predicate_value);
    int queried=predicate!=NULL||g_pg.reports.active!=NULL||g_pg.reports.failed;
    if(predicate)ID3D11Predicate_Release(predicate);
    if(queried){++hybrid_host.query_failed;return 0;}
    uint32_t format=g_pg.trace_methods[0x208/4],pitch=g_pg.trace_methods[0x20C/4];
    uint32_t color=g_pg.trace_methods[0x210/4],depth=g_pg.trace_methods[0x214/4];
    unsigned w=g_pg.surface_clip_h>>16,h=g_pg.surface_clip_v>>16;
    NvSurface *c=NULL,*d=NULL;
    if(surface_path_enabled()&&(format&0xFFFF)==0x128&&depth&&w&&h&&
       !(g_pg.surface_clip_h&0xFFFF)&&!(g_pg.surface_clip_v&0xFFFF)){
        c=nv_surface_find_cached(&g_pg.surfaces,0,color,pitch&0xFFFF,w,h);
        d=nv_surface_find_cached(&g_pg.surfaces,1,depth,pitch>>16,w,h);
    }
    if(!c||!d||!c->rtv||!d->dsv||c->samples!=1||d->samples!=1||
       c->physical_width!=d->physical_width||c->physical_height!=d->physical_height){++hybrid_host.target_failed;return 0;}
    ID3D11RenderTargetView *rtv=NULL;ID3D11DepthStencilView *dsv=NULL;
    ID3D11DeviceContext_OMGetRenderTargets(ctx,1,&rtv,&dsv);
    int same=rtv==c->rtv&&dsv==d->dsv;
    if(rtv)ID3D11RenderTargetView_Release(rtv);if(dsv)ID3D11DepthStencilView_Release(dsv);
    D3D11_VIEWPORT viewport;UINT viewports=1;
    ID3D11DeviceContext_RSGetViewports(ctx,&viewports,&viewport);
    if(!same||viewports!=1||viewport.TopLeftX!=0||viewport.TopLeftY!=0||viewport.MinDepth!=0||viewport.MaxDepth!=1||
       viewport.Width!=(float)c->physical_width||viewport.Height!=(float)c->physical_height){++hybrid_host.target_failed;return 0;}
    out->color=c->rtv;out->depth=d->dsv;out->viewport=viewport;out->width=w;out->height=h;
    return 1;
}
static int uc_hybrid_renderer(void) {
    if(!hybrid_host.initialized){
        hybrid_host.initialized=1;wchar_t geo[4096],materials[4096],shaders[4096];
        DWORD a=GetEnvironmentVariableW(L"UCRR_HYBRID_GEO",geo,4096),b=GetEnvironmentVariableW(L"UCRR_HYBRID_MATERIALS",materials,4096);
        DWORD k=GetEnvironmentVariableW(L"UCRR_HYBRID_SHADERS",shaders,4096);
        if(a&&a<4096&&b&&b<4096&&k&&k<4096)
            hybrid_host.renderer=uc_native_control_lighting_create(d3d8_GetD3D11Device(),d3d8_GetD3D11Context(),geo,materials,shaders,hybrid_host.error,sizeof(hybrid_host.error));
        else strcpy(hybrid_host.error,"Missing hybrid GEO/material/shader paths");
        if(!hybrid_host.renderer)++hybrid_host.create_failed;
    }
    return hybrid_host.renderer!=NULL;
}
static int uc_hybrid_apply(const UcHybridLightingScene *s,const UcHybridLightingRecord *records,uint32_t count) {
    ++hybrid_host.calls;if(!count)return 1;
    UcHybridTarget target;if(!uc_hybrid_target(&target))return 0;
    float vp[16];if(!uc_hybrid_projection(vp,s->view_projection,(float)target.width,(float)target.height)){++hybrid_host.projection_failed;return 0;}
    if(!uc_hybrid_renderer())return 0;
    UCNativeControlLight lights[512];if(count>512)return 0;
    for(unsigned i=0;i<count;i++){
        const UcHybridLightingRecord *r=records+i;UCNativeControlLight *l=lights+i;
        l->source_index=r->source_index;l->radius=r->radius;l->intensity=r->intensity;
        memcpy(l->color,r->color,sizeof(l->color));l->shadows=r->shadow;l->specular=r->specular;
        for(unsigned j=0;j<3;j++)l->position[j]=(r->local[0]*r->world[j]+r->local[1]*r->world[4+j])+
            (r->local[2]*r->world[8+j]+r->local[3]*r->world[12+j]);
    }
    int ok=uc_native_control_lighting_draw(hybrid_host.renderer,target.color,target.depth,&target.viewport,vp,s->camera+12,s->exposure,
        lights,count,1,&hybrid_host.last,hybrid_host.error,sizeof(hybrid_host.error));
    d3d8_vertex_binding_invalidate();d3d8_target_binding_invalidate();d3d8_viewport_binding_invalidate();
    if(!ok){++hybrid_host.draw_failed;return 0;}
    d3d8_CustomLightingSurfaceWritten();++hybrid_host.draw_ok;
    hybrid_host.batches+=hybrid_host.last.drawn_batches;hybrid_host.lights+=hybrid_host.last.drawn_lights;
    return 1;
}
int uc_hybrid_lighting_host_apply(const UcHybridLightingScene *s,const UcHybridLightingRecord *records,uint32_t count) {
    if(!uc_hybrid_scenery_lighting_visible()){uc_hybrid_title(4);return 1;}
    int ok=uc_hybrid_apply(s,records,count);
    unsigned status=!ok?3:hybrid_host.batches?2:1;
    uc_hybrid_title(status);
    return ok;
}
void uc_hybrid_lighting_host_capture(FILE *f) {
    fprintf(f,"{\"calls\":%llu,\"drain_failed\":%llu,\"target_failed\":%llu,\"query_failed\":%llu,\"projection_failed\":%llu,\"create_failed\":%llu,\"draw_failed\":%llu,\"draw_ok\":%llu,\"batches\":%llu,\"lights\":%llu,\"error\":\"",
        hybrid_host.calls,hybrid_host.drain_failed,hybrid_host.target_failed,hybrid_host.query_failed,hybrid_host.projection_failed,
        hybrid_host.create_failed,hybrid_host.draw_failed,hybrid_host.draw_ok,hybrid_host.batches,hybrid_host.lights);
    for(const char *p=hybrid_host.error;*p;p++){if(*p=='"'||*p=='\\')fputc('\\',f);if((unsigned char)*p>=32)fputc(*p,f);}
    fprintf(f,"\",\"last_drawn_lights\":%u,\"last_drawn_batches\":%u,\"unsupported_materials\":%u,\"excluded_specular_batches\":%u,\"shadowed_lights\":%u,\"transient_shadow_omissions\":%u,\"transient_specular_omissions\":%u",
        hybrid_host.last.drawn_lights,hybrid_host.last.drawn_batches,hybrid_host.last.unsupported_materials,hybrid_host.last.excluded_specular_batches,hybrid_host.last.shadowed_lights,
        hybrid_host.last.transient_shadow_omissions,hybrid_host.last.transient_specular_omissions);
#if defined(UCRR_HYBRID_NATIVE_PARTICLES)
    fputs(",\"native_particles\":",f);uc_hybrid_particles_capture(f);
#endif
#if defined(UCRR_HYBRID_EXTENDED)
    fputs(",\"extended\":",f);uc_hybrid_extended_capture(f);
#endif
    fputc('}',f);
}
#if defined(UCRR_HYBRID_EXTENDED)
int uc_hybrid_casters_host_submit(const UcHybridLightingScene *scene,const NativeShadowCaster *casters,uint32_t count) {
    if(!scene||scene->caller!=0x9EA8Au||(count&&!casters))return 0;
    if(!count&&!hybrid_host.renderer)return 1;
    UcHybridTarget target;
    if(!uc_hybrid_target(&target)||!uc_hybrid_renderer()){
        if(hybrid_host.renderer){char ignored[1];uc_native_control_lighting_set_casters(hybrid_host.renderer,NULL,0,ignored,sizeof ignored);}
        ++hybrid_host.caster_failed;return 0;
    }
    int ok=uc_native_control_lighting_set_casters(hybrid_host.renderer,casters,count,hybrid_host.error,sizeof hybrid_host.error);
    if(ok){++hybrid_host.caster_frames;hybrid_host.caster_pairs+=count;}
    else{
        ++hybrid_host.caster_failed;
        /* Never carry a previous pose/membership across a rejected frame. */
        char ignored[1];uc_native_control_lighting_set_casters(hybrid_host.renderer,NULL,0,ignored,sizeof ignored);
    }
    return ok;
}
int uc_hybrid_lighting_host_spots(const UcHybridLightingScene *s,const UCNativeControlSpot *spots,uint32_t count) {
    if(!count)return 1;
    UcHybridTarget target;if(!s||!spots||count>512||!uc_hybrid_target(&target))return 0;
    float vp[16];if(!uc_hybrid_projection(vp,s->view_projection,(float)target.width,(float)target.height)){++hybrid_host.projection_failed;return 0;}
    if(!uc_hybrid_renderer())return 0;
    uint32_t batches=0;
    int ok=uc_native_control_spotlight_draw(hybrid_host.renderer,target.color,target.depth,&target.viewport,
        vp,s->exposure,spots,count,&batches,hybrid_host.error,sizeof hybrid_host.error);
    d3d8_vertex_binding_invalidate();d3d8_target_binding_invalidate();d3d8_viewport_binding_invalidate();
    if(ok){hybrid_host.spot_batches+=batches;if(batches)d3d8_CustomLightingSurfaceWritten();}
    else{++hybrid_host.draw_failed;uc_hybrid_title(3);}
    return ok;
}
#endif
