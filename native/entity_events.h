#ifndef UC_ENTITY_EVENTS_H
#define UC_ENTITY_EVENTS_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* The original synchronous event route: 207D70,1F8FC0,1F90A0,20A540,110510.
 * Borrowed registry slots preserve the entire generation-bearing ID. A null
 * listener terminates the three-slot scan; the first matching subscription wins.
 * Payload size is forwarded by the entity but not consumed by this subscriber
 * family. Callback payload validation belongs to the concrete event receiver. */
typedef struct UCEntityEventSubscription {
    uint32_t event;
    int (*receive)(void *owner,const void *payload,uint32_t *sender);
} UCEntityEventSubscription;
typedef struct UCEntityEventListener {
    const UCEntityEventSubscription *subscriptions;
    uint32_t count;
    void *owner;
} UCEntityEventListener;
typedef struct UCEntityEventTarget {
    UCEntityEventListener *listeners[3];
} UCEntityEventTarget;
typedef struct UCEntityEventSlot {
    int32_t id;
    UCEntityEventTarget *target;
} UCEntityEventSlot;
typedef struct UCEntityEventRegistry {
    UCEntityEventSlot slots[1024];
} UCEntityEventRegistry;
/* No registration/allocation, queues, script receiver or entity lifetime claim.
 * Valid owners/entries remain alive through this nonreentrant call. Up to256
 * subscriptions per listener; callbacks complete synchronously, return0 on
 * success and may mutate payload/sender. They must retain the dispatch graph.
 * Missing/stale/sentinel IDs are successful unhandled events. Return0 success,
 * 1 rejected inputs (no effects),2 receiver failure (effects may be committed).
 * 'handled' is published only on successful completion; do not retry failure2. */
UCEntityEventTarget *uc_entity_event_lookup(const UCEntityEventRegistry *,int32_t id);
int uc_entity_event_send(const UCEntityEventRegistry *,int32_t id,uint32_t event,
    const void *payload,uint32_t payload_size,uint32_t *sender,uint8_t *handled);
#ifdef __cplusplus
}
#endif
#endif
