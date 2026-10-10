#ifndef D3D8_BLEND_ALPHA_H
#define D3D8_BLEND_ALPHA_H
/* D3D8's RGBA factor must be split into legal D3D11 RGB and alpha factors. */
static D3D11_BLEND d3d8_blend_alpha_factor(D3D11_BLEND factor)
{
    switch(factor) {
    case D3D11_BLEND_SRC_COLOR:return D3D11_BLEND_SRC_ALPHA;
    case D3D11_BLEND_INV_SRC_COLOR:return D3D11_BLEND_INV_SRC_ALPHA;
    case D3D11_BLEND_DEST_COLOR:return D3D11_BLEND_DEST_ALPHA;
    case D3D11_BLEND_INV_DEST_COLOR:return D3D11_BLEND_INV_DEST_ALPHA;
    case D3D11_BLEND_SRC_ALPHA_SAT:return D3D11_BLEND_ONE;
    default:return factor;
    }
}
#endif
