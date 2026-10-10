#ifndef UC_CONTROL_MODEL_RECEIVER_H
#define UC_CONTROL_MODEL_RECEIVER_H
#include "model_render.h"
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCControlModelVertex { float position[3],normal[3]; } UCControlModelVertex;
/* Decode only the index-only buffer emitted by B4CA0/38590D. The final word is
 * the mutable call-return slot, not geometry. No jumps/calls are followed.
 * Triangle strips expand in original alternating order, retaining degenerates.
 * All indices must be below vertex_count. NULL output queries the required
 * triangle-corner count; otherwise capacity is in uint32_t elements. Failure
 * publishes neither output nor count. Input/output must not overlap. */
int uc_control_model_indices(const void *pushbuffer,size_t bytes,uint32_t vertex_count,
    uint32_t *output,size_t capacity,size_t *count);
/* Same validated buffer contract, retaining source list(5)/strip(6) topology
 * and original ordered indices rather than expanding strips to triangle corners.
 * Failure leaves both count and primitive unchanged. */
int uc_control_model_indexed(const void *pushbuffer,size_t bytes,uint32_t vertex_count,
    uint32_t *output,size_t capacity,size_t *count,uint32_t *primitive);
/* Live stream0 is ten bytes: normalized SHORT3 plus packed 11/11/10 normal.
 * Stream4 is eight bytes: three signed16 palette offsets, two byte weights.
 * Caller supplies the current ordered palette, not a resampled/bind pose.
 * Reuses source-qualified skinning, including negative third weights and the
 * unnormalized transformed normal. Result remains in RECEIVER LOCAL space;
 * world/camera/light transforms and material admission remain caller-owned.
 * All spans are borrowed for the call only. Failure leaves output unchanged. */
int uc_control_model_vertex(const void *position10,const void *skin8,
    const UCModelRenderPalette *,UCControlModelVertex *);
#ifdef __cplusplus
}
#endif
#endif
