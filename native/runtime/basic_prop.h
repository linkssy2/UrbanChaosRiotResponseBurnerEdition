#pragma once
#include "mission_world.h"
#include "model.h"
extern "C" {
#include "../basic_prop.h"
#include "../rigid_prop.h"
#include "../solid_block.h"
}
/* Five actual Westside BasicProp records, initialized only through world.ensure.
 * Real authored base/registry/model/mission ownership; type6 body setup is an
 * explicit PC static rounded-box adapter using exact imported2A3E50 shapes.
 * No Havok, impact/audio or general BasicProp variant/rotation implementation.
 * Actual assets have one group, both visibility booleans true, zero rotations,
 * value0 and no difficulty variant. No external flag100 mutation is admitted:
 * then current-player absence or either thermovision query leaves it clear. */
class NativeBasicProp {
    NativeAnimatedModel model_;
    std::vector<UCSolidBlockGeometry> boxes_;
    std::vector<UCModelCollisionShape> shapes_;
    UCBasicPropParameters parameters_{};
    bool ready_=false;
    int failure_=0;
public:
    NativeMissionEntity entity;
    int construct(NativeRenderer &,NativeMissionWorld &,uint32_t source_index,
        const UCArchive *,const uint8_t archive_hash[32],const std::filesystem::path &catalog);
    int update();
    int late()const{return ready_&&!failure_?0:failure_?failure_:1;}
    void draw(NativeRenderer &,const float view_projection[16],float inverse_exposure=1,NativeModelLighting *lighting=nullptr,const NativeLightingFrame *frame=nullptr);
    void collect_shadows(NativeRenderer &,NativeModelLighting &,const NativeLightingFrame &,class NativeModelShadowCasters &,float fraction=1);
    int close_detached();
    const std::vector<UCSolidBlockGeometry>&boxes()const{return boxes_;}
    const std::vector<UCModelCollisionShape>&collision_shapes()const{return shapes_;}
    const UCBasicPropParameters&parameters()const{return parameters_;}
    bool ready()const{return ready_;}
};
