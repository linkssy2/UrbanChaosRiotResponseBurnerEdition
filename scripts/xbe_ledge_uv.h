#pragma once
#include "xbe_hd_ui_restored.h"
#include "xbe_ledge_uv_data.h"

namespace uc_ledge_uv {
// Repair only collapsed ledge V coordinates in exact retail static meshes.
inline bool Apply(std::span<const uint8_t> positions,std::span<uint8_t> uv) {
    for(const auto& node:nodes) {
        if(positions.size()!=size_t(node.vertices)*16||uv.size()!=size_t(node.vertices)*8)continue;
        if(!uc_hd_ui::restored::MatchesDigest(positions,node.positions)||
           !uc_hd_ui::restored::MatchesDigest(uv,node.uv))continue;
        for(uint32_t i=0;i<node.count;++i)
            if(edits[node.first+i].vertex>=node.vertices)return false;
        for(uint32_t i=0;i<node.count;++i) {
            const auto& edit=edits[node.first+i];
            std::memcpy(uv.data()+size_t(edit.vertex)*8+4,&edit.value,4);
        }
        return true;
    }
    return false;
}
}
