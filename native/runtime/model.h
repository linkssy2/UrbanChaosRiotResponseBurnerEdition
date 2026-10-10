#pragma once
#include "assets.h"
#include "model_lighting.h"
extern "C" {
#include "../animation_binding.h"
#include "../model_render.h"
}

/* One explicitly selected model/clip (or authored initial pose with null clip,
 * including models that name an animation template but have no active clip).
 * Gameplay owns selection, time, tint and
 * placement. No inferred equipment, NPC lifecycle or animation event policy. */
struct NativeAnimatedModel {
    UCModel source{};
    UCAnimationBinding binding{};
    UCAnimationSample sampler{};
    UCModelRender prepared{};
    std::vector<NativeMaterial> materials;
    NativeGpuMesh mesh;
    std::shared_ptr<NativeModelLightData> lighting;
    // Actual NPC equipment state, borrowed through pose preparation. NULL has
    // no modifier; named bone resolution/visibility remain the character owner.
    const UCModelEquipment *equipment=nullptr;
    NativeAnimatedModel()=default;
    NativeAnimatedModel(const NativeAnimatedModel&)=delete;
    NativeAnimatedModel &operator=(const NativeAnimatedModel&)=delete;
    ~NativeAnimatedModel();
    void load(NativeRenderer &,const UCArchive *,const uint8_t archive_hash[32],
              const std::filesystem::path &catalog,const char *model,const char *clip,
              const UCArchive *clip_archive=nullptr,UCArchiveLookup *texture_lookup=nullptr,
              uint32_t selected_group=UINT32_MAX,size_t *consumed_prefix=nullptr);
    /* consumed_prefix explicitly admits the qualified model prefix of a full
     * retained resource (e.g. SimpleCutscene trailer); default remains strict.
     * It cannot be combined with group selection, and is published on success. */
    void load_action(NativeRenderer &,const UCArchive *,const uint8_t archive_hash[32],
              const std::filesystem::path &catalog,const char *model,const char *animation_catalog,
              const char *alias,const UCArchive *clip_archive=nullptr);
    void sample(NativeRenderer &,const UCAnimationBlendMath &,float phase,const float pose_root[16]);
    void sample(NativeRenderer &,const UCAnimationBlendMath &,const UCAnimationBindingSample *,size_t count,
                const float pose_root[16],bool graph=false);
    void bind_pose(NativeRenderer &,const float pose_root[16]);
    // Publish an already sampled/modified local pose once per simulation step.
    void prepare(NativeRenderer &,const float (*local)[16],const float pose_root[16]);
    // Body skin publication for all supported material texture sequences;
    // selection uses the existing actual resource list, never a new palette.
    void select_skin(int32_t selection);
    // PC presentation between completed simulation poses. Never advances events.
    void reset_presentation();
    void present(NativeRenderer &,float fraction);
    const UCModelRenderVertex *presented_vertices()const{return presented_fraction_==1||presentation_reset_||presentation_vertices_.size()!=prepared.vertex_count?prepared.vertices:presentation_vertices_.data();}
    bool presentation_bone(uint32_t bone,float fraction,float out[16]) const;
    void draw(NativeRenderer &,const float object_view_projection[16],const float tint[4],
              const NativeTextureContext *texture_context=nullptr,float inverse_exposure=1,bool alpha_exempt=false);
    void draw_lit(NativeRenderer &,NativeModelLighting &,const NativeLightingFrame &,
              const float object_world[16],const float object_clip[16],const float tint[4],
              const NativeTextureContext *texture_context=nullptr,float inverse_exposure=1,
              bool alpha_exempt=false,NativeModelLightQuery query={});
private:
    std::unique_ptr<float[][16]> equipment_local_;
    std::vector<UCTextureSequenceState> skin_states_;
    std::vector<UCModelRenderVertex> previous_vertices_,presentation_vertices_;
    std::vector<float> previous_world_;
    std::vector<uint8_t> previous_valid_;
    bool presentation_reset_=true;float presented_fraction_=1;
};
void native_interpolate_transform(const float before[16],const float after[16],float fraction,float out[16]);
