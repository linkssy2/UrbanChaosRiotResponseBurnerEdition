#ifndef UC_HYBRID_PARTICLE_REGISTRY_GUEST_H
#define UC_HYBRID_PARTICLE_REGISTRY_GUEST_H
/* Original renderer wrapper registration/removal on the live renderer pool
 * and list. Operations: Line, triangle, shared quad, rotating, ordinary,
 * common removal. No rejection after a completing callback. */
int uc_hybrid_particle_registry(unsigned operation);
#endif
