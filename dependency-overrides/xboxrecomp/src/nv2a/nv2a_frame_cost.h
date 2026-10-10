#ifndef NV2A_FRAME_COST_H
#define NV2A_FRAME_COST_H
#include <stdint.h>
#include <string.h>
/* Single decoder owner. Sample whole present-to-present intervals, not every
 * vertex. Exclusive OUTER drain stages; recursive drains remain in their
 * caller's stage. No allocations, guest reads, waits or logging in this ledger.
 * Uninstrumented/over-budget work remains explicitly unassigned. */
enum { NV_COST_COMMANDS, NV_COST_REPORTS, NV_COST_FLUSH, NV_COST_PRESENT,
       NV_COST_INDEX_PACKET, NV_COST_CONSTANT_PACKET, NV_COST_SUBMIT, NV_COST_TEXTURE,
       NV_COST_LIGHTING, NV_COST_UPLOAD, NV_COST_HOST_DRAW,
       NV_COST_CPU_IMMEDIATE, NV_COST_CPU_GPU_FIRST, NV_COST_CPU_INDEXED,
       NV_COST_CPU_INLINE, NV_COST_CPU_OTHER, NV_COST_SOFTWARE_METHOD, NV_COST_SLOTS };
#define NV_COST_MAX_ROOTS 8192u
/* Schema3 lighting/upload/draw children exceed 16K in the starting-van trace.
 * Keep a finite budget, but allow those children to finish attribution. */
#define NV_COST_MAX_SCOPES 32768u
typedef struct NvFrameCost {
    uint64_t (*clock)(void);
    uint64_t begin, cursor, ticks[NV_COST_SLOTS], total, unassigned;
    uint64_t next_frame;
    unsigned enabled, active, depth, stage, samples, roots, omitted, errors;
    unsigned random;
    unsigned scopes,scope_depth,scope_omitted,clocks;
} NvFrameCost;
/* Defined by the live decoder; renderer wrappers share its owner-thread ledger. */
extern NvFrameCost nv_frame_cost_shared;
static uint64_t nv_frame_cost_clock_read(NvFrameCost*s){++s->clocks;return s->clock();}
static void nv_frame_cost_init(NvFrameCost *s,int enabled,uint64_t (*clock)(void)) {
    memset(s,0,sizeof(*s)); s->enabled=!!enabled;s->clock=clock;s->random=0x91E10DA5u;
}
static unsigned nv_frame_cost_enter(NvFrameCost *s) {
    if(!s->active)return 0;
    if(s->depth)return ++s->depth;
    if(s->roots>=NV_COST_MAX_ROOTS){++s->omitted;return 0;}
    ++s->roots;s->depth=1;s->stage=NV_COST_COMMANDS;s->cursor=nv_frame_cost_clock_read(s);return 1;
}
static void nv_frame_cost_stage(NvFrameCost *s,unsigned token,unsigned stage) {
    if(token!=1)return;
    uint64_t now=nv_frame_cost_clock_read(s);
    if(s->depth!=1||stage>=NV_COST_SLOTS||now<s->cursor){++s->errors;return;}
    s->ticks[s->stage]+=now-s->cursor;s->cursor=now;s->stage=stage;
}
static void nv_frame_cost_leave(NvFrameCost *s,unsigned token) {
    if(!token)return;
    if(s->depth!=token){++s->errors;return;}
    if(token==1){if(s->scope_depth)++s->errors;nv_frame_cost_stage(s,token,s->stage);}
    --s->depth;
}
/* Nested exclusive child scopes; no clocks outside a sampled outer drain.
 * A recursive decoder drain stays in the caller's category. Omitted children
 * stay in the parent and are disclosed rather than extrapolated. */
static unsigned nv_frame_cost_push(NvFrameCost*s,unsigned category){
    if(!s->active||s->depth!=1)return 0;
    if(category>=NV_COST_SLOTS){++s->errors;return 0;}
    if(s->scopes>=NV_COST_MAX_SCOPES||s->scope_depth>=16){++s->scope_omitted;return 0;}
    unsigned previous=s->stage;
    ++s->scopes;++s->scope_depth;
    nv_frame_cost_stage(s,1,category);
    return (s->scope_depth<<8)|(previous+1);
}
static void nv_frame_cost_pop(NvFrameCost*s,unsigned token){
    if(!token)return;
    if(s->depth!=1||s->scope_depth!=(token>>8)||!(token&255)||((token&255)-1)>=NV_COST_SLOTS){++s->errors;return;}
    nv_frame_cost_stage(s,1,(token&255)-1);--s->scope_depth;
}
/* Call after the outer present returns. A ready signal only gates sampling;
 * it is NOT evidence of a visible gameplay scene. Returns a completed sample.
 * Next interval is scheduled 49..80 boundaries later, at most 128 samples. */
static int nv_frame_cost_boundary(NvFrameCost *s,uint64_t frame,int ready) {
    if(!s->enabled||s->samples>=128)return 0;
    if(s->depth||s->scope_depth){++s->errors;return 0;}
    if(s->active){
        uint64_t now=nv_frame_cost_clock_read(s),sum=0;
        for(unsigned i=0;i<NV_COST_SLOTS;++i)sum+=s->ticks[i];
        s->total=now>=s->begin?now-s->begin:0;
        if(now<s->begin||sum>s->total){++s->errors;s->unassigned=0;}
        else s->unassigned=s->total-sum;
        s->active=0;++s->samples;
        s->random=s->random*1664525u+1013904223u;
        s->next_frame=frame+49u+(s->random>>27);
        return 1;
    }
    if(ready&&frame>=s->next_frame){
        memset(s->ticks,0,sizeof(s->ticks));
        s->roots=s->omitted=s->errors=0;s->scopes=s->scope_depth=s->scope_omitted=s->clocks=0;
        s->begin=nv_frame_cost_clock_read(s);s->active=1;
    }
    return 0;
}
#endif
