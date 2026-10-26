#pragma once
#include "xbe_hd_ui_restored.h"
#include "xbe_arm_mesh_data.h"

namespace uc_arm_mesh {
inline bool Apply(std::span<uint8_t> positions,std::span<uint8_t> uv,std::span<uint8_t> skin) {
    for(const auto& mesh:meshes) {
        if(positions.size()!=size_t(mesh.vertices)*10||
           !uc_hd_ui::restored::MatchesDigest(positions,mesh.positions_sha256))continue;
        // All companion streams must match before any write. A partial edit
        // would retain the old forearm-driven stretch or corrupt another skin.
        if(uv.size()!=size_t(mesh.vertices)*8||skin.size()!=size_t(mesh.vertices)*8||
           !uc_hd_ui::restored::MatchesDigest(uv,mesh.uv_sha256)||
           !uc_hd_ui::restored::MatchesDigest(skin,mesh.skin_sha256))return false;
        for(unsigned i=0;i<mesh.count;++i) {
            int16_t x;std::memcpy(&x,positions.data()+mesh.ring[i]*10,2);
            x=x<0?-mesh.shoulder:mesh.shoulder;
            std::memcpy(positions.data()+mesh.ring[i]*10,&x,2);
            std::memcpy(uv.data()+mesh.ring[i]*8+4,&mesh.v[i],4);
            // The second existing palette influence is L/R UpperArm. The old
            // edge was mostly ForeTwist, so moving it alone made a straight
            // tube. Bind the shoulder edge to UpperArm; the original short
            // elbow ring, forearm, hands and animation remain untouched.
            skin[mesh.ring[i]*8+6]=0;
            skin[mesh.ring[i]*8+7]=255;
        }
        return true;
    }
    return false;
}
}
