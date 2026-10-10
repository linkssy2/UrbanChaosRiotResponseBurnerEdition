#ifndef UC_ENTITY_SLOTS_H
#define UC_ENTITY_SLOTS_H
#include "entity_events.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Full207C70 allocation into the actual1024 generation-bearing slots. Borrowed
 * target may be null, as in the source. next_id is the actual slot-table+2000,
 * in its initialized nonnegative domain <7FFFFBFF. Scan at most1024 candidates;
 * occupied slots are never overwritten. Full table succeeds with id=-1 and no
 * state change. Returns0 success,1 invalid arguments (all outputs unchanged).
 * Distinct storage, stable graph, no concurrency/reentry. No heap, entity
 * construction/publication, registry reset, destruction or slot removal claim. */
int uc_entity_slot_acquire(UCEntityEventRegistry *,uint32_t *next_id,
    UCEntityEventTarget *,int32_t *id);
/* Slot-only projections of209E20 initialization and208D90 shutdown.
 * Initialize marks IDs empty and resets the counter; existing target pointers
 * are deliberately retained, never inspected while ID is empty. Release marks
 * only an exact generation match empty, preserving target and counter.
 * A stale/sentinel ID is a successful no-op. Return1 on null arguments before
 * mutation. Storage must be initialized, disjoint and exclusively owned.
 * These do not detach lists/listeners, destroy entities or decide retirement:
 * shutdown calls release after list detachment and before entity destruction.
 * Do not use initialization to discard live owners or infer ordinary per-frame
 * retirement from the shutdown projection. */
int uc_entity_slots_initialize(UCEntityEventRegistry *,uint32_t *next_id);
int uc_entity_slot_release(UCEntityEventRegistry *,int32_t id);
#ifdef __cplusplus
}
#endif
#endif
