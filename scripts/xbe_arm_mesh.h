#pragma once
#include "xbe_hd_ui_restored.h"
#include "xbe_arm_mesh_data.h"

namespace uc_arm_mesh {
inline bool Apply(std::span<uint8_t> positions,std::span<uint8_t> uv) {
    for(const auto& mesh:meshes) {
        if(positions.size()!=size_t(mesh.vertices)*10||
           !uc_hd_ui::restored::MatchesDigest(positions,mesh.sha256))continue;
        if(uv.size()!=size_t(mesh.vertices)*8)return false;
        const bool sleeve=mesh.vertices==2387;
        if(sleeve&&!uc_hd_ui::restored::MatchesDigest(uv,sleeve_uv_sha256))return false;
        // Continue the open elbow rings toward the shoulders, preserving hands and skin weights.
        for(unsigned i=0;i<mesh.count;++i) {
            int16_t x;std::memcpy(&x,positions.data()+mesh.ring[i]*10,2);
            x=x<0?-5000:5000;
            std::memcpy(positions.data()+mesh.ring[i]*10,&x,2);
            // Preserve fabric texel density along the extended sleeve; glove/cuff UVs are separate.
            if(sleeve)std::memcpy(uv.data()+mesh.ring[i]*8+4,&sleeve_v[i],4);
        }
        return true;
    }
    return false;
}
}
