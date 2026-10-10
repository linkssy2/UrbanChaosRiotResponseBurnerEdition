#ifndef UC_NATIVE_MODEL_BOUNDS_H
#define UC_NATIVE_MODEL_BOUNDS_H
#include "model_asset.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCModelBounds { float minimum[3],maximum[3],center[3],extent[3]; } UCModelBounds;
/* B8FE0: compose bind correction with supplied bone world (no position_scale),
 * transform each authored box, union in file order. With no bound records,
 * retain the constructor's source-group bounds. No vertex-derived substitute.
 * Loaded immutable model; world_count equals bone_count. Optional validity is
 * one byte/bone; NULL asserts every supplied world matrix is established.
 * Only referenced worlds are read. Finite inputs and intermediate/output domain,
 * nearest-even FP, no contraction/reassociation. Failure preserves output.
 * Output is caller-owned and must not overlap the model or its input arrays.
 * This supplies bounds, not culling, draw sorting or a pose update. */
UCModelStatus uc_model_bounds(const UCModel *model,const float (*world)[16],
    size_t world_count,const uint8_t *world_valid,UCModelBounds *out);
#ifdef __cplusplus
}
#endif
#endif
