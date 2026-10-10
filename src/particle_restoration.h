#ifndef UCRR_PARTICLE_RESTORATION_H
#define UCRR_PARTICLE_RESTORATION_H
#include <stdint.h>
void uc_particle_restoration_init(void);
int uc_particle_restoration_enabled(void);
int uc_particle_restoration_admit(uint32_t wrapper, uint32_t frame, uint32_t frame_end,
                                  uint32_t count, uint32_t textured);
void uc_particle_restoration_capture_boundary(int start, uint64_t qpc);
void uc_particle_restoration_draw(void);
#endif
