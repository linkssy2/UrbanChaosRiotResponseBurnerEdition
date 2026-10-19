#ifndef UCRR_SHADOW_REMOVAL_H
#define UCRR_SHADOW_REMOVAL_H
#include <stdint.h>
void uc_shadow_removal_init(void);
int uc_shadow_removal_enabled(void);
/* Effective point-light caster suppression on the initialized render owner.
 * Read-only: unlike the diagnostic filter, this never updates counters. */
int uc_shadow_point_shadows_suppressed(void);
uint8_t uc_shadow_removal_filter(uint8_t original, uint32_t object, uint32_t caller);
void uc_shadow_removal_caster(uint32_t object, uint32_t caller);
void uc_shadow_removal_capture_boundary(int start, uint64_t qpc);
#endif
