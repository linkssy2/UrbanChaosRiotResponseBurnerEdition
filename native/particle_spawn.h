#ifndef UC_PARTICLE_SPAWN_H
#define UC_PARTICLE_SPAWN_H
#include "particle_motion.h"
#include "game_random.h"
typedef struct UCParticleSpawn {
    uint32_t shape; /* Original descriptor0..2; text-to-enum parser is separate. */
    float size[3],angle_min,angle_max,force_min,force_max,radius;
    float current[16],previous[16]; /* Original row-major source matrices. */
} UCParticleSpawn;
/* One full239510 birth, borrowing the SHARED game RNG. Disjoint owners;
 * no allocation. Bounded finite fields, shape0..2, round-to-nearest/normal
 * denormals. Return1 rejects before mutation; return2 means a degenerate random
 * direction/nonfinite result consumed RNG but did not publish the particle.
 * Source geometry, seed ordering, cone distribution and fixed1/30 birth offset
 * are preserved. CRT acos uses the host double implementation. No draw/manager. */
int uc_particle_spawn(const UCParticleSpawn *,UCGameRandom *,UCParticle *);
#endif
