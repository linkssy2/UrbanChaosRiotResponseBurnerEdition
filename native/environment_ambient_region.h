#ifndef UC_ENVIRONMENT_AMBIENT_REGION_H
#define UC_ENVIRONMENT_AMBIENT_REGION_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Actual1EFA30 constructor fields, separate from the one world ambient owner.
 * Resolved property RGB/fade supplied, or NULL defaultsRGB255/fade0. */
typedef struct UCEnvironmentAmbientRegion {float rgb[3],fade;float start_position[3],start_rgb[3];uint8_t initial,active;} UCEnvironmentAmbientRegion;
int uc_environment_ambient_region_init(UCEnvironmentAmbientRegion *,const float rgb[3],const float *fade);
/* Source type1 volume21F210->21E5C0, inclusive X,Z,Y comparisons. Finite input,
 * ordered lower<=upper. No entity lookup or volume transformation invented. */
int uc_environment_ambient_region_contains(const float lower[3],const float upper[3],const float position[3],uint32_t *inside);
typedef struct UCEnvironmentAmbientRegionHooks {
 uint32_t (*membership)(void *); /* only LOW BYTE */
 void (*position)(void *,float xyz[3]); /* actual first player, completing */
 void (*current_rgb)(void *,float rgb[3]); /* live manager via57D70 */
 void (*initial_rgb)(void *,const float rgb[3]); /* world owner57DC0 */
 void (*target_rgb)(void *,const float rgb[3]); /* world owner57E00 */
} UCEnvironmentAmbientRegionHooks;
/* Full1EFC20. Initial flag always clears on first call, even outside. Initial
 * inside applies immediately and stores FLT_MAX position. Later outside clears
 * active; later inside fade<=0 sets target without reading position/current.
 * fade>0 requires actual player; reentry captures position/current before
 * distance interpolation. x87 squared sum/sqrt represented as hostdouble then
 * originalfloat rounding; no physical80-bit exceptional parity claim.
 * Services complete and cannot mutate this region; getters may change their
 * own current world/manager. FiniteRGB in[-4096,4096], finitefade/positions.
 * Dynamic invalid getter rejects after earlier callbacks/field edits; no
 * rollback/retry guarantee. Static guards pre-effect. No allocation/scheduler. */
int uc_environment_ambient_region_update(UCEnvironmentAmbientRegion *,const UCEnvironmentAmbientRegionHooks *,void *);
#ifdef __cplusplus
}
#endif
#endif
