#ifndef NV2A_TEXTURE_ADDRESS_H
#define NV2A_TEXTURE_ADDRESS_H
/* NV2A wrap/mirror/edge-clamp match the corresponding D3D8 values.
 * Border/GL-clamp variants need separate border semantics; do not alias them. */
static unsigned nv_texture_address(unsigned packed, unsigned axis)
{
    if (axis > 2) return 0;
    unsigned mode = (packed >> (axis * 8)) & 15;
    return mode >= 1 && mode <= 3 ? mode : 0;
}
#endif
