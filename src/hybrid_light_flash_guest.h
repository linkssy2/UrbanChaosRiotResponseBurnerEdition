#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Existing LightFlash guest owner. Zero means no guest state was changed. */
int uc_hybrid_light_flash(uint32_t entry);
#ifdef __cplusplus
}
#endif
