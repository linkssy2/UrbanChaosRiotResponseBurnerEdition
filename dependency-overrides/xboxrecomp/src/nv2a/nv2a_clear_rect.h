#ifndef NV2A_CLEAR_RECT_H
#define NV2A_CLEAR_RECT_H
/* NV097 stores inclusive 12-bit endpoints, not origin plus dimensions.
 * Inverted rectangles remain empty; out-of-range clipping is host-side. */
static D3DRECT nv2a_decode_clear_rect(uint32_t horizontal, uint32_t vertical)
{
    D3DRECT r;
    r.x1 = horizontal & 0xfff;
    r.y1 = vertical & 0xfff;
    r.x2 = ((horizontal >> 16) & 0xfff) + 1;
    r.y2 = ((vertical >> 16) & 0xfff) + 1;
    return r;
}
#endif
