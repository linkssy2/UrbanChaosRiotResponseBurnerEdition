#ifndef UC_HYBRID_ANIMATION_SAMPLER_GUEST_H
#define UC_HYBRID_ANIMATION_SAMPLER_GUEST_H
#include <stdint.h>
/* At original1F3CAC, after the original interval/cache update. Returns the
 * first retained source track (lower is the live local EBP), or zero with all guest state/MXCSR unchanged.
 * Only bounded primary-RAM, disjoint, callback-free prefixes are admitted. */
uint32_t uc_hybrid_animation_sampler(uint32_t clip,uint32_t stack,uint32_t lower);
#endif
