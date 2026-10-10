#ifndef UC_ENVIRONMENT_FOG_CONTROLLER_H
#define UC_ENVIRONMENT_FOG_CONTROLLER_H
#include "environment_fog.h"
#include "scene_portal_start.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCEnvironmentFogState {
    uint8_t enabled;                 /* geometry+41 */
    float start,end;                 /* geometry+44/+48 */
    uint32_t color;                  /* geometry+4C, packed ARGB */
    float maximum;                   /* geometry+50 */
} UCEnvironmentFogState;
typedef struct UCEnvironmentFogController {
    UCFogProperties properties;
    uint8_t first,transition;
} UCEnvironmentFogController;
typedef struct UCEnvironmentFogScene {
    UCEnvironmentFogState *fog;
    const UCScenePortalStart *rooms;  /* SAME portal owner+178 camera-room list */
} UCEnvironmentFogScene;
typedef struct UCEnvironmentFogHooks {
    uint32_t (*world_mode)(void *);
    UCEnvironmentFogScene *(*scene)(void *);
    float (*delta)(void *);
} UCEnvironmentFogHooks;
/* Represented 195F10 property/flag initialization, not entity allocation,
 * registration, editor properties or names ownership. Input is the actual
 * 32-byte serialized payload. Enabled normalizes the LOW byte; distance0 only.
 * Prior target copies and unwritten anchor are omitted because this admitted
 * path never reads them. Source position is borrowed separately from entity+40.
 * All values finite, distances magnitude<=1e8, RGB/max within[0,1], RN and normal
 * denormals. Output/input must be disjoint. Returns0 complete,1 invalid. */
int uc_environment_fog_controller_init(UCEnvironmentFogController *,const UCFogProperties *);
/* Full195A50,1954D0/AAB80,195530 and distance0 branch of full1955E0.
 * World mode uses LOW byte. scene resolves the CURRENT primary TREE key0,
 * exposing its SAME fog fields and portal owner, not entity.active. Calls are
 * repeated at source boundaries; after a nonnull first scene, the second must
 * be nonnull. A present portal is required for membership; an empty start list
 * legitimately returns false and may still install initially if fog is off.
 * Sources/setters are concrete fields. Every returned scene/fog/start list and
 * current position must remain accessible/valid until the operation finishes.
 * Up to4096 finite room bounds, count<=capacity. All hooks complete, never fail
 * or reenter this owner, and preserve the admitted distance0/RGB/value domain;
 * scene/delta may change current scene, properties or external state. Delta is
 * current finite[0,60] each time (six independent reads in original order).
 * Required pure preadmission by caller covers future callback publications.
 * Static initial validation precedes hooks; a later contract violation is not
 * retryable. No entity scheduler, graphics registry construction, portal
 * collection timing, positive-distance fade or fog GPU draw is implied. */
int uc_environment_fog_controller_update(UCEnvironmentFogController *,const float position[3],
    const UCEnvironmentFogHooks *,void *owner);
/* Captured current fog object and concrete setters; same admission/contracts. */
int uc_environment_fog_controller_install(UCEnvironmentFogController *,const UCEnvironmentFogHooks *,void *owner);
#ifdef __cplusplus
}
#endif
#endif
