#ifndef UC_EXPLOSION_DEBRIS_H
#define UC_EXPLOSION_DEBRIS_H
#include "game_random.h"
#ifdef __cplusplus
extern "C" {
#endif
/*484F0's actual32-byte record. The trailing three bytes remain source-unwritten.*/
typedef struct UCExplosionDebrisParticle {float position[3],velocity[3],size;uint8_t stopped,padding[3];} UCExplosionDebrisParticle;
typedef struct UCExplosionDebris {float duration,remaining;UCExplosionDebrisParticle *particles;uint32_t count;} UCExplosionDebris;
typedef int (*UCExplosionDebrisQuery)(void*,const float from[3],const float to[3],
 uint32_t type,uint32_t group,uint32_t mask,float impulse,float point[3],float normal[3],uint8_t *hit);
/*Pure record portion484F0; caller owns actual allocation/texture/render lease.
 *Shared2D3780 draws remain ordered Z,Y,X.0success,1 invalid,2 committed failure.*/
int uc_explosion_debris_initialize(UCExplosionDebris*,UCExplosionDebrisParticle*,uint32_t,
 const float position[3],float size,float duration,float power,UCGameRandom*);
/* Trusted internal 481B0 bounce/publication kernel. velocity/to are the
 * pre-query values; a NULL point means miss. Does not validate or query. */
void uc_explosion_debris_resolve(UCExplosionDebrisParticle*,float,
 const float velocity[3],const float to[3],const float *point,const float *normal);
/*Whole481B0, including expiry-before-query and original full-delta bounce.*/
int uc_explosion_debris_step(UCExplosionDebris*,float,UCExplosionDebrisQuery,void*);
/*48700 outputs (XYZ,size) and (1,1,1,remaining/duration), no camera transform.*/
int uc_explosion_debris_vertices(const UCExplosionDebris*,uint32_t first,uint32_t count,
 float (*position_size)[4],float (*color)[4]);
#ifdef __cplusplus
}
#endif
#endif
