#ifndef UC_HYBRID_PARTICLE_STATUS_GUEST_H
#define UC_HYBRID_PARTICLE_STATUS_GUEST_H
#include <stdint.h>
/* Complete concrete-family update and finished methods. Zero declines before
 * effects. Retained callbacks own valid live objects and may change CPU/state. */
int uc_hybrid_particle_status(uint32_t entry);
#endif
