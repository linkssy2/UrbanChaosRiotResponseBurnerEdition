#ifndef UC_NATIVE_COLLISION_SAP_H
#define UC_NATIVE_COLLISION_SAP_H
#include "collision_asset.h"

#define UC_COLLISION_SAP_DETACHED UINT32_MAX
#define UC_COLLISION_SAP_MAX_BODIES 16383u

typedef struct UCCollisionSAPHandle {
    const void *identity;
    uint32_t index;
} UCCollisionSAPHandle;
typedef struct UCCollisionSAPEndpoint { uint16_t value, node; } UCCollisionSAPEndpoint;
typedef struct UCCollisionSAPNode {
    uint16_t min[3], max[3]; /* Endpoint indices, in XYZ order. */
    UCCollisionSAPHandle *handle;
} UCCollisionSAPNode;
typedef struct UCCollisionSAPView {
    float offset_low[4], offset_high[4], scale[4];
    float bias;
    uint32_t node_count; /* Includes node0 sentinel. */
    const UCCollisionSAPNode *nodes;
    const UCCollisionSAPEndpoint *axis[3];
    uint32_t endpoint_count; /* 2*node_count on each axis. */
} UCCollisionSAPView;
typedef struct UCCollisionSAPPair {
    const UCCollisionSAPHandle *first, *second;
} UCCollisionSAPPair;
typedef struct UCCollisionSAPPairs {
    UCCollisionSAPPair *data;
    uint32_t count, capacity; /* Append after existing pairs. */
} UCCollisionSAPPairs;
typedef struct UCCollisionSAPBounds { float lower[4], upper[4]; } UCCollisionSAPBounds;
typedef struct UCCollisionSAP UCCollisionSAP;

/* Stationary unpartitioned 327760/325350/325690 slice. Arrays are owned;
 * handles and their identity owners are BORROWED, stable and alive through
 * removal/free and consumption of pair output. The real world/registry retains
 * those owners. This module neither registers a world body nor sends pair
 * notifications. No dynamic updates, partition markers or concurrent mutation.
 * Initialize each detached handle as {identity,UC_COLLISION_SAP_DETACHED}.
 * Create requires finite float4 bounds, strictly increasing XYZ with finite
 * range/scale. Add accepts finite float4 ordered XYZ AABBs (W is ignored).
 * SSE/x87 round-to-nearest, DAZ/FTZ off are required and checked. Body capacity
 * is1..16383 (source signed16 endpoint-index comparisons); persistent arrays are
 * allocated at create. Failed calls leave owner, handles,
 * pair output and output pointers unchanged. Single-operation pairs append in
 * source node order.
 * Caller-supplied output storage must not alias the owner or handles.
 * Readonly views remain valid until the next mutation/free. No public writes
 * to attached handles/view arrays. Free invalidates attached handle indices. */
#ifdef __cplusplus
extern "C" {
#endif
UCCollisionStatus uc_collision_sap_create(const float lower[4], const float upper[4],
    uint32_t body_capacity, UCCollisionSAP **out);
void uc_collision_sap_free(UCCollisionSAP *sap);
const UCCollisionSAPView *uc_collision_sap_view(const UCCollisionSAP *sap);
UCCollisionStatus uc_collision_sap_add(UCCollisionSAP *sap, UCCollisionSAPHandle *handle,
    const float lower[4], const float upper[4], UCCollisionSAPPairs *pairs);
UCCollisionStatus uc_collision_sap_remove(UCCollisionSAP *sap, UCCollisionSAPHandle *handle,
    UCCollisionSAPPairs *pairs);
/* 328BE0/3290E0 batch operations on this same owner. Input handles and identities
 * must be distinct; add identities must also differ from every attached identity.
 * Add appends nodes in input order, uses original value-only endpoint quicksort
 * and new-before-old merge ties. Pair output follows327BF0's X sweep and active
 * selected/unselected swap-last lists, with (active,current) orientation; it is
 * NOT repeated single-operation output. Remove compacts the entire selected set
 * using descending survivors for ascending holes, independent of input order.
 * Count0 permits NULL input arrays and leaves state/output unchanged. Bounds,
 * handles, arrays and pair output must not alias owner storage or one another.
 * These calls allocate and free one bounded temporary block (O(existing+count))
 * for validation, sweep work and add's trial arrays. No second index persists.
 * Allocation/validation/output-capacity failure precedes any public mutation.
 * All handles remain borrowed; even removed handles must survive pair use. */
UCCollisionStatus uc_collision_sap_add_batch(UCCollisionSAP *sap,
    UCCollisionSAPHandle *const *handles,const UCCollisionSAPBounds *bounds,
    uint32_t count,UCCollisionSAPPairs *pairs);
UCCollisionStatus uc_collision_sap_remove_batch(UCCollisionSAP *sap,
    UCCollisionSAPHandle *const *handles,uint32_t count,UCCollisionSAPPairs *pairs);
#ifdef __cplusplus
}
#endif
#endif
