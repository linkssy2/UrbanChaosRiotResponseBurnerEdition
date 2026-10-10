#ifndef D3D8_CLEAR_RECT_H
#define D3D8_CLEAR_RECT_H
#include <d3d11_1.h>
/* Exclusive right/bottom edges. Empty intersections are no-ops. */
static int d3d8_clip_clear_rect(D3D11_RECT *r, LONG width, LONG height)
{
    if (r->left < 0) r->left = 0;
    if (r->top < 0) r->top = 0;
    if (r->right > width) r->right = width;
    if (r->bottom > height) r->bottom = height;
    return r->left < r->right && r->top < r->bottom;
}
/* ClearView leaves graphics pipeline bindings unchanged. */
static HRESULT d3d8_clear_color_rect(ID3D11DeviceContext1 *context,
    ID3D11RenderTargetView *view, const float color[4], D3D11_RECT rect,
    LONG width, LONG height)
{
    if (!d3d8_clip_clear_rect(&rect, width, height)) return S_OK;
    if (!context) return E_NOTIMPL;
    ID3D11DeviceContext1_ClearView(context, (ID3D11View *)view, color, &rect, 1);
    return S_OK;
}
#endif
