#pragma once
#include "model.h"
#include "integration_shadows.h"
#include <array>
// PC opaque depth-cube silhouettes with original point membership and authored
// cast enable. Dedicated Xbox stencil geometry/alpha silhouettes are not replaced.
class NativeModelShadowCasters {
    struct Pair {uint32_t first_vertex,vertex_count,first_index,index_count,light;};
    std::vector<std::array<float,3>> positions_;
    std::vector<uint32_t> indices_;std::vector<Pair> pairs_;
    std::vector<NativeShadowCaster> views_;
public:
    static bool eligible(const NativeAnimatedModel &,uint32_t entity_secondary);
    void begin(){positions_.clear();indices_.clear();pairs_.clear();views_.clear();}
    // Already presented base mesh and exact source membership; no camera cull.
    void add(const NativeAnimatedModel &,const float world[16],uint32_t entity_secondary,
        const UCModelLightMembership *,const NativeLightingFrame &);
    void submit(NativeIntegrationShadows &);
    uint32_t pairs()const{return uint32_t(pairs_.size());}
    uint32_t indices()const{return uint32_t(indices_.size());}
};
