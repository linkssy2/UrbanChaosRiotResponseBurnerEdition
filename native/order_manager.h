#ifndef UC_ORDER_MANAGER_H
#define UC_ORDER_MANAGER_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCOrderManagerAction {
 int32_t client,command;const char *name;uint32_t slot,hud;uint8_t contextual;
} UCOrderManagerAction;
typedef struct UCOrderManager {
 float remaining;int32_t forced_client;
 uint8_t invalid_last,available,forcing,held,opened;
} UCOrderManager;
typedef struct UCOrderManagerServices {
 void *owner;
 int (*look)(void *);
 int (*property)(void *,int32_t,const char *,uint8_t fallback,uint8_t *);
 int (*veto)(void *,const char *,uint8_t *);
 int (*control)(void *,uint8_t *);
 int (*gameplay)(void *,uint8_t *);
 int (*input)(void *,uint32_t query,uint32_t mode,double *);
 int (*delta)(void *,double *);
 int (*hud)(void *,uint32_t id,uint8_t mode); /*0 hide,1 white show,2 selected flash*/
 int (*send)(void *,int32_t client,int32_t command);
 int (*sound)(void *);
 int (*forced)(void *);
 int (*prompt)(void *,uint32_t source_candidate_ordinal);
} UCOrderManagerServices;
/*158510 after exact pending-client discovery, with same ordered attached
 * actions and copied eligible-action frontier. The source properties/queries
 * are completing callbacks; no missing input, veto or look owner is a noop.
 * Actions and scratch must be disjoint and stable for this call. send may
 * mutate actual manager/client owners, but not this borrowed source snapshot.
 *0 completed,1 invalid beforeeffects,2 committed callbackfailure.*/
void uc_order_manager_init(UCOrderManager *);
int uc_order_manager_select(UCOrderManager *,const UCOrderManagerServices *,
 const UCOrderManagerAction *,uint32_t count,uint32_t clients,int32_t parent,
 uint32_t pad,uint32_t *eligible,uint32_t capacity);
#ifdef __cplusplus
}
#endif
#endif
