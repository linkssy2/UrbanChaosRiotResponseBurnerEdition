#ifndef UC_NATIVE_MODEL_VISIBILITY_H
#define UC_NATIVE_MODEL_VISIBILITY_H
#include "model_bounds.h"
#ifdef __cplusplus
extern "C" {
#endif
/* ACC50 -> AC730. Row-major source model-to-clip matrix and authored bounds.
 * Result: 0 outside, 1 intersecting, 2 wholly inside. Plane order, strict
 * comparisons and scalar operation grouping are retained. No normalization,
 * vertex-derived padding, occlusion, material selection or record publication.
 * Only bounds.minimum/maximum are consumed; inverted limits are not sorted.
 * Inputs are borrowed for the call. Output must be disjoint from inputs.
 * Finite consumed inputs and evaluated intermediates, nearest-even arithmetic,
 * no contraction/reassociation. Invalid input preserves output. FP status is
 * not part of the contract. No allocation or persistent state.
 * The explicit-plane form is AC730: replaces only the near plane. The wrapper
 * takes that plane from matrix column 2 exactly as ACC50. */
UCModelStatus uc_model_visibility_plane(const float matrix[16],
    const UCModelBounds *bounds,const float near_plane[4],uint32_t *classification);
UCModelStatus uc_model_visibility(const float matrix[16],
    const UCModelBounds *bounds,uint32_t *classification);
#ifdef __cplusplus
}
#endif
#endif
