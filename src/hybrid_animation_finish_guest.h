#ifndef UC_HYBRID_ANIMATION_FINISH_GUEST_H
#define UC_HYBRID_ANIMATION_FINISH_GUEST_H
#include <stdint.h>
/* Only at the pinned 1F1C92 seam, after original callbacks/root allocation.
 * Success publishes a prefix and its prior frame; caller executes the complete
 * original suffix from *first. Decline preserves all guest state and host CSR.
 * Borrowed guest storage must remain stable during this callback-free stage. */
int uc_hybrid_animation_finish(uint32_t state,uint32_t stack,uint32_t *first);
#endif
