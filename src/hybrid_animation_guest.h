#ifndef UC_HYBRID_ANIMATION_GUEST_H
#define UC_HYBRID_ANIMATION_GUEST_H
#include <stdint.h>
/* Opt-in private numerical staging at the three closed guest calls. Success
 * publishes matrix output only; the generated source tail then preserves the
 * exact guest CPU and stack ABI. A decline has no guest or host-FP effects.
 * Borrowed inputs must remain stable during each synchronous callback-free call.
 * Locals also require three enabled nonroot bones. Counts 6..256 are admission
 * bounds, not limits of the original engine. Constant-source diagnostics bypass
 * all phases so that every original quaternion trace call remains observable. */
int uc_hybrid_animation_locals(uint32_t state,uint32_t stack,uint32_t *last_enabled);
int uc_hybrid_animation_world(uint32_t model,uint32_t stack);
int uc_hybrid_animation_palette(uint32_t model,uint32_t stack);
#endif
