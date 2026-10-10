#ifndef UC_NATIVE_HUD_HEALTH_H
#define UC_NATIVE_HUD_HEALTH_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* OriginalBDAB0, after the caller resolves the current session mode and
 * progression selector5 count. No save or actor+A0 substitution. */
float uc_hud_health_maximum(uint32_t session_mode,uint32_t health_tier);
typedef struct UCHudHealth {
    float color[4];
    float delay,delayed_fraction,phase,fraction,extent;
} UCHudHealth;
/* Scalar76A50 state only; no widget/heartbeat construction. Original root scale2.
 * Finite scale in(0,16], resolved mode/tier; output untouched on failure. */
int uc_hud_health_initialize(UCHudHealth *,uint32_t session_mode,uint32_t health_tier,float scale);
/* Original76DC0/72DB0 presentation history AFTER its child's update callback.
 * The actual inventory supplies health; current session/progression owner
 * supplies mode/tier each tick. Finite health[0,4*maximum], delta[0,1], and a
 * valid initialized state. No implicit callback or heartbeat simulation.
 * Failure leaves state unchanged. The retained constructor extent never changes
 * when the current progression normalization changes. */
int uc_hud_health_step(UCHudHealth *,float health,uint32_t session_mode,uint32_t health_tier,float delta);
#ifdef __cplusplus
}
#endif
#endif
