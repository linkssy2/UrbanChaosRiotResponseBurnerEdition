#ifndef UC_ENTITY_RECORD_OPS_H
#define UC_ENTITY_RECORD_OPS_H
#include "entity_records.h"
#include "entity_registration.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCEntityRecord {
    int32_t id;
    UCEntityEventTarget *target;
    uint32_t *flags;
} UCEntityRecord;
typedef struct UCEntityRecordMemory {
    void *owner;
    UCEntityRecord *(*allocate)(void *);
    void (*release)(void *,UCEntityRecord *);
} UCEntityRecordMemory;
typedef struct UCEntityRecordInsertResult {
    UCEntityRecord *record;
    uint8_t inserted;
} UCEntityRecordInsertResult;
/*207E50 over the SAME bucket owner and borrowed registration fields. Target
 * identity/flags/parent pointers are stable aliases of the actual entity.
 * Every live collection payload must be a distinct allocated record with a
 * valid id; a nonnull target has its actual flags alias, never a copied word.
 * Duplicate IDs/targets are valid. Exclusive use; no reentry. Views do not own
 * records. Input descriptors/output must not alias mutable owned storage.
 * Before effects the actual owner is checked for bounded acyclic traversal
 * with nonempty reachable bucket heads. Source can retain empty/cyclic links
 * after reuse; those states return1 unchanged, never silently skip/repair them.
 * Initial relative controls the parent-ID search BEFORE allocation. The
 * completing allocator may mutate current creation/entity fields and this
 * collection, but must retain all captured owners/record addresses until this
 * call returns and leave finite generic traversal for a chosen relative
 * insertion (the allocator cannot create a cycle then request that scan).
 * The allocated record must be new writable storage, disjoint
 * from the graph. Afterwards current relative and, if needed, priority are
 * reread; the old parent pointer is not resolved again.
 * Return0 completes the source operations;1 rejects arguments before effects;
 * 2 reports an unsupported NULL allocation after possible allocator effects,
 * leaves output unchanged and does not claim the source NULL-payload branch.
 * Output always accounts for the allocated payload, even if insertion fails:
 * full pool leaks no ownership silently; missing relative parent consumes a
 * pool node before failing. No rollback/free is invented. The admitted
 * allocator completes with a nonnull record. inserted0 is not gameplay success.
 * Orphan allocations remain caller-owned; destroy(collection) never frees
 * records. Actual successful gameplay must preadmit allocation/pool capacity. */
int uc_entity_record_insert(const UCEntityRecordMemory *,UCEntityRecords *,
    const UCEntityCreation *,const UCEntityRegistrationTarget *,int32_t id,
    UCEntityRecordInsertResult *);
/*207F70 and207FF0 render scan: first ID in bucket/forward order only. Update
 * allows NULL collection (source absent owner) and leaves a matching record
 * attached when target flags already contain8. Otherwise publish flags8,
 * detach via actual221D00 owner, then release the payload. Render skips flags.
 * Completing release may immediately poison/free payload and mutate external
 * owners; no record/collection reads follow it. It must not fail or reenter.
 * Return0 completed,1 invalid before effects; removed0 includes miss/flag8.
 * No slot/list/entity destruction; output storage outlives release and must
 * be disjoint. Existing records and borrowed target/flags remain valid up to
 * their release; source NULL record scans are outside this admitted domain. */
int uc_entity_record_remove_update(const UCEntityRecordMemory *,UCEntityRecords *,int32_t id,uint8_t *removed);
int uc_entity_record_remove_render(const UCEntityRecordMemory *,UCEntityRecords *,int32_t id,uint8_t *removed);
#ifdef __cplusplus
}
#endif
#endif
