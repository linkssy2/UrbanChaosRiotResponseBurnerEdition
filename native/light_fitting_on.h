#ifndef UC_LIGHT_FITTING_ON_H
#define UC_LIGHT_FITTING_ON_H
#include "game_random.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Borrow the actual LightFitting fields, shared clock/RNG and CURRENT controller.
 * identity is the enclosing live entity:1786C0 adds its low12 address bits to
 * world time. Native allocation addresses are a PC identity adaptation, not a
 * claim that an independently allocated Xbox entity has the same phase. */
typedef struct UCLightFittingOn {
    const void *identity;
    const uint32_t *mode;
    const float *period,*clock,*delta,*position,*health;
    float *spark_delay;
    const uint8_t *damage;
    void *const *controller,*const *bank;
    UCGameRandom *random;
} UCLightFittingOn;
typedef struct UCLightFittingOnServices {
    void *owner;
    int (*publish)(void *,void *controller,float);
    float (*factor)(void *,void *controller);
    uint32_t (*state)(void *,void *controller);
    int (*broken)(void *,void *controller,uint8_t);
    int (*sparks)(void *);
    const float *(*player_position)(void *); /* actual player0 query; NULL valid */
    int (*sound)(void *,void *bank); /* actual216BD0("Flicker",0) */
    int (*yield)(void *); /* real primary coroutine suspension/completion */
} UCLightFittingOnServices;
/*1786C0 on-state entry and loop, modes2/3/4 only. All services are completing
 * effects, not optional notifications. The caller owns actual controller,
 * sparks/audio and coroutine implementations. publish/broken/sparks/sound/yield
 * return0 completed; other values stop with committed failure2, never rollback.
 * Slots and identity stay alive/stable across callbacks, including suspension;
 * contents may change within this domain. factor returns a finite value; player
 * position is finite with magnitude<=1e6 and survives its query until the
 * immediate distance read. No callbacks change FP mode or reenter.
 * Finite clock0..2^20, period .001..100, delta0..1, |position|/|health|<=1e6,
 * delay[-4,4], valid shared RNG, RN/FTZ-off. All pointed storage is disjoint
 * except intended SAME owners. Initial failure1 has no effects; later domain
 * failure2 retains prior effects. Returns0 once current controller state!=0.
 * This is not a controller factory, broken/dying mode, or full effect backend. */
int uc_light_fitting_on(const UCLightFittingOn *,const UCLightFittingOnServices *);
#ifdef __cplusplus
}
#endif
#endif
