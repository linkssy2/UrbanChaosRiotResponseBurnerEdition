#ifndef UC_HYBRID_PARTICLE_ROTATING_GUEST_H
#define UC_HYBRID_PARTICLE_ROTATING_GUEST_H
#include <stdint.h>
/* Exact39-record rotating phases at pinned9A215 consumer. Returns0 without
 * guest/FP changes so the retained original local loop runs once. Success
 * retains all join state; camera caller seeds equal local comparison flags. */
int uc_hybrid_particle_rotating(uint32_t stack,uint32_t frame);
int uc_hybrid_particle_rotating_camera(uint32_t stack,uint32_t frame);
#endif
