#ifndef UC_ORDER_TARGET_H
#define UC_ORDER_TARGET_H
#include "point_light_geometry.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCOrderTarget {int32_t id;uint32_t flags;UCModelBounds bounds;float world[16];} UCOrderTarget;
/*45490 ordered SELECTABLEITEMS frontier, actual entity+5C flags and virtual3C
 * local bounds/virtual2C world. No inferred capsule or visibility substitute.
 * Returns -1 invalid,0 no hit,1 hit; miss publishes ordinal UINT32_MAX and
 * FLT_MAX distance. Uses already qualified2D4360 inverse/RCPSS arithmetic.
 * Source minimum radius is a bounds expansion only when |extent|<50.*/
int uc_order_target_select(const float from[3],const float direction[3],
 const UCOrderTarget *,uint32_t count,uint32_t *ordinal,float *distance);
/*156E60 ray construction from the actual player's Look point matrix.*/
int uc_order_target_ray(const float look[16],float from[3],float to[3],float direction[3]);
/*156E60 after the world query/45490. hit_point is NULL for a scene miss;
 * same_object means exact collision-object/candidate identity, not proximity.
 * Always publishes hit_point or ray end. Returns -1 invalid,0 not admitted,
 * 1 admitted. Caller must mark the candidate 0x1000000 BEFORE object-type query;
 * publish ID/type only when that property exists. This never moves the look
 * point onto the candidate. Candidate distance is45490 output, not a new ray.*/
int uc_order_target_resolve(const float from[3],const float to[3],const float *hit_point,
 uint8_t candidate,uint8_t same_object,float distance,float look_point[3]);
#ifdef __cplusplus
}
#endif
#endif
