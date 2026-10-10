#ifndef UC_HYBRID_PARTICLE_EMITTER_GUEST_H
#define UC_HYBRID_PARTICLE_EMITTER_GUEST_H
#include <stdint.h>
/* Complete concrete emitter methods on the original guest objects. Zero
 * declines before effects. Completing pool callbacks retain their original
 * ownership contract; CRT primitives remain original and execute once. */
int uc_hybrid_particle_emitter(uint32_t address);
#endif
