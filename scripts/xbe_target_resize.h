#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace uc_target_resize {
// A raw GPU copy cannot resize retained surfaces. Compare host/guest ratios,
// not dimensions: different-sized guest surfaces can still share one scale.
template<class Target>
bool DrawCopy(const Target& src,const Target& dst) {
    return src.fmt!=dst.fmt || uint64_t(src.w)*dst.gw!=uint64_t(dst.w)*src.gw ||
           uint64_t(src.h)*dst.gh!=uint64_t(dst.h)*src.gh;
}
struct CopyScale { float x,y;uint32_t taps; };
template<class Target>
CopyScale DrawScale(const Target& src,const Target& dst) {
    const float x=float(dst.w)/dst.gw,y=float(dst.h)/dst.gh;
    float shrink=std::max({float(src.w)/src.gw/x,float(src.h)/src.gh/y,1.f});
    return {x,y,uint32_t(std::ceil(shrink-1e-3f))};
}
// Match ClearTarget's guest rectangles/viewport. A partial overwrite must never
// destroy an existing GPU-only image, including depth or stencil contents.
template<class Target,class Viewport,class Clear>
bool Complete(const Target& t,const Viewport& viewport,const Clear& clear) {
    const uint32_t planes=t.depth?3u:0xf0u;
    if((clear.flags&planes)!=planes) return false;
    if(clear.count&&clear.rects) {
        for(uint32_t i=0;i<clear.count;++i) {
            int32_t r[4];std::memcpy(r,clear.rects+uint64_t(i)*sizeof(r),sizeof(r));
            if(r[0]<=0&&r[1]<=0&&r[2]>=int64_t(t.gw)&&r[3]>=int64_t(t.gh)) return true;
        }
        return false;
    }
    return viewport.x<=0 && viewport.y<=0 &&
           double(viewport.x)+viewport.w>=t.gw && double(viewport.y)+viewport.h>=t.gh;
}
// Both active attachments must either already have the requested size or be
// fully overwritten together. Never make a new color buffer drop its old depth.
constexpr bool Pair(bool color_changes,bool color_complete,bool depth_changes,bool depth_complete) {
    return (!color_changes||color_complete)&&(!depth_changes||depth_complete);
}
}
