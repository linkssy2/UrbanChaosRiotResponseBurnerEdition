#ifndef UC_EXPLOSION_EFFECTS_H
#define UC_EXPLOSION_EFFECTS_H
#include "shot_lightflash.h"
#ifdef __cplusplus
extern "C" {
#endif
enum {UC_EXPLOSION_WARP_RING,UC_EXPLOSION_CAMERA_PUSH,UC_EXPLOSION_CAMERA_KICK};
typedef struct UCExplosionVisualChild {
 uint32_t kind,parent_id,mode;
 float start_size,end_size,lifetime,strength;
 float force,fade_in,fade_out,minimum_range,maximum_range;
} UCExplosionVisualChild;
typedef struct UCExplosionEffectServices {
 void *owner;
 int (*world)(void *,float frame[16]);
 int (*particle)(void *,const char *asset,const float frame[16]);
 /* Complete real deferred20A230 creation; descriptor position/rotation are
  * untouched defaults for these mode1 children. No successful omitted effect. */
 int (*child)(void *,const UCExplosionVisualChild *);
 int (*flash)(void *,const UCShotLightFlashRequest *);
} UCExplosionEffectServices;
/* Whole E98E0/E9650 requests, successful allocation/property domain. Borrowed
 * entity fields are read at their actual source frontiers; callbacks may alter
 * them but not release the entity. Source particle_type2 chooses Gas quartet;
 * other values choose Std. Type1 suppresses children/flash only. Finite world
 * matrix and scale are required. No debris emitter exists in these bodies.
 * Child owners, dynamic light lifetime and real particle consumer are required
 * services, not proven by this recipe. 0 complete,1 invalid before effects,
 * 2 reached callback failure: committed effects require session teardown. */
int uc_explosion_effect_particles(const uint32_t *id,const int32_t *particle_type,
 const UCExplosionEffectServices *);
int uc_explosion_effect_flash(const uint32_t *id,const int32_t *particle_type,
 const float *scale,const UCExplosionEffectServices *);
const char *uc_explosion_particle_asset(int32_t particle_type,unsigned index);
#ifdef __cplusplus
}
#endif
#endif
