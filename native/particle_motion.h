#ifndef UC_NATIVE_PARTICLE_MOTION_H
#define UC_NATIVE_PARTICLE_MOTION_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCParticle {
    float position[3],radius,velocity[3];
    uint16_t age,seed;
} UCParticle;
typedef struct UCParticleBounds {float minimum[3],maximum[3],center[3],half_extent[3];} UCParticleBounds;
typedef struct UCParticleMotion {
    float delta,gravity_step[3],wind_step[3],drag,size_start,size_delta;
    uint16_t age_step;
} UCParticleMotion;
/* Complete239330 record-motion projection plus2D38F0 bounds. No spawning,
 * particle source/trigger/descriptor ownership, material or rendering implied.
 * Records are caller-owned, packed32-byte records. count<=capacity<=1048576;
 * disjoint borrowed parameters/count/bounds. Finite float inputs bounded by
 *1048576, delta0..1, RN and gradual underflow. Positive/negative radius and drag
 * follow source arithmetic; no invented clamps. Rejection preserves all state.
 *
 * Stable survivor order and age overflow retirement. Radius uses NEW16-bit
 * age; seed is preserved. Writes only the surviving record prefix, count and
 * bounds. Initial count0 leaves bounds untouched. If ALL particles retire,
 * original FLT_MAX sentinel bounds produce infinite half extents; retained
 * exactly. Bounds describe point positions, not expanded particle radii.
 * SIMD fourth-lane arithmetic/FP status flags are not observable output here;
 * the original packs age/seed in that unused numeric lane. */
int uc_particle_motion(UCParticle *,uint32_t capacity,uint32_t *count,
    const UCParticleMotion *,UCParticleBounds *);
#ifdef __cplusplus
}
#endif
#endif
