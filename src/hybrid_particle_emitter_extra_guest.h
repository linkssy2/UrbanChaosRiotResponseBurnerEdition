#ifndef UC_HYBRID_PARTICLE_EMITTER_EXTRA_GUEST_H
#define UC_HYBRID_PARTICLE_EMITTER_EXTRA_GUEST_H
#include <stdint.h>
/* Conditional and fire emitter classes on the existing guest graph. Zero
 * declines before effects. Original shared services and child callbacks run
 * exactly once; their completing lifetime contract is retained. */
int uc_hybrid_particle_emitter_extra(uint32_t address);
#endif
