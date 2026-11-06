#pragma once
#include "gpu/vertex.h"
#include "xbe_hd_ui_restored.h"
#include <dxgiformat.h>

namespace uc_road_barrier {
// The near road edge in retail Canals has one-sided metal strips. The opening
// camera crosses them. Both ambient/depth and texture passes need the reverse
// face: the texture pass multiplies existing color without writing depth.
inline bool TwoSided(const gpu::State& s,const gpu::Assembled& a) {
    if(a.topology!=gpu::Topology::Triangles || !a.strip || !a.index16 || !a.index_src ||
       a.vertex_count!=80 || a.draw_count || a.pretransformed || a.repacked ||
       !(a.regs&1) || (a.decode&15) || !s.vs || !s.ps || !(s.vs_handle&1) ||
       s.vs->program_type!=0x2078 ||
       (s.vs->program.size()!=32 && s.vs->program.size()!=24))return false;
    const bool prepass=s.vs->program.size()==24;
    if(prepass && (a.regs!=1 || s.vs->decl.size()!=3 ||
       s.vs->decl[0]!=0x20000000 || s.vs->decl[1]!=0x40320000 || s.vs->decl[2]!=0xFFFFFFFF ||
       !s.simple[gpu::kMZWriteEnable>>2] || s.simple[gpu::kMBlendEnable>>2] ||
       (s.simple[gpu::kMColorWriteEnable>>2]&0x10101)!=0x10101))return false;
    const char* positions=nullptr;
    const char* indices=nullptr;
    if(a.index_count==119 && a.base_vertex==0) {
        positions="07ea5d467eda9b6236c83a935728c3cead6d3afc71773f4df66b358bd8629c25";
        indices="2a5dbfa7ee9928c2e6ef6c01531952755f7c6a94b44a5f26d4d60b607b51f5f7";
    } else if(a.index_count==115 && a.base_vertex==-2553) {
        positions="e8c5b92c628e3ab562e4c5cae0b5c2753c1ec50d0a7e8e6759c0d593de0e66ab";
        indices="c0581bb2a4454ab9b02c182e8fd9315a5cfc38000b274663c22c597baed08d85";
    } else return false;
    const auto& element=a.elements[0];
    if(element.format!=DXGI_FORMAT_R32G32B32_FLOAT || element.offset ||
       element.buffer>=a.buffer_count || a.buffer_count>a.buffers.size())return false;
    const auto& buffer=a.buffers[element.buffer];
    if(!buffer.src || buffer.stride!=16 || buffer.bytes!=1280)return false;
    // All cheap shape checks precede hashing; no pointer cache can outlive a
    // resource or hide changed bytes. Existing SHA256 support also serves Proton.
    using uc_hd_ui::restored::MatchesDigest;
    return MatchesDigest({buffer.src,buffer.bytes},positions) &&
        MatchesDigest({static_cast<const uint8_t*>(a.index_src),size_t(a.index_count)*2},indices) &&
        MatchesDigest({reinterpret_cast<const uint8_t*>(s.vs->program.data()),s.vs->program.size()*4},
            prepass?"cb77f10155ca286b59ded767748bbbf1b5ec718ce97cd553a6fadebb30556c81":
            "f344e7896cc354ee5273cc00bf1304629a6943f8f093386f20c75bde1a6ad88f") &&
        MatchesDigest({reinterpret_cast<const uint8_t*>(s.ps->def),sizeof(s.ps->def)},
            prepass?"ac4445efa2005f2f249e0044276b47f33d600abee238e7e2982653753490b1c2":
            "f36e9362bd742fed0a6d44b9c41439b5e62b5971976e488c37cd12d71037b00c");
}
}
