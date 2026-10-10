#ifndef UC_NPC_THROWN_TRIGGER_H
#define UC_NPC_THROWN_TRIGGER_H
#include "npc_thrown_weapon.h"
#include "particle_trigger.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCNpcThrownTriggerServices {
 void *owner;
 /*Actual generation registry resolution followed by the real thrown owner.
  * NULL means entity absent; never a replacement live/default projection.*/
 const UCNpcThrownWeapon*(*resolve)(void*,int32_t);
} UCNpcThrownTriggerServices;
/*Full DA970 custom virtual+8; DA750's exact 'show particles' getter is219.
 * Missing/disabled pauses underlying trigger, emits0, preserves lifetime.*/
int uc_npc_thrown_trigger_emit(UCParticleTrigger*,int32_t,float,const UCNpcThrownTriggerServices*,uint32_t*);
#ifdef __cplusplus
}
#endif
#endif
