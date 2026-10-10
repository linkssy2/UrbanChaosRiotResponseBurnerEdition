/* Opt-in experiment. Included by the actual PGRAPH owner after its draw code.
 * Capture mode retains every original draw. Replacement is approximate and
 * may only omit after resources, scene target and projection are qualified. */
#include "custom_lighting_guest.h"
#include "point_lighting.h"
#include "point_lighting_host.h"
#include "projection_contract.h"
#include "program_contract.h"
#include "nv2a_pb_live.h"
#include <wchar.h>

typedef struct UcCLHostCounts {
    uint64_t prepare_calls,prepare_ok,apply_calls,apply_ok,draws,draw_failed;
    uint64_t drains,drain_failed,reject_target,reject_program,reject_query;
    uint64_t reject_viewport,changed_target,changed_viewport,resources_failed;
    uint64_t lights_total,max_lights,late_failure;
} UcCLHostCounts;
typedef struct UcCLHostSnapshot {
    uint64_t epoch; unsigned phase,reason;
    uint32_t format,pitch,color,depth,clip_h,clip_v,physical_width,physical_height;
    unsigned query_active,program_start,program_ok;
    uint32_t program[7][4];float scale[4],bias[4],depth_max;
    float derived[16],constants104[4][4];uint8_t dirty[4];unsigned derived_ok;
    UcCustomLightingScene scene;
} UcCLHostSnapshot;
static struct {
    UCPointLightingHost *renderer;
    DWORD owner;unsigned initialized,mode,enabled;
    uint64_t birth,frequency,initialized_qpc,start_qpc,end_qpc,epoch;
    LONG owner_error;
    NvSurface *color,*depth;
    ID3D11Texture2D *color_texture,*depth_texture;
    uint32_t key[6];float scale[4],bias[4],depth_max;
    UCPointLightingFrame frame;
    UcCLHostCounts counts,start,end;
    UcCLHostSnapshot snapshots[8];unsigned next_snapshot,snapshot_count;
} uc_cl_host;
enum {UC_CL_OK,UC_CL_DRAIN,UC_CL_TARGET,UC_CL_PROGRAM,UC_CL_QUERY,UC_CL_VIEWPORT,
      UC_CL_CHANGED_TARGET,UC_CL_CHANGED_VIEWPORT,UC_CL_RESOURCE,UC_CL_DRAW};
