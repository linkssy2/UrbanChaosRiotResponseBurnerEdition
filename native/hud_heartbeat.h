#ifndef UC_NATIVE_HUD_HEARTBEAT_H
#define UC_NATIVE_HUD_HEARTBEAT_H
#include "game_random.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Original70F80/71960 retained history for the player's53-column monitor.
 * No actor registry, widget allocation graph or sound playback is invented. */
typedef struct UCHudHeartbeat {
    float previous_health,damage,damage_color[4],alert,alert_color[4];
    float sweep,pulse;
    int32_t alarms;
    float elapsed;
    float samples[53],colors[53][4];
} UCHudHeartbeat;
typedef struct UCHudHeartbeatActor {
    float health,maximum; /* Actual218120(+9C),89A80(+A0), not barBDAB0. */
    uint8_t present,external_color;
} UCHudHeartbeatActor;
int uc_hud_heartbeat_initialize(UCHudHeartbeat *);
/* Finite health0..4*maximum, maximum(0,65536], delta0..1. Borrow the SHARED
 * game RNG; never privately reseed it for the HUD. Sound is a required completing
 * service for original57FC0("HeartBeep"); it may consume that same RNG, but may
 * not mutate/retain this owner or input. Caller admits service completion before
 * calling. Invalid input leaves state/RNG unchanged and invokes no effects.
 * Actor absent still ages colors/elapsed; present performs the original history
 * update. Child-widget update and drawing belong to the parent composition. */
int uc_hud_heartbeat_step(UCHudHeartbeat *,const UCHudHeartbeatActor *,float delta,
    UCGameRandom *,void (*sound)(void *),void *);
#ifdef __cplusplus
}
#endif
#endif
