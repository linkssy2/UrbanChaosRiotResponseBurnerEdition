#ifndef UC_DRAW_PREPARATION_CENSUS_H
#define UC_DRAW_PREPARATION_CENSUS_H
/* Observation only. Exact value recurrence is not a resource-lifetime token,
 * skipped preparation, or an FPS measurement. Runtime hooks are PGRAPH-local. */
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#define UC_DRAW_CENSUS_WAYS 64u
#define UC_DRAW_CENSUS_LIMIT 65536u

typedef struct UcDrawDescription {
    uint32_t program[136][4], formats[16];
    uint32_t combiner[55], prefix[23], render[23], sampler[4][14];
    uint32_t texture[4][5], conversion[24], viewport[8], route[8];
} UcDrawDescription;
typedef struct UcDrawCensusEntry {
    uint64_t hash, age;
    UcDrawDescription key;
} UcDrawCensusEntry;
typedef struct UcDrawCensusBin {
    uint64_t prefix, textures, completed, raw_rejected;
    uint64_t selected, observed, capped, hits, misses, adjacent, evictions;
    uint64_t query_observed, typed_observed;
    uint64_t first_frame, last_frame;
    unsigned previous;
    UcDrawCensusEntry entries[UC_DRAW_CENSUS_WAYS];
} UcDrawCensusBin;

static uint64_t uc_draw_description_hash(const UcDrawDescription *key) {
    const unsigned char *p=(const unsigned char*)key;
    uint64_t h=UINT64_C(14695981039346656037);
    for(unsigned i=0;i<sizeof(*key);++i)h=(h^p[i])*UINT64_C(1099511628211);
    return h;
}
/* Full equality verifies hash candidates. Independent per-bin dictionaries
 * describe the sampled stream, never a real 64-entry cache over every draw. */
static void uc_draw_census_observe(UcDrawCensusBin *b,const UcDrawDescription *key,
    uint64_t frame,unsigned query,unsigned typed) {
    uint64_t h=uc_draw_description_hash(key), oldest=UINT64_MAX;
    unsigned found=UC_DRAW_CENSUS_WAYS, victim=0;
    for(unsigned i=0;i<UC_DRAW_CENSUS_WAYS;++i){
        UcDrawCensusEntry *e=&b->entries[i];
        if(e->age&&e->hash==h&&!memcmp(&e->key,key,sizeof(*key))){found=i;break;}
        if(e->age<oldest){oldest=e->age;victim=i;}
    }
    if(!b->observed)b->first_frame=frame;
    b->last_frame=frame;++b->observed;
    if(query)++b->query_observed;if(typed)++b->typed_observed;
    if(found!=UC_DRAW_CENSUS_WAYS){
        ++b->hits;if(b->observed>1&&found==b->previous)++b->adjacent;
    }else{
        ++b->misses;found=victim;
        if(b->entries[found].age)++b->evictions;
        b->entries[found].hash=h;b->entries[found].key=*key;
    }
    b->entries[found].age=b->observed;b->previous=found;
}
static int uc_draw_census_selected(uint64_t ordinal) {
    return (ordinal&63u)==1u;
}

#ifndef UC_DRAW_CENSUS_CORE_ONLY
#if defined(UCRR_LIGHTING_LIST_PROBE) && UCRR_LIGHTING_LIST_PROBE && !UCRR_PLAY_ONLY
#include <stdlib.h>
#include <wchar.h>
#include "guest_lighting_coverage.h"
static struct {
    unsigned initialized, enabled, active, finished, pending, in_light, raw;
    DWORD owner;
    uint64_t start_qpc,end_qpc,entries,nested,owner_errors;
    uint32_t slope,units;
    UcDrawCensusBin bin[2];
} uc_draw_census;

