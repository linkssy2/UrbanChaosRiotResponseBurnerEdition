#ifndef UC_ENTITY_RECORD_PRIORITY_H
#define UC_ENTITY_RECORD_PRIORITY_H
#include "entity_record_ops.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCEntityPriorityLists {
    UCEntityRecords *render,*update; /* current world owner+2C/+30 */
} UCEntityPriorityLists;
/* Full2080A0: first matching ID only, detach its existing payload and reinsert
 * at signed-clamped priority. No new payload or flags publication. Uses the
 * actual bucket owner, retaining source stale node/bucket fields. Existing
 * payloads must be valid, distinct records; duplicate IDs are allowed.
 * The completing release is only reached if reinsertion returns sourcefalse;
 * it may immediately free the detached record. No subsequent record read.
 * Initial traversal must be bounded/nonempty; no graph repair is invented.
 * Return0 completed,1 rejected without effects. The release branch cannot be
 * reached after successful removal in the current admitted pool owner.
 */
int uc_entity_record_priority(const UCEntityRecordMemory *,UCEntityRecords *,
                              int32_t id,int32_t priority);
/* Full208110 over the live world+4 owner: render first, then reread owner and
 * update collection. Owner and lists remain alive; memory/output descriptors
 * must not alias them. Invalid initial pair returns1 without effects; invalid
 * second graph/owner returns2 with first effects committed. Never retry2 as a
 * fresh request. This does not construct/register entities or schedule frames. */
int uc_entity_record_priority_both(const UCEntityRecordMemory *,
    const UCEntityPriorityLists *const *active,int32_t id,int32_t priority);
#ifdef __cplusplus
}
#endif
#endif
