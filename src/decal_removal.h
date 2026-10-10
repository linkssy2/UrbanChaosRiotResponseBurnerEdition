#ifndef UCRR_DECAL_REMOVAL_H
#define UCRR_DECAL_REMOVAL_H
#include <stdint.h>
void uc_decal_removal_init(void);
int uc_decal_removal_enabled(void);
uint8_t uc_decal_removal_filter(uint8_t original, uint32_t object, uint32_t caller);
void uc_decal_removal_render(uint32_t object, uint32_t context);
void uc_decal_removal_visibility_wrapper(void);
void uc_decal_removal_render_wrapper(void);
void uc_decal_removal_capture_boundary(int start, uint64_t qpc);
#endif