static int uc_draw_census_owner(void) {
    if(!uc_draw_census.active)return 0;
    if(GetCurrentThreadId()!=uc_draw_census.owner){++uc_draw_census.owner_errors;return 0;}
    return 1;
}
static void uc_draw_census_enter(void) {
    if(!uc_draw_census_owner())return;
    ++uc_draw_census.entries;
    if(uc_draw_census.pending)++uc_draw_census.nested;
    uc_draw_census.pending=0;uc_draw_census.raw=0;
}
static void uc_draw_census_prefix(int direct_gpu,float slope,int units) {
    if(!uc_draw_census_owner()||!direct_gpu||!g_pg.indexed_draw||g_pg.draw_mode!=6||
       raw_draw_retain_enabled()||nv_removal.enabled)return;
    uc_draw_census.pending=1;
    uc_draw_census.in_light=!!uc_guest_light_in_traversal();
    memcpy(&uc_draw_census.slope,&slope,4);memcpy(&uc_draw_census.units,&units,4);
    ++uc_draw_census.bin[0].prefix;
    if(uc_draw_census.in_light)++uc_draw_census.bin[1].prefix;
}
static void uc_draw_census_textures(void) {
    if(!uc_draw_census_owner()||uc_draw_census.pending!=1)return;
    uc_draw_census.pending=2;++uc_draw_census.bin[0].textures;
    if(uc_draw_census.in_light)++uc_draw_census.bin[1].textures;
}
static void uc_draw_census_raw_active(int active) {
    if(uc_draw_census_owner()&&uc_draw_census.pending)uc_draw_census.raw=!!active;
}
static void uc_draw_description_capture(UcDrawDescription *k,const float conversion[6][4]) {
    memset(k,0,sizeof(*k));
    memcpy(k->program,g_pg.vertex_program,sizeof(k->program));
    memcpy(k->formats,g_pg.array_format,sizeof(k->formats));
    /* Exact raw combiner source, including unused stages/colors, conservatively
     * distinguishes more inputs than the compiled shader may consume. */
    static const unsigned spans[]={0x260,0xAC0,0xAA0,0x1E40,0xA60,0xA80};
    for(unsigned s=0;s<6;++s)memcpy(k->combiner+8*s,g_pg.trace_methods+spans[s]/4,32);
    static const unsigned combiner_tail[]={0x1E60,0x288,0x28C,0x1E70,0x1E78,0x1E20,0x1E24};
    for(unsigned i=0;i<7;++i)k->combiner[48+i]=g_pg.trace_methods[combiner_tail[i]/4];
    static const unsigned prefix_methods[]={0x354,0x35C,0x32C,0x364,0x368,0x36C,0x360,
        0x370,0x374,0x378,0x39C,0x3A0,0x2A8,0x330};
    for(unsigned i=0;i<14;++i)k->prefix[i]=g_pg.trace_methods[prefix_methods[i]/4];
    uint32_t mirrors[]={g_pg.depth_test,g_pg.cull_enable,g_pg.blend_enable,g_pg.blend_sfactor,
        g_pg.blend_dfactor,g_pg.alpha_test,g_pg.alpha_func,g_pg.alpha_ref,g_pg.color_mask};
    memcpy(k->prefix+14,mirrors,sizeof(mirrors));
    /* These are all render-state inputs of current states_apply and raw PS
     * constant preparation, including inherited values absent in old probes. */
    static const unsigned rs_index[]={D3DRS_ALPHABLENDENABLE,D3DRS_SRCBLEND,D3DRS_DESTBLEND,
        D3DRS_BLENDOP,D3DRS_COLORWRITEENABLE,D3DRS_ZENABLE,D3DRS_ZWRITEENABLE,D3DRS_ZFUNC,
        D3DRS_STENCILENABLE,D3DRS_STENCILMASK,D3DRS_STENCILWRITEMASK,D3DRS_STENCILFUNC,
        D3DRS_STENCILFAIL,D3DRS_STENCILZFAIL,D3DRS_STENCILPASS,D3DRS_STENCILREF,
        D3DRS_FILLMODE,D3DRS_CULLMODE,D3DRS_FOGCOLOR,D3DRS_ALPHAREF,D3DRS_ALPHAFUNC,
        D3DRS_ALPHATESTENABLE,D3DRS_FOGENABLE};
    const DWORD *rs=d3d8_GetRenderStates();
    for(unsigned i=0;i<23;++i)k->render[i]=rs[rs_index[i]];
    static const unsigned tss_index[]={D3DTSS_MAGFILTER,D3DTSS_MINFILTER,D3DTSS_MIPFILTER,
        D3DTSS_ADDRESSU,D3DTSS_ADDRESSV,D3DTSS_MAXANISOTROPY,D3DTSS_MIPMAPLODBIAS,
        D3DTSS_COLOROP,D3DTSS_COLORARG1,D3DTSS_COLORARG2,D3DTSS_ALPHAOP,D3DTSS_ALPHAARG1,D3DTSS_ALPHAARG2};
    for(unsigned t=0;t<4;++t){
        const DWORD *ts=d3d8_GetTSS(t);
        for(unsigned i=0;i<13;++i)k->sampler[t][i]=ts[tss_index[i]];
        /* ADDRESSW is private because PC enum25 conflicts with Xbox COLORSIGN.
         * PGRAPH always establishes W from this exact input before this hook. */
        k->sampler[t][13]=g_pg.tex[t].address;
        uint32_t texture[]={g_pg.tex[t].format,g_pg.tex[t].image_rect,g_pg.tex[t].control0,
            g_pg.tex[t].filter,g_pg.tex[t].host_levels};
        memcpy(k->texture[t],texture,sizeof(texture));
    }
    memcpy(k->conversion,conversion,sizeof(k->conversion));
    memcpy(k->viewport,g_pg.vp_scale,16);memcpy(k->viewport+4,g_pg.vp_offset,16);
    k->route[0]=g_pg.transform_program_start;k->route[1]=g_pg.transform_execution_mode;
    k->route[2]=g_pg.trace_methods[0x208/4];k->route[3]=d3d8_GetHostMsaa();
    k->route[4]=uc_draw_census.slope;k->route[5]=uc_draw_census.units;
    k->route[6]=_mm_getcsr();k->route[7]=!!surface_path_enabled();
}
static void uc_draw_census_complete(const float conversion[6][4]) {
    if(!uc_draw_census_owner()||uc_draw_census.pending!=2)return;
    uc_draw_census.pending=3;
    for(unsigned b=0;b<=uc_draw_census.in_light;++b)++uc_draw_census.bin[b].completed;
    if(!uc_draw_census.raw){
        for(unsigned b=0;b<=uc_draw_census.in_light;++b)++uc_draw_census.bin[b].raw_rejected;
        return;
    }
    /* Independent ordinal per bin: light observations remain distributed even
     * if their phase is locked to a multiple of the all-draw sample interval. */
    unsigned take[2]={0,0},any=0;
    for(unsigned b=0;b<=uc_draw_census.in_light;++b){
        UcDrawCensusBin *bin=&uc_draw_census.bin[b];
        if(!uc_draw_census_selected(bin->completed))continue;
        ++bin->selected;
        if(bin->observed>=UC_DRAW_CENSUS_LIMIT){++bin->capped;continue;}
        take[b]=1;any=1;
    }
    if(!any)return;
    UcDrawDescription key;uc_draw_description_capture(&key,conversion);
    unsigned typed=0;
    for(unsigned t=0;t<4;++t){unsigned m=(g_pg.shader_stage_program>>(5*t))&31;typed|=(m==2||m==3);}
    for(unsigned b=0;b<2;++b)if(take[b])uc_draw_census_observe(&uc_draw_census.bin[b],&key,
        g_pg.stats.frames,g_pg.reports.active!=NULL,typed);
}
static void uc_draw_census_leave(void) {
    if(uc_draw_census_owner())uc_draw_census.pending=0;
}
static void uc_draw_census_write_bin(FILE *f,const UcDrawCensusBin *b) {
    fprintf(f,"{\"prefix\":%llu,\"textures\":%llu,\"completed\":%llu,\"raw_rejected\":%llu,"
        "\"selected\":%llu,\"observed\":%llu,\"capped\":%llu,\"sampled_lru_hits\":%llu,"
        "\"sampled_lru_misses\":%llu,\"sampled_adjacent_equal\":%llu,\"evictions\":%llu,"
        "\"query_observed\":%llu,\"typed_observed\":%llu,\"first_frame\":%llu,\"last_frame\":%llu}",
        (unsigned long long)b->prefix,(unsigned long long)b->textures,(unsigned long long)b->completed,
        (unsigned long long)b->raw_rejected,(unsigned long long)b->selected,(unsigned long long)b->observed,
        (unsigned long long)b->capped,(unsigned long long)b->hits,(unsigned long long)b->misses,
        (unsigned long long)b->adjacent,(unsigned long long)b->evictions,(unsigned long long)b->query_observed,
        (unsigned long long)b->typed_observed,(unsigned long long)b->first_frame,(unsigned long long)b->last_frame);
}
void uc_draw_census_boundary(int start,uint64_t qpc) {
    if(!uc_draw_census.initialized){
        const char *v=getenv("UCRR_LIGHTING_LIST_PROBE");
        uc_draw_census.enabled=v&&!strcmp(v,"1");uc_draw_census.initialized=1;
    }
    if(!uc_draw_census.enabled)return;
    if(start){
        if(uc_draw_census.active||uc_draw_census.finished)return;
        memset(uc_draw_census.bin,0,sizeof(uc_draw_census.bin));
        uc_draw_census.entries=uc_draw_census.nested=uc_draw_census.owner_errors=0;
        uc_draw_census.pending=0;uc_draw_census.owner=GetCurrentThreadId();
        uc_draw_census.start_qpc=qpc;uc_draw_census.end_qpc=0;uc_draw_census.active=1;return;
    }
    if(!uc_draw_census_owner())return;
    uc_draw_census.active=0;uc_draw_census.finished=1;uc_draw_census.end_qpc=qpc;uc_draw_census.pending=0;
    wchar_t path[MAX_PATH];DWORD n=GetModuleFileNameW(NULL,path,MAX_PATH);
    if(!n||n>=MAX_PATH){fprintf(stderr,"[DRAW-CENSUS] output path unavailable\n");return;}
    wchar_t *slash=wcsrchr(path,L'\\');
    if(!slash||slash-path>MAX_PATH-96){fprintf(stderr,"[DRAW-CENSUS] output path too long\n");return;}
    swprintf(slash+1,MAX_PATH-(slash+1-path),L"draw-preparation-census-%lu-%llu.json",
        GetCurrentProcessId(),(unsigned long long)uc_draw_census.start_qpc);
    FILE *f=_wfopen(path,L"wbx");
    if(!f){fprintf(stderr,"[DRAW-CENSUS] exclusive output creation failed\n");return;}
    fprintf(f,"{\"schema\":1,\"observer_only\":true,\"pid\":%lu,\"owner_tid\":%lu,"
        "\"start_qpc\":%llu,\"end_qpc\":%llu,\"entries\":%llu,\"nested\":%llu,\"owner_errors\":%llu,"
        "\"key_bytes\":%u,\"sample_stride\":64,\"sample_cap_per_bin\":%u,\"all\":",
        GetCurrentProcessId(),uc_draw_census.owner,(unsigned long long)uc_draw_census.start_qpc,
        (unsigned long long)uc_draw_census.end_qpc,(unsigned long long)uc_draw_census.entries,
        (unsigned long long)uc_draw_census.nested,(unsigned long long)uc_draw_census.owner_errors,
        (unsigned)sizeof(UcDrawDescription),UC_DRAW_CENSUS_LIMIT);
    uc_draw_census_write_bin(f,&uc_draw_census.bin[0]);fputs(",\"inside_98DC0\":",f);
    uc_draw_census_write_bin(f,&uc_draw_census.bin[1]);fputs("}\n",f);
    int failed=ferror(f);if(fclose(f)!=0)failed=1;
    if(failed)fprintf(stderr,"[DRAW-CENSUS] output write/close failed; evidence invalid\n");
}
#else
#define uc_draw_census_enter() ((void)0)
#define uc_draw_census_prefix(d,s,u) ((void)0)
#define uc_draw_census_textures() ((void)0)
#define uc_draw_census_raw_active(a) ((void)0)
#define uc_draw_census_complete(c) ((void)0)
#define uc_draw_census_leave() ((void)0)
void uc_draw_census_boundary(int start,uint64_t qpc){(void)start;(void)qpc;}
#endif
#endif
#endif
