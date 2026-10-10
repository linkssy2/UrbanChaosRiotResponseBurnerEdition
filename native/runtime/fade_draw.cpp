#include "fade_draw.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
void native_fade_draw(NativeRenderer &r,NativeGpuMesh &mesh,const UCFade &fade){
    if(!std::isfinite(fade.alpha)||fade.alpha<0||fade.alpha>1.1f||fade.white>1)
        throw std::invalid_argument("Invalid native fade");
    if(fade.alpha==0)return;
    if(!mesh.vertices){
        const NativeVertex vertices[]={{{-1,1,0},{0,0}},{{1,1,0},{0,0}},{{1,-1,0},{0,0}},{{-1,-1,0},{0,0}}};
        const uint32_t indices[]={0,1,2,0,2,3};mesh=r.upload(vertices,4,indices,6);
    }
    const float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    const float tint[4]={float(fade.white),float(fade.white),float(fade.white),std::min(fade.alpha,1.f)};
    NativeDrawState state;state.depth_disabled=true;state.blend_mode=1;state.write_alpha=false;
    r.draw(mesh,identity,tint,nullptr,0,0,state);
}
