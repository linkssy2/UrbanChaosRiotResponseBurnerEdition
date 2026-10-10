#ifndef UCRR_SPECULAR_REMOVAL_H
#define UCRR_SPECULAR_REMOVAL_H
#include <stdint.h>
void uc_specular_removal_init(void);
int uc_specular_removal_enabled(void);
uint8_t uc_specular_removal_filter(uint8_t authored, uint32_t object_va, uint32_t caller_return_va);
void uc_specular_removal_capture_boundary(int start, uint64_t qpc);
#endif
