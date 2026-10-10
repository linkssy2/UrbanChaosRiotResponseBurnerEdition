#ifndef UC_NATIVE_COLLISION_RAY_H
#define UC_NATIVE_COLLISION_RAY_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCCollisionRay {
    float from[4], to[4];
    uint32_t filter;
} UCCollisionRay;
/* Original fresh-query preparation: render XYZ centimetres to physics XZY
 * metres (W=1), minimum render length, default mask, uint16 type/group and
 * packed filter, then 550-metre segment cap. Returns 1 prepared, 0 too short,
 * -1 invalid/unsupported; non-success preserves output. Finite inputs and
 * intermediates, RN and DAZ/FTZ off; disjoint input/output. Uses host RSQRTSS
 * and source refinement, not the retained lift's reciprocal sqrt. Original
 * x87 length predicate is qualified with double-backed arithmetic only.
 * Assumes a fresh reset query and available world. Does not own the reusable
 * query, counters, traversal, hit cache, impact impulse or result publication. */
int uc_collision_ray_prepare(const float start[3], const float end[3],
    uint32_t type, uint32_t group, uint32_t mask, UCCollisionRay *out);
#ifdef __cplusplus
}
#endif
#endif
