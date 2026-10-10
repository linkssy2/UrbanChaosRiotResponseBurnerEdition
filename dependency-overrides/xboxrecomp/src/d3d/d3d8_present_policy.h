#ifndef D3D8_PRESENT_POLICY_H
#define D3D8_PRESENT_POLICY_H
#include <dxgi.h>

/* Host-only, opt-in presentation experiment. Keep the legacy GDI mirror and
 * exclusive swapchains on their existing route. No guest clocks are changed. */
static int d3d8_configure_flip(DXGI_SWAP_CHAIN_DESC *desc, int requested, int gdi)
{
    if (!requested || gdi || !desc->Windowed || desc->SampleDesc.Count != 1 ||
        desc->BufferCount > 16) return 0;
    desc->SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    if (desc->BufferCount < 2) desc->BufferCount = 2;
    return 1;
}
#endif
