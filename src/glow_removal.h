#ifndef UCRR_GLOW_REMOVAL_H
#define UCRR_GLOW_REMOVAL_H
#include <stdint.h>
void uc_glow_removal_init(void);
int uc_glow_removal_enabled(void);
int uc_glow_removal_filter(uint32_t object_va, uint32_t caller_return_va,
                          uint32_t scissor_count, uint32_t exclusive,
                          uint32_t rect_va, uint32_t width, uint32_t height);
void uc_glow_removal_capture_boundary(int start, uint64_t qpc);
void uc_glow_stage(void);
#endif
