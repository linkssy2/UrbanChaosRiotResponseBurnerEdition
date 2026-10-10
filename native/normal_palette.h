#ifndef UC_NATIVE_NORMAL_PALETTE_H
#define UC_NATIVE_NORMAL_PALETTE_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Full ADFA0's deterministic 256-entry normal palette, in ARGB byte order.
 * Entries0..254 have alpha255; entry255 is exactly0080FF80 (alpha0).
 * Native double sin/cos/sqrt represent the existing double-backed x87 lift;
 * this does not claim bit-identical Xbox transcendental intermediate state.
 * The caller owns the 256 words. No palette binding or texture ownership. */
void uc_normal_palette(uint32_t output[256]);
#ifdef __cplusplus
}
#endif
#endif