static void uc_cl_host_init(const UcCustomLightingScene*s){
    if(uc_cl_host.initialized)return;
    uc_cl_host.initialized=1;uc_cl_host.enabled=1;uc_cl_host.mode=s->mode;
    uc_cl_host.owner=GetCurrentThreadId();LARGE_INTEGER q;
    QueryPerformanceFrequency(&q);uc_cl_host.frequency=q.QuadPart;
    QueryPerformanceCounter(&q);uc_cl_host.initialized_qpc=q.QuadPart;
    FILETIME b,e,k,u;GetProcessTimes(GetCurrentProcess(),&b,&e,&k,&u);
    uc_cl_host.birth=((uint64_t)b.dwHighDateTime<<32)|b.dwLowDateTime;
}
void uc_custom_lighting_host_init(void){
    const char*e=getenv("UCRR_CUSTOM_LIGHTING"),*m=getenv("UCRR_CUSTOM_LIGHTING_MASK");
    if(!e||strcmp(e,"1")||!m||(m[0]!='0'&&m[0]!='1')||m[1])return;
    UcCustomLightingScene s;memset(&s,0,sizeof(s));s.mode=m[0]-'0';uc_cl_host_init(&s);
}
static int uc_cl_program(void){
    return uc_custom_lighting_program_matches(g_pg.vertex_program,g_pg.transform_program_start,g_pg.transform_execution_mode);
}
static UcCLHostSnapshot *uc_cl_snapshot(const UcCustomLightingScene*s,unsigned phase){
    UcCLHostSnapshot *v=&uc_cl_host.snapshots[uc_cl_host.next_snapshot++%8];
    if(uc_cl_host.snapshot_count<8)++uc_cl_host.snapshot_count;
    memset(v,0,sizeof(*v));v->epoch=s->epoch;v->phase=phase;v->scene=*s;
    v->format=g_pg.trace_methods[0x208/4];v->pitch=g_pg.trace_methods[0x20C/4];
    v->color=g_pg.trace_methods[0x210/4];v->depth=g_pg.trace_methods[0x214/4];
    v->clip_h=g_pg.surface_clip_h;v->clip_v=g_pg.surface_clip_v;
    v->query_active=g_pg.reports.active!=NULL;v->program_start=g_pg.transform_program_start;
    if(v->program_start<=129)memcpy(v->program,g_pg.vertex_program+v->program_start,sizeof(v->program));
    memcpy(v->scale,g_pg.vertex_constants[58],16);memcpy(v->bias,g_pg.vertex_constants[59],16);
    v->depth_max=u2f(g_pg.trace_methods[NV097_SET_CLIP_MAX/4]);v->program_ok=uc_cl_program();
    memcpy(v->constants104,g_pg.vertex_constants+104,64);
    uint32_t provider=s->matrix_va;
    if(provider>=0x80000000u&&provider<0x84000000u)provider-=0x80000000u;
    if(provider>=0x10000u&&(uint64_t)provider+0x194<=g_pg.guest_memory_size&&g_pg.guest_memory){
        memcpy(v->derived,g_pg.guest_memory+provider+0x150,64);memcpy(v->dirty,g_pg.guest_memory+provider+0x190,4);
        v->derived_ok=uc_custom_lighting_projection_matches(v->derived,v->constants104,v->dirty);
    }
    return v;
}
static int uc_cl_drain(void){
    ++uc_cl_host.counts.drains;
    uint8_t *ram=g_pg.guest_memory;size_t size=g_pg.guest_memory_size;
    uint32_t device=0,flags=0,port=0;
    if(!ram||size<0x3F423Cu)goto bad;
    memcpy(&device,ram+0x3F4238,4);
    if(device<0x10000u||(uint64_t)device+0x1C24u>size)goto bad;
    memcpy(&flags,ram+device+8,4);memcpy(&port,ram+device+0x1C20,4);
    if((flags&4u)||port!=0xFD800000u)goto bad;
    if(!nv2a_pb_live_drain_reason(ram,size,0x003F4238u,NV_WAIT_OTHER))goto bad;
    /* An unterminated primitive is not a legal insertion boundary. Do not
     * manufacture END or present. Legal retained batches can now be submitted. */
    if(g_pg.in_draw)goto bad;
    gameplay_batch_flush();d3d8_BeforeHostMutation();return 1;
bad:++uc_cl_host.counts.drain_failed;return 0;
}
static int uc_cl_targets(UcCLHostSnapshot*v,NvSurface**color,NvSurface**depth){
    unsigned w=v->clip_h>>16,h=v->clip_v>>16;
    if(!surface_path_enabled()||(v->format&0xFFFF)!=0x128||!v->depth||
       (v->clip_h&0xFFFF)||(v->clip_v&0xFFFF)||!w||!h)goto bad;
    *color=nv_surface_find_cached(&g_pg.surfaces,0,v->color,v->pitch&0xFFFF,w,h);
    *depth=nv_surface_find_cached(&g_pg.surfaces,1,v->depth,v->pitch>>16,w,h);
    if(!*color||!*depth||!(*color)->rtv||!(*depth)->srv||(*color)->samples!=1||(*depth)->samples!=1||
       (*color)->physical_width!=(*depth)->physical_width||(*color)->physical_height!=(*depth)->physical_height)goto bad;
    ID3D11DeviceContext*ctx=d3d8_GetD3D11Context();ID3D11RenderTargetView*rtv=NULL;ID3D11DepthStencilView*dsv=NULL;
    if(!ctx)goto bad;ID3D11DeviceContext_OMGetRenderTargets(ctx,1,&rtv,&dsv);
    int same=rtv==(*color)->rtv&&dsv==(*depth)->dsv;
    if(rtv)ID3D11RenderTargetView_Release(rtv);if(dsv)ID3D11DepthStencilView_Release(dsv);
    if(!same)goto bad;
    v->physical_width=(*color)->physical_width;v->physical_height=(*color)->physical_height;return 1;
bad:++uc_cl_host.counts.reject_target;v->reason=UC_CL_TARGET;return 0;
}
static int uc_cl_query(UcCLHostSnapshot*v){
    ID3D11Predicate*p=NULL;BOOL value=FALSE;
    ID3D11DeviceContext_GetPredication(d3d8_GetD3D11Context(),&p,&value);
    int reject=p!=NULL||g_pg.reports.active!=NULL||g_pg.reports.failed;
    if(p)ID3D11Predicate_Release(p);
    if(reject){++uc_cl_host.counts.reject_query;v->reason=UC_CL_QUERY;return 0;}return 1;
}
static int uc_cl_frame(UcCLHostSnapshot*v,UCPointLightingFrame*f){
    if(!v->program_ok||!v->derived_ok){++uc_cl_host.counts.reject_program;v->reason=UC_CL_PROGRAM;return 0;}
    unsigned count=1;D3D11_VIEWPORT vp;memset(&vp,0,sizeof(vp));
    ID3D11DeviceContext_RSGetViewports(d3d8_GetD3D11Context(),&count,&vp);
    if(count!=1||vp.TopLeftX!=0||vp.TopLeftY!=0||vp.MinDepth!=0||vp.MaxDepth!=1||
       vp.Width!=(float)v->physical_width||vp.Height!=(float)v->physical_height||
       !uc_point_lighting_compose_inverse(f->inverse_view_projection,v->scene.inverse_view_projection,
          v->scale,v->bias,(float)(v->clip_h>>16),(float)(v->clip_v>>16),v->depth_max)){
        ++uc_cl_host.counts.reject_viewport;v->reason=UC_CL_VIEWPORT;return 0;}
    memcpy(f->camera_position,v->scene.camera_transform+12,12);f->camera_position[3]=1;
    f->image_size[0]=v->physical_width;f->image_size[1]=v->physical_height;
    f->clear_depth=1;return 1;
}
static int uc_cl_same_viewport(unsigned width,unsigned height){
    unsigned count=1;D3D11_VIEWPORT vp;memset(&vp,0,sizeof(vp));
    ID3D11DeviceContext_RSGetViewports(d3d8_GetD3D11Context(),&count,&vp);
    return count==1&&vp.TopLeftX==0&&vp.TopLeftY==0&&vp.MinDepth==0&&vp.MaxDepth==1&&
        vp.Width==(float)width&&vp.Height==(float)height;
}
int uc_custom_lighting_host_prepare(const UcCustomLightingScene*s){
    uc_cl_host_init(s);if(GetCurrentThreadId()!=uc_cl_host.owner){InterlockedIncrement(&uc_cl_host.owner_error);return 0;}
    ++uc_cl_host.counts.prepare_calls;int drained=uc_cl_drain();UcCLHostSnapshot*v=uc_cl_snapshot(s,0);
    if(!drained){v->reason=UC_CL_DRAIN;return 0;}
    NvSurface*c=NULL,*d=NULL;UCPointLightingFrame f;memset(&f,0,sizeof(f));
    if(!uc_cl_targets(v,&c,&d)||!uc_cl_frame(v,&f)||!uc_cl_query(v))return 0;
    if((!uc_cl_host.renderer&&!uc_point_lighting_host_init(&uc_cl_host.renderer,d3d8_GetD3D11Device()))||
       !uc_point_lighting_host_prepare(uc_cl_host.renderer,c->physical_width,c->physical_height,DXGI_FORMAT_B8G8R8A8_UNORM)){
        ++uc_cl_host.counts.resources_failed;v->reason=UC_CL_RESOURCE;return 0;}
    if(FAILED(uc_point_lighting_host_validate(uc_cl_host.renderer,d3d8_GetD3D11Context(),c->texture,c->rtv,d->srv,&f))){
        ++uc_cl_host.counts.resources_failed;v->reason=UC_CL_RESOURCE;return 0;}
    uc_cl_host.epoch=s->epoch;uc_cl_host.color=c;uc_cl_host.depth=d;uc_cl_host.frame=f;
    uc_cl_host.color_texture=c->texture;uc_cl_host.depth_texture=d->texture;
    ID3D11Texture2D_AddRef(c->texture);ID3D11Texture2D_AddRef(d->texture);
    uint32_t key[]={v->format,v->pitch,v->color,v->depth,v->clip_h,v->clip_v};memcpy(uc_cl_host.key,key,sizeof(key));
    memcpy(uc_cl_host.scale,v->scale,16);memcpy(uc_cl_host.bias,v->bias,16);uc_cl_host.depth_max=v->depth_max;
    ++uc_cl_host.counts.prepare_ok;return 1;
}
int uc_custom_lighting_host_apply(const UcCustomLightingScene*s,const UcCustomLightRecord*r,uint32_t count){
    if(GetCurrentThreadId()!=uc_cl_host.owner){InterlockedIncrement(&uc_cl_host.owner_error);return 0;}
    ++uc_cl_host.counts.apply_calls;int drained=uc_cl_drain();UcCLHostSnapshot*v=uc_cl_snapshot(s,1);
    if(!drained){v->reason=UC_CL_DRAIN;goto bad;}
    NvSurface*c=NULL,*d=NULL;UCPointLightingFrame f;memset(&f,0,sizeof(f));
    if(!uc_cl_targets(v,&c,&d))goto bad;
    uint32_t key[]={v->format,v->pitch,v->color,v->depth,v->clip_h,v->clip_v};
    if(s->epoch!=uc_cl_host.epoch||memcmp(key,uc_cl_host.key,sizeof(key))||c->texture!=uc_cl_host.color_texture||d->texture!=uc_cl_host.depth_texture){
        ++uc_cl_host.counts.changed_target;v->reason=UC_CL_CHANGED_TARGET;goto bad;}
    /* Suffix shader may differ. The prepare prefix established the projection;
     * constant changes, target changes and camera changes remain hard failures. */
    if(memcmp(v->scale,uc_cl_host.scale,16)||memcmp(v->bias,uc_cl_host.bias,16)||v->depth_max!=uc_cl_host.depth_max||
       !uc_cl_same_viewport(v->physical_width,v->physical_height)){
        ++uc_cl_host.counts.changed_viewport;v->reason=UC_CL_CHANGED_VIEWPORT;goto bad;}
    if(!uc_cl_query(v)||!count||count>UC_POINT_LIGHTING_MAX_LIGHTS)goto bad;
    uc_cl_host.counts.lights_total+=count;if(count>uc_cl_host.counts.max_lights)uc_cl_host.counts.max_lights=count;
    if(s->mode){
        UCPointLight lights[UC_POINT_LIGHTING_MAX_LIGHTS];for(unsigned i=0;i<count;i++)memcpy(&lights[i],&r[i].gpu,sizeof(lights[i]));
        f=uc_cl_host.frame;f.light_count=count;
        HRESULT hr=uc_point_lighting_host_draw(uc_cl_host.renderer,d3d8_GetD3D11Context(),c->texture,c->rtv,d->srv,&f,lights,count);
        d3d8_vertex_binding_invalidate();d3d8_target_binding_invalidate();d3d8_viewport_binding_invalidate();
        if(FAILED(hr)){++uc_cl_host.counts.draw_failed;v->reason=UC_CL_DRAW;goto bad;}
        d3d8_CustomLightingSurfaceWritten();++uc_cl_host.counts.draws;
    }
    ++uc_cl_host.counts.apply_ok;return 1;
bad:if(s->mode)++uc_cl_host.counts.late_failure;return 0;
}
void uc_custom_lighting_host_discard(uint64_t epoch){
    if(epoch!=uc_cl_host.epoch)return;
    if(uc_cl_host.color_texture)ID3D11Texture2D_Release(uc_cl_host.color_texture);
    if(uc_cl_host.depth_texture)ID3D11Texture2D_Release(uc_cl_host.depth_texture);
    uc_cl_host.color_texture=uc_cl_host.depth_texture=NULL;uc_cl_host.color=uc_cl_host.depth=NULL;uc_cl_host.epoch=0;
}
static void uc_cl_host_shutdown(void){
    uc_custom_lighting_host_discard(uc_cl_host.epoch);
    if(uc_cl_host.renderer)uc_point_lighting_host_shutdown(uc_cl_host.renderer);uc_cl_host.renderer=NULL;
}
static void uc_cl_host_counts(FILE*f,const UcCLHostCounts*c){
    static const char*names[]={"prepare_calls","prepare_ok","apply_calls","apply_ok","draws","draw_failed","drains","drain_failed","reject_target","reject_program","reject_query","reject_viewport","changed_target","changed_viewport","resources_failed","lights_total","max_lights","late_failure"};
    const uint64_t*n=(const uint64_t*)c;fputc('{',f);for(unsigned i=0;i<18;i++)fprintf(f,"%s\"%s\":%llu",i?",":"",names[i],(unsigned long long)n[i]);fputc('}',f);
}
static void uc_cl_host_float(FILE*f,const float*v,unsigned n){
    fputc('[',f);for(unsigned i=0;i<n;i++){if(i)fputc(',',f);if(isfinite(v[i]))fprintf(f,"%.9g",v[i]);else fputs("null",f);}fputc(']',f);
}
void uc_custom_lighting_host_capture_boundary(int start,uint64_t qpc){
    if(!uc_cl_host.enabled||GetCurrentThreadId()!=uc_cl_host.owner)return;
    if(start){uc_cl_host.start_qpc=qpc;uc_cl_host.start=uc_cl_host.counts;return;}
    uc_cl_host.end_qpc=qpc;uc_cl_host.end=uc_cl_host.counts;
    wchar_t path[MAX_PATH];DWORD n=GetModuleFileNameW(NULL,path,MAX_PATH);if(!n||n>=MAX_PATH)return;
    wchar_t*slash=wcsrchr(path,L'\\');if(!slash||slash-path>MAX_PATH-80)return;
    swprintf(slash+1,MAX_PATH-(slash+1-path),L"custom-lighting-host-%lu.json",GetCurrentProcessId());
    FILE*f=_wfopen(path,L"wb");if(!f)return;LARGE_INTEGER now;QueryPerformanceCounter(&now);
    fprintf(f,"{\"schema\":1,\"experiment\":\"custom-lighting-host\",\"pid\":%lu,\"process_start_filetime_utc\":\"%llu\",\"enabled\":true,\"mask\":%u,\"immutable_mode\":true,\"owner_tid\":%lu,\"owner_error\":%ld,\"qpc_frequency\":%llu,\"initialized_qpc\":%llu,\"snapshot_qpc\":%llu,\"counters\":",GetCurrentProcessId(),(unsigned long long)uc_cl_host.birth,uc_cl_host.mode,uc_cl_host.owner,uc_cl_host.owner_error,(unsigned long long)uc_cl_host.frequency,(unsigned long long)uc_cl_host.initialized_qpc,(unsigned long long)now.QuadPart);
    uc_cl_host_counts(f,&uc_cl_host.end);fprintf(f,",\"capture_window\":{\"start_qpc\":%llu,\"end_qpc\":%llu,\"start_counts\":",(unsigned long long)uc_cl_host.start_qpc,(unsigned long long)uc_cl_host.end_qpc);
    uc_cl_host_counts(f,&uc_cl_host.start);fputs(",\"end_counts\":",f);uc_cl_host_counts(f,&uc_cl_host.end);fputs("},\"snapshots\":[",f);
    for(unsigned i=0;i<uc_cl_host.snapshot_count;i++){
        const UcCLHostSnapshot*v=&uc_cl_host.snapshots[(uc_cl_host.next_snapshot-uc_cl_host.snapshot_count+i)%8];
        fprintf(f,"%s{\"epoch\":%llu,\"phase\":%u,\"error_reason\":%u,\"format\":%u,\"pitch\":%u,\"color\":%u,\"depth\":%u,\"clip_h\":%u,\"clip_v\":%u,\"physical_width\":%u,\"physical_height\":%u,\"query_active\":%u,\"program_start\":%u,\"program_ok\":%u,\"program\":[",i?",":"",(unsigned long long)v->epoch,v->phase,v->reason,v->format,v->pitch,v->color,v->depth,v->clip_h,v->clip_v,v->physical_width,v->physical_height,v->query_active,v->program_start,v->program_ok);
        for(unsigned j=0;j<28;j++)fprintf(f,"%s%u",j?",":"",((const uint32_t*)v->program)[j]);
        fputs("],\"scale\":",f);uc_cl_host_float(f,v->scale,4);fputs(",\"bias\":",f);uc_cl_host_float(f,v->bias,4);
        fputs(",\"depth_max\":",f);uc_cl_host_float(f,&v->depth_max,1);fputs(",\"camera_transform\":",f);uc_cl_host_float(f,v->scene.camera_transform,16);
        fputs(",\"view_projection\":",f);uc_cl_host_float(f,v->scene.view_projection,16);fputs(",\"inverse_view_projection\":",f);uc_cl_host_float(f,v->scene.inverse_view_projection,16);
        fprintf(f,",\"derived_ok\":%u,\"dirty\":[%u,%u,%u,%u],\"derived\":",v->derived_ok,v->dirty[0],v->dirty[1],v->dirty[2],v->dirty[3]);
        uc_cl_host_float(f,v->derived,16);fputs(",\"constants104\":",f);uc_cl_host_float(f,&v->constants104[0][0],16);fputc('}',f);
    }fputs("]}\n",f);fclose(f);
}
