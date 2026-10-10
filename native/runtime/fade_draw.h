#pragma once
#include "renderer.h"
extern "C" {
#include "../fade.h"
}
/* PC full-viewport endpoint for category1 2225C0: black/white RGB and clamped
 * alpha. Caller submits after scene/HUD, keeps mesh and owns fade scheduling.
 * No category0 viewport border or full original material pipeline claim. */
void native_fade_draw(NativeRenderer &,NativeGpuMesh &,const UCFade &);
