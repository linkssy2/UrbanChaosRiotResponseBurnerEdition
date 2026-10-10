#ifndef UC_EXPLOSION_CHILDREN_H
#define UC_EXPLOSION_CHILDREN_H
#include "explosion_effects.h"
#include "particle_asset.h"
#include "camera_push.h"
#ifdef __cplusplus
extern "C" {
#endif
/* EE4B0/EE0A0 descriptor projection after real218A60 mode1 base. One burst,
 * attached entity source219530(hidden0), plain heat Sprite, actual Ring.tga.
 * The returned asset must remain alive through particle unregistration. */
int uc_world_warp_asset(const UCExplosionVisualChild *,UCParticleAsset *);
/* Both1CCC00/1CCE00 send identical event19 nine-float payloads. Their only
 * different behavior here comes from supplied descriptor values. */
int uc_explosion_camera_input(const UCExplosionVisualChild *,const float world[16],UCCameraPushInput *);
/* Whole EE2A0: remove on OLD remaining<0, then subtract float world delta.
 *0 complete,1 invalid,2 reached service failure. Caller retains object through
 * removal and the subtraction. Camera objects instead remove on first update. */
int uc_world_warp_step(float *remaining,float delta,void *owner,int (*remove)(void *));
#ifdef __cplusplus
}
#endif
#endif
