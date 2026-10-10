#ifndef UC_ENTITY_PENDING_H
#define UC_ENTITY_PENDING_H
#include "entity_list.h"
#ifdef __cplusplus
extern "C" {
#endif
/*208F80 final pending-list block2092A1..2092D6. Caller has reached that source
 * frontier after update/post-update services and with world flag0x80 admitted.
 * Capture each next node before detaching current's first pointer match, then
 * call the actual deleting destructor for nonnull targets. NULL entries are
 * detached without a callback. Destruction may immediately free the target.
 * Pending owner/storage must survive; callbacks must complete and may prepend
 * NEW distinct targets, but cannot remove/recycle remaining captured nodes or
 * free their targets. Newly prepended entries await the next drain. Each live
 * nonnull target has only one entry. Exclusive nonreentrant use, borrowed values.
 * No entity update, generic destructor implementation, slot invalidation,
 * flag marking or earlier world services are implied by this narrow owner.
 * Return0 completed,1 null owner/callback before effects. */
int uc_entity_pending_drain(UCEntityList *,void (*destroy)(void *,void *target),void *owner);
#ifdef __cplusplus
}
#endif
#endif
