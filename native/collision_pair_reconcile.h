#ifndef UC_NATIVE_COLLISION_PAIR_RECONCILE_H
#define UC_NATIVE_COLLISION_PAIR_RECONCILE_H
#include "collision_sap.h"
#define UC_COLLISION_PAIR_RECONCILE_MAX 32768u
/* Pure3191A0 cancellation of unordered handle-pointer pairs. Pointer VALUES are
 * opaque borrowed keys, never dereferenced; both live pointers must be nonnull.
 * Each count is <=32768: source large-path index occupies signed high16 bits,
 * with multiplicity in low16. Counts must fit capacities; arrays/descriptors
 * are readable/writable, disjoint and stable for the call. No concurrent access.
 * The smaller initial count selects the algorithm: below32, both arrays use
 * first-match/tail-swap cancellation. Otherwise first duplicates coalesce,
 * second cancellations tail-swap, and first survivors compact stably. Original
 * orientations and ALL stale-tail writes are preserved; capacity/data unchanged.
 * Small calls allocate nothing. Large calls allocate/free one temporary sorted
 * key array, at most32768 entries (768KiB on64bit); no persistent map or guest
 * runtime. Validation/allocation failure leaves descriptors and storage intact.
 * Callers composing callbacks must preadmit completion; this pure leaf cannot
 * roll back effects already performed by an outer controller. */
#ifdef __cplusplus
extern "C" {
#endif
UCCollisionStatus uc_collision_pair_reconcile(UCCollisionSAPPairs *first,
    UCCollisionSAPPairs *second);
#ifdef __cplusplus
}
#endif
#endif
