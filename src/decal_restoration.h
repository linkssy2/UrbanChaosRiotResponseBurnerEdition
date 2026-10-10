#ifndef UC_DECAL_RESTORATION_H
#define UC_DECAL_RESTORATION_H
#include <stdint.h>
#ifndef UCRR_DECAL_RESTORATION
#define UCRR_DECAL_RESTORATION 0
#endif
void uc_decal_restoration_init(void);
int uc_decal_restoration_enabled(void);
void uc_decal_restoration_callback_wrapper(void);
void uc_decal_restoration_capture_boundary(int start, uint64_t qpc);
#endif
