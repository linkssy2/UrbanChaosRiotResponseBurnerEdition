#pragma once
#include "renderer.h"
#include "../depth_blur.h"
#include <memory>
#include <string>
/*920B0 on the existing PC RGBA8/depth surfaces: full-view linear downsample,
 * original A7710 sigma1.25, original A6F00 table, then point composite with
 * SRC_ALPHA/INV_SRC_ALPHA and RGB-only writes. No per-frame allocations/readback.
 * Uses modern normalized UV/depth projection; no physical NV2A raster claim. */
class NativeDepthBlur {
 struct State;std::unique_ptr<State>state_;std::string shader_;
public:
 explicit NativeDepthBlur(const std::string &filter_shader);
 ~NativeDepthBlur();
 void draw(NativeRenderer&,float m22,float m32,const UCDepthBlurSettings&);
};
