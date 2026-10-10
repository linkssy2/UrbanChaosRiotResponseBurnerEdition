#ifndef NV2A_FRAME_COST_BURST_H
#define NV2A_FRAME_COST_BURST_H
#include "nv2a_frame_cost.h"
/* Owner-thread scheduling only. The UI request is transferred atomically by
 * the caller. Never discard an open scope or reset an in-flight sample. */
typedef struct NvFrameCostBurst {unsigned enabled,pending,remaining,requests;} NvFrameCostBurst;
static int nv_frame_cost_burst_begin(NvFrameCostBurst*b,NvFrameCost*s,uint64_t frame){
 if(!b->enabled||!b->pending||s->active||s->depth||s->scope_depth)return 0;
 b->pending=0;
 if(b->requests>=32)return 0;
 ++b->requests;b->remaining=16;s->enabled=1;s->samples=0;s->next_frame=frame;
 return 1;
}
static void nv_frame_cost_burst_end(NvFrameCostBurst*b,NvFrameCost*s,uint64_t frame){
 if(!b->enabled||!b->remaining)return;
 if(!--b->remaining)s->enabled=0;
 else s->next_frame=frame+3;
}
#endif
