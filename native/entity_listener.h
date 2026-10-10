#ifndef UC_ENTITY_LISTENER_H
#define UC_ENTITY_LISTENER_H
#include "entity_events.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCEntityListenerList UCEntityListenerList;
typedef struct UCEntitySubscriptions UCEntitySubscriptions;
/* Native storage for the actual1F9270 circular ID list; duplicate IDs and
 * insertion order are retained. Creation/close are explicit host ownership,
 * not original world construction/destruction. No scheduler traversal claim. */
int uc_entity_listener_list_create(UCEntityListenerList **);
void uc_entity_listener_list_close(UCEntityListenerList *);
uint32_t uc_entity_listener_list_count(const UCEntityListenerList *);
int uc_entity_listener_list_copy(const UCEntityListenerList *,int32_t *,uint32_t capacity);
/* Actual1F9150: detach/free the first matching ID; duplicates and later IDs
 * retain their order. Missing ID is successful. No entity/listener mutation. */
int uc_entity_listener_list_remove_first(UCEntityListenerList *,int32_t id);
/* Whole1F91A0 over this actual live registration list. Reads current node.next
 * AFTER each synchronous callback: appends and removal of other nodes are
 * observed immediately. Missing/stale IDs and unhandled events are successful.
 * Current-node removal, list close and recursive broadcast are outside the
 * source-safe ownership domain; remove_first rejects current-node removal.
 * Registry/list/receiver owners must outlive dispatch. Visits bounded65536.
 * Return0 complete;1 invalid beforeeffects;2 committed receiver/traversalfailure.
 * Payload and SAME sender pointer are forwarded to every actual target. */
int uc_entity_listener_broadcast(UCEntityListenerList*,const UCEntityEventRegistry*,
 uint32_t event,const void*payload,uint32_t size,uint32_t*sender);
/* Full1F9120/20A4F0 registration through the same generation-ID registry.
 * target_id is the actual selected entity+54, explicitly supplied metadata.
 * If slot0 is empty, append its ID to list BEFORE filling first empty slot.
 * Full three-slot targets silently do nothing; no deduplication. Registry,
 * target, listener and list must stay valid/nonreentrant. Missing target is
 * success, inserted0. Host list bound65536. Invalid/OOM returns1 with no effects;
 * original allocator completion is the admitted source domain. inserted is
 * written only on success. List can be NULL when its branch is not reached. */
int uc_entity_listener_register(const UCEntityEventRegistry *,int32_t request,
    int32_t target_id,UCEntityEventListener *,UCEntityListenerList *,int *inserted);
/* Concrete zero-third-word subscription family from GunFlare187600. Native
 * pointer width replaces source12-byte records; the unused original third word
 * is fixed0. Count/capacity and relocation policy are retained: floor1.5 growth,
 * at least count+1. Captures an aliased entry before allocation; frees prior
 * storage before publishing new pointers. Readonly view has stable identity,
 * but its subscriptions pointer may change after append. At most256 entries.
 * Closing requires prior detachment from every target; it is host cleanup,
 * not original entity unregister/destructor. No callback invocation here. */
int uc_entity_subscriptions_create(void *owner,UCEntitySubscriptions **);
UCEntityEventListener *uc_entity_subscriptions_listener(UCEntitySubscriptions *);
uint32_t uc_entity_subscriptions_capacity(const UCEntitySubscriptions *);
int uc_entity_subscriptions_append(UCEntitySubscriptions *,const UCEntityEventSubscription *);
/* Actual173F70 array suffix: release entries, then publish NULL/count0/capacity0.
 * Keeps the embedded listener identity alive; no target/list unregister occurs. */
void uc_entity_subscriptions_clear(UCEntitySubscriptions *);
void uc_entity_subscriptions_close(UCEntitySubscriptions *);
#ifdef __cplusplus
}
#endif
#endif
