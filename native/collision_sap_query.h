#ifndef UC_NATIVE_COLLISION_SAP_QUERY_H
#define UC_NATIVE_COLLISION_SAP_QUERY_H
#include "collision_sap.h"
/* Read-only3251E0/324100 stationary unpartitioned query over the SAME attached
 * SAP view. Selected must be the exact attached handle. Appends (selected,other)
 * in ascending CURRENT node index, excluding selected/sentinel. No float bounds
 * reconstruction, re-quantization, allocation, callbacks or retained references.
 * Readable view/node/endpoint/handle storage and all identities remain stable
 * through this call and pair consumption. Never mutate through the view.
 * At most16383 bodies. Structural validation checks endpoint order, identities,
 * handle indices and node/endpoint backlinks before querying. Calibration fields
 * are unused; float mode does not affect this integer-only operation. Pointer
 * readability/lifetimes remain caller obligations, not discoverable from C data.
 * Pair descriptor/storage must be disjoint from the view and all input storage.
 * Invalid input or insufficient capacity preserves owner, handles, full output
 * storage and its count. Prefix pairs are preserved verbatim. No dynamic or
 * partition-marker view, concurrent mutation or whole-world refresh is supplied. */
#ifdef __cplusplus
extern "C" {
#endif
UCCollisionStatus uc_collision_sap_query(const UCCollisionSAPView *,
    const UCCollisionSAPHandle *selected,UCCollisionSAPPairs *pairs);
#ifdef __cplusplus
}
#endif
#endif
