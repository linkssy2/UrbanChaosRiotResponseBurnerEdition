#ifndef UC_MAINPLAYER_MUZZLE_H
#define UC_MAINPLAYER_MUZZLE_H
#include "entity_events.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCMainplayerMuzzlePayload {
    uint32_t item;
    uint8_t alternate,padding[3];
} UCMainplayerMuzzlePayload;
/* Full C4390 ordinary Mainplayer, primary fixed item2/17. Actual definitions
 * are kinds2/17, so exactly first GunFlare ID+80C is notified; +810 is unused.
 * event is the resolved4F9E14 code, not a new hardcoded ID. sender is player+54.
 * Current equipment remains stable through synchronous callbacks. No mounted,
 * dual/alternate weapons, entity creation, transform placement or effect owner.
 * Returns entity-event status0/1/2. Missing entity is source-success, unhandled;
 * caller may inspect handled but must not retry a committed receiver failure. */
int uc_mainplayer_muzzle_notify(const UCEntityEventRegistry *,int32_t flare,
    uint32_t sender,uint32_t item,uint32_t event,uint8_t *handled);
#ifdef __cplusplus
}
#endif
#endif
