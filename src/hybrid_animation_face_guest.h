#ifndef UC_HYBRID_ANIMATION_FACE_GUEST_H
#define UC_HYBRID_ANIMATION_FACE_GUEST_H
#include <stdint.h>
/* Called once at2B6308, after the original prologue and record-pointer setup.
 * Zero means unchanged fallback. Nonzero commits only a private matrix prefix;
 * the original last active record and trailing skipped records finish the ABI. */
uint32_t uc_hybrid_animation_face(uint32_t node,uint32_t stack,uint32_t frame);
#endif
