#ifndef UC_ENTITY_RETIREMENT_H
#define UC_ENTITY_RETIREMENT_H
#include "entity_slots.h"
#include "entity_list.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCEntityRetirement {
    UCEntityEventRegistry *registry;
    UCEntityList *live,*pending;
    void *owner;
    int (*remove_update_record)(void *owner,int32_t id);
    int (*remove_render_record)(void *owner,int32_t id);
} UCEntityRetirement;
/*207FF0: update record, render record, live-list detach, exact ID invalidation,
 * then pending-list prepend. The two completing services implement207F70 and
 * the render-record scan/detach/free, respectively; return0 when completed.
 * They may mutate the same registry/list contents before lookup, but must keep
 * this descriptor, owners and all captured objects alive. Exclusive access,
 * no reentry; live/pending are distinct, valid constructor-reachable lists.
 * Return0 completed,1 invalid arguments before effects,2 service failure after
 * possible effects (do not retry). queued is published only on return0.
 * queued=0 includes a stale/sentinel/null-target ID or a FULL pending pool:
 * source ignores failed enqueue AFTER invalidating the slot. Preserve that
 * outcome honestly; a caller requiring retirement must ensure real capacity
 * before effects. This does not destruct/free the entity, mark its flags, clean
 * listeners/model/audio, or run208F80. Retained target pointers are borrowed.
 * queued storage must not alias this ownership graph. */
int uc_entity_retire(const UCEntityRetirement *,int32_t id,uint8_t *queued);
#ifdef __cplusplus
}
#endif
#endif
