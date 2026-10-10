#ifndef UCRR_SHADOW_REMOVAL_H
#define UCRR_SHADOW_REMOVAL_H
#include <stdint.h>
void uc_shadow_removal_init(void);
int uc_shadow_removal_enabled(void);
uint8_t uc_shadow_removal_filter(uint8_t original, uint32_t object, uint32_t caller);
void uc_shadow_removal_caster(uint32_t object, uint32_t caller);
void uc_shadow_removal_capture_boundary(int start, uint64_t qpc);
#endif
