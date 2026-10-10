#ifndef NV2A_CPU_VERTEX_COST_H
#define NV2A_CPU_VERTEX_COST_H
#include "nv2a_frame_cost.h"
/* CPU evaluator calls only, never the bulk GPU attribute-capture loop.
 * Shares the bounded sampled-frame budget. No clocks outside an active root;
 * omitted calls remain in their parent and increment scope_omitted. */
#define NV_CPU_VERTEX_COST_BEGIN(route) nv_frame_cost_push(&nv_frame_cost_shared, \
    NV_COST_CPU_IMMEDIATE+((route)<4?(route):4))
#define NV_CPU_VERTEX_COST_END(token) nv_frame_cost_pop(&nv_frame_cost_shared,(token))
#endif
