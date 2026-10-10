#ifndef UC_ENTITY_UPDATE_H
#define UC_ENTITY_UPDATE_H
#include "entity_record_ops.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCEntityUpdateFields {
    uint32_t *secondary_flags;
    void **helper;
} UCEntityUpdateFields;
typedef struct UCEntityUpdateServices {
    void *owner;
    UCEntityUpdateFields (*fields)(void *,UCEntityEventTarget *);
    void (*remove)(void *,UCEntityEventTarget *);
    void (*step)(void *,UCEntityEventTarget *);
    void (*helper_tick)(void *,void *,float);
    void (*late)(void *,UCEntityEventTarget *);
    void (*destroy)(void *,UCEntityEventTarget *);
} UCEntityUpdateServices;
/*208F80's active record passes2091A6..20929C, including current delta stores.
 * Actual bucket owner/pending list and borrowed record target/flags, not copies.
 * Fields is a pure infallible mapping to stable actual entity fields. All live
 * records have distinct valid payloads and nonnull targets/flags/field aliases.
 * Completing callbacks may remove their CURRENT record and change live fields;
 * captured future nodes/buckets/targets must remain alive with unchanged meaning.
 * Priority insertion may prepend the SAME currently nonempty bucket without
 * recycling any existing node. Relative insertion may follow the CURRENT record
 * or a fresh record inserted during that same step. Captured next nodes retain
 * their next links/targets; new records receive late in the second pass and their
 * first step on the following frame. Insertion after a captured future record,
 * new bucket, future-record removal, reentry or owner replacement is excluded.
 * Removal/destruction must detach all references to the dying entity first.
 * Destroy may immediately free it; nothing from that target is read afterward.
 * Current record may be freed by step/remove, so only captured entity aliases
 * are used after callbacks. helper_tick is required for nonnull helper78 only;
 * its NULL branch is the actual216040 early return. All hooks except fields
 * perform real work; no missing game service is supplied by this controller.
 * Callbacks and descriptors are stable, output delta aliases are disjoint from
 * inputs/owned graph. Pending capacity/lifetimes satisfy entity_pending's rules.
 * Return0 completed;1 arguments/initial unsafe graph rejected before effects;
 * 2 unsafe graph after the first pass, with prior effects committed. Never retry
 * 2 as a fresh tick. Callback graph/memory violations are outside this contract.
 * Earlier deferred queues, frame flags/time accumulation, world maintenance,
 * final pending drain and world-tail updates remain the caller's real owners. */
int uc_entity_update_records(UCEntityRecords *,UCEntityList *,float delta,float other_delta,
    float *current_delta,float *current_other_delta,const UCEntityUpdateServices *);
#ifdef __cplusplus
}
#endif
#endif
