#ifndef UCRR_LIGHT_REMOVAL_H
#define UCRR_LIGHT_REMOVAL_H
#include <stdint.h>
void uc_light_removal_init(void);
int uc_light_removal_enabled(void);
int uc_light_removal_admit(uint32_t object_va, uint32_t caller_return_va);
void uc_light_removal_capture_boundary(int start, uint64_t qpc);
void uc_light_removal_begin(void);
/* Optional user setting. Legacy experiment processes remain immutable unless
 * UCRR_LIGHT_REMOVAL_LIVE=1 was present at initialization. Requests may come
 * from UI threads; apply_pending must run on the initialized owner only after
 * a complete frame (or before the very first guest render). 1 means original
 * lighting is enabled, 0 means selected lights are removed. */
int uc_light_removal_live_toggle_enabled(void);
int uc_light_removal_request_lighting(int on);
int uc_light_removal_requested_lighting(void);
int uc_light_removal_active_lighting(void);
void uc_light_removal_apply_pending(void);
#endif
