#ifndef UC_THROWABLE_TEMPLATE_H
#define UC_THROWABLE_TEMPLATE_H
#include "entity_authored.h"
#include "entity_listener.h"
#include "entity_names.h"
#ifdef __cplusplus
extern "C" {
#endif
/* F8E50 Molotov template: actual base-only entity, listener, one subscription
 * and TEMPLATE_MOLOTOV name. No actor/coroutine or duplicate registry. */
typedef struct UCThrowableTemplate {
    UCEntitySubscriptions *subscriptions;
    const UCEntityFreshView *view;
    UCEntityNames *names;
} UCThrowableTemplate;
/* A fresh zero-initialized state and caller-owned real base view. event_code
 * borrows actual4FA014 (normally210), read after listener registration. receive
 * is the REQUIRED completing F8210 projectile service, never a quiet success.
 * Construction itself never dispatches it. Completing constructor effects may
 * mutate current metadata/IDs; borrowed owners remain valid and nonreentrant.
 * active=NULL selects the actual dynamic empty159C0 descriptor mode0 used by
 *1D3840; caller supplies the factory's current parent/definition/record context.
 *0 complete,1 pre-effect reject,2 committed allocation/service failure. Any
 * subscriptions published on failure remain owned; no rollback/retry. */
int uc_throwable_template_construct(UCThrowableTemplate *,const UCEntityFreshContext *,
    const UCEntityFreshView *,const UCEntityAuthored *const *,UCEntityListenerList *,
    UCEntityNames *,const uint32_t *event_code,void *receive_owner,
    int (*receive)(void *,const void *,uint32_t *));
/* Actual template vtable+30/+34=11030: leaves ALL entity state untouched. */
int uc_throwable_template_idle(const UCThrowableTemplate *);
/* Native application shutdown only, after real slot/live/record detachment.
 * Clears target listener references and owned subscription memory. Name removal
 * reuses1F7E20. Does not claim F8F20/20A480 generic destruction or removal from
 * the shared listener-ID list; that list retains its source stale ID until the
 * real world maintenance owner or application close. */
int uc_throwable_template_close_detached(UCThrowableTemplate *);
#ifdef __cplusplus
}
#endif
#endif
