#pragma once
#include "integration_lighting.h"
#include "assets.h"
extern "C" {
#include "../model_render.h"
#include "../model_light_membership.h"
#include "../material_prepass.h"
}
struct NativeAnimatedModel;
struct NativeModelLightMaterial { UCMaterialPrepass prepass{};float specular=0;bool supported=false,two_sided=false,receives=false,write_alpha=false,masked=false;ComPtr<ID3D11ShaderResourceView> auxiliary; };
// One persistent prepared light frontier per model. Pose/vertices upload once,
// then all admitted lights reuse the same owned streams. No command capture.
struct NativeModelLightData {
    UCModelRender prepared{};NativeGpuMesh mesh;ComPtr<ID3D11Buffer> vectors;
    std::vector<NativeModelLightMaterial> materials;
    std::vector<UCModelRenderVertex> previous_vertices,present_vertices;
    std::vector<UCModelRenderVectors> previous_vectors,present_vectors;
    UCModelLightMembership *membership=nullptr;uint32_t generation=0;
    std::vector<const UCModelMembershipLight*> points;
    uint64_t pose_revision=0,member_pose=0,member_frame=0;const void *member_owner=nullptr;
    float member_world[16]{};
    bool reset=true;
    ~NativeModelLightData();
    void load(NativeRenderer &,const UCArchive *,const UCModel &,const std::vector<NativeMaterial>&);
    void prepare(NativeRenderer &,const UCModelRender &);
    void present(NativeRenderer &,float);
};
struct NativeModelLightQuery {
    void *owner=nullptr;
    // Actual B2C90 provider+4: receiver center -> light world center; nonzero
    // means blocked. NULL represents B2C90 graphics+94 absent (returns0).
    int (*occluded)(void *,const float receiver_center[3],const float light_center[3])=nullptr;
};
// Shared device resources, actual ambient/light/multiply order. First supported
// domain: ordinary/auxiliary skinned materials and qualified masked hair.
class NativeModelLighting {
    struct Impl;std::unique_ptr<Impl> p_;
public:
    NativeModelLighting(NativeRenderer &,NativeIntegrationLighting &,const std::filesystem::path &shaders);
    ~NativeModelLighting();
    const UCModelLightMembership *prepare_membership(NativeAnimatedModel &,const NativeLightingFrame &,
        const float object_world[16],NativeModelLightQuery query={});
    // Model draw handles texture selection and fallback for unsupported materials.
    // The frame is the ACTUAL owner: world actor or separate weapon manager.
    void draw(NativeAnimatedModel &,NativeRenderer &,const NativeLightingFrame &,
        const float object_world[16],const float object_clip[16],const float tint[4],
        const NativeTextureContext *,float inverse_exposure,bool alpha_exempt,
        NativeModelLightQuery query={});
};
