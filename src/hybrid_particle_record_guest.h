#ifndef UC_HYBRID_PARTICLE_RECORD_GUEST_H
#define UC_HYBRID_PARTICLE_RECORD_GUEST_H
#include <stdint.h>
/* Optional ordinary world-space particle record phase,16..39 items at the
 * pinned9B4C4 consumer. Returns0 without guest or FP changes; caller resumes
 * the original loop. Success preserves exact join state before239F67. */
int uc_hybrid_particle_record(uint32_t stack,uint32_t frame);
/* Same owner/count/caller contract for the camera-relative239CC4 phase.
 * The original caller sets its local decrement result0 and rejoins239D89. */
int uc_hybrid_particle_record_camera(uint32_t stack,uint32_t frame);
#endif
