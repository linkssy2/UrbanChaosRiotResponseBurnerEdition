#ifndef UC_LIGHT_FITTING_H
#define UC_LIGHT_FITTING_H
#include "entity_stream.h"
#include "game_random.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCLightFittingParameters {
    int32_t linked_entity;
    float health;
    uint32_t initial_state,flicker_mode;
    float period,value;
} UCLightFittingParameters;
/* Immutable six-property projection of177230; no coroutine/base/RNG effects. */
int uc_light_fitting_parameters(const UCLevelEntity *,const uint8_t **,
    UCLightFittingParameters *);
typedef struct UCLightFittingValues {uint32_t group;float factor,intensity;} UCLightFittingValues;
/*1777E0 selected group and factor, then176430 with exactly one registered
 * contribution. Groups0 means no model; otherwise1..3, matching actual assets.
 * Immutable finite factors and base intensity, magnitude<=10000, RN/FTZ-off.
 * Borrowed inputs/output are disjoint. No owner or
 * packet mutation during this pure projection. Initial controller factors are
 * all1; on/off loop publishes factor28=1/0. A zero factor is not clamped up.
 * Does not implement controller allocation, group visibility callbacks, linked
 * light registration, flicker, health/damage, sound or actor construction.
 * Output is unchanged on invalid admission.0 completed,1 invalid. */
int uc_light_fitting_values(uint32_t groups,float factor20,float factor24,
    float factor28,float base_intensity,UCLightFittingValues *);
/*177230 constructor's unconditional2D3780(2,4), on the SAME game RNG.
 * State/output are disjoint; RN rounding. Invalid calls preserve output. */
int uc_light_fitting_delay(UCGameRandom *,float *);
/*59280 uses the controller's copied177F40 position and inclusive4EE70 AABB
 * test, not the light radius or model bound. Finite inputs;0 success. */
int uc_light_fitting_area_contains(const float bounds[6],const float position[3],unsigned *inside);
#ifdef __cplusplus
}
#endif
#endif
