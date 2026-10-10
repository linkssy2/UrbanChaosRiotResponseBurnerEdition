#pragma once
#include "assets.h"
extern "C" {
#include "../ui_motion.h"
}
// Source25CFD0 box and25D070 mode4 tick/cross. kind0=box,1=tick,2=cross.
// The projection is the exact65760 rectangle/color before its floor snapping.
struct NativeObjectiveIconState {
    UCUIMotion motion{};
    float elapsed=0,grow=1;
    uint32_t color=0xffffffffu,kind=0;
    uint8_t started=0,settled=0,finished=0;
};
struct NativeObjectiveIconQuad {float rectangle[4]{};uint32_t color=0;bool draw=false;};
int native_objective_icon_begin(NativeObjectiveIconState &,uint32_t kind,uint32_t box_color);
int native_objective_icon_step(NativeObjectiveIconState &,float delta,NativeObjectiveIconQuad &);
class NativeObjectiveIcons {
    ComPtr<ID3D11ShaderResourceView> textures_[4];
    NativeGpuMesh mesh_;
    NativeObjectiveIconState icons_[3];
    unsigned texture_indices_[3]{};
    unsigned count_=0;
public:
    void load(const UCArchive *,NativeRenderer &);
    //77720 ordinary objective icon=-1 path. Box for0..4; tick3/cross4.
    void begin(int32_t type,uint32_t box_color,int32_t role=-1);
    void clear(){count_=0;}
    void finish(){for(unsigned i=0;i<count_;i++)icons_[i].motion.finishing=1;}
    bool empty()const{return count_==0;}
    //Like original virtual4, each widget advances once immediately before draw.
    void draw(NativeRenderer &,float delta);
};
