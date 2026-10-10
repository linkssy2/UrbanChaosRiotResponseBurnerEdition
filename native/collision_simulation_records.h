#ifndef UC_NATIVE_COLLISION_SIMULATION_RECORDS_H
#define UC_NATIVE_COLLISION_SIMULATION_RECORDS_H
#include "collision_asset.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Typed projection of a source64-byte record, NOT a packed guest layout.
 * body[] represents source+4/+8. Payload words have no inferred meaning. */
typedef struct UCCollisionSimulationRecord {
    uint32_t source_word0;
    const void *body[2];
    uint8_t opaque_before_gap[16]; /* Source+0C..+1B, copied on swap. */
    uint32_t untouched_word;       /* Source+1C stays at its destination. */
    uint8_t opaque_tail[32];        /* Source+20..+3F, copied on swap. */
} UCCollisionSimulationRecord;
typedef struct UCCollisionSimulationRecords {
    UCCollisionSimulationRecord *data;
    uint32_t count,capacity;
} UCCollisionSimulationRecords;
/* Original2F7400 cleanup only. For every match in either body position, reduce
 * count, copy the last live record EXCEPT its+1C word into the matching slot,
 * then recheck that slot. Unused tail storage remains untouched by clearing.
 * Identities are compared only, never dereferenced/retained/released. Borrowed
 * storage/identities must remain stable; no callbacks or concurrent mutation.
 * Native padding has no source meaning. Capacity<=65536; valid count<=capacity,
 * data covers capacity records, descriptor and data disjoint. Empty{NULL,0,0}
 * is admitted. Errors preserve descriptor and all records. No simulation/world
 * construction, record production or cleanup of payload-owned resources claimed. */
UCCollisionStatus uc_collision_simulation_remove(UCCollisionSimulationRecords *,const void *body);
#ifdef __cplusplus
}
#endif
#endif
