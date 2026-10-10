#ifndef UC_BULLET_TRACERS_H
#define UC_BULLET_TRACERS_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCBulletTracer {float from[3],to[3],padding[2],rgba[4],width,remaining,total,reserved;} UCBulletTracer;
typedef struct UCBulletTracers {uint32_t count;UCBulletTracer items[16];} UCBulletTracers;
typedef struct UCBulletTracerVertex {float position[3],uv[2],rgba[4];} UCBulletTracerVertex;
typedef struct UCBulletTracersServices {
 void *owner;
 /*0 missing audio owner,1 live214F40(0) position,-1 unavailable.*/
 int (*listener)(void *,float[3]);
 /*Actual Banks\\Impacts.amp Entry, position/velocity, final argument0.*/
 int (*flyby)(void *,const float[3],const float[3]);
} UCBulletTracersServices;
void uc_bullet_tracers_init(UCBulletTracers *);
/*6A4C0.0 success,1 invalid,2 committed service failure. A full16-entry
 * collection still performs the actual near-listener audio branch.*/
int uc_bullet_tracers_append(UCBulletTracers *,const float[3],const float[3],const float[4],float,const UCBulletTracersServices *);
/*6A3C0/6A3E0: positive delta gates ONE fixed source step, not dt-scaled fade.*/
int uc_bullet_tracers_step(UCBulletTracers *,float delta);
/*6A000: three vertices each, in source order, camera forward row2.*/
int uc_bullet_tracers_vertices(const UCBulletTracers *,const float camera_forward[3],UCBulletTracerVertex *,uint32_t capacity);
#ifdef __cplusplus
}
#endif
#endif
