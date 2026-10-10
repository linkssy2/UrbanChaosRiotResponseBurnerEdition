#pragma once
#include "model.h"
extern "C" {
#include "../level_asset.h"
}
#include <memory>

struct NativeActorView {
    uint32_t entity_index=0,spawn_index=0,behaviour_index=0,faction=0;
    const char *name=nullptr,*model=nullptr,*clip=nullptr;
    float world[16]{},low[3]{},high[3]{},phase=0,pending_damage=0;
    uint32_t hits=0,unhandled_animation_events=0;
    int32_t next_behaviour=-1,pending_behaviour=-1;
    float behaviour_elapsed=0;
    uint32_t behaviour_updates=0,missing_sound_events=0;
};
struct NativeActorHit {
    uint32_t actor=0,entity_index=0,triangle=0,faction=0;
    float fraction=0,position[3]{},normal[3]{};
};
/* Explicit PC actor-preview activation, NOT mission startup or AI. Borrow the
 * loaded Westside level for this owner's lifetime. Every selected entity must
 * be an authored Character/CharacterCop with a CharacterSpawnPoint and an
 * actual PlayLoopedAnimBehaviour. Its own model, spawn transform and clip are
 * used without replacement assets or arena coordinates. All selected material
 * draws must be supported. No NPC equipment, audio, navigation or health/death
 * state machine is implied. Animation events remain counted, not completed.
 *
 * Hit tests use the SAME completed skinned triangles that are rendered, as an
 * explicit PC geometric query, not recovered character collision bodies.
 * Render centimetres XYZ at the API; caller compares fraction with its static
 * world hit before publishing damage. Only the explicit selection is activated;
 * its original mission activation gates are not evaluated by this preview.
 * PlayLoopedAnim update uses the recovered controller and borrows the SAME
 * mission flag bytes. A transition to an unsupported next behaviour stops at
 * that exact boundary and exposes pending_behaviour, never completing fake AI.
 * Animation/body, behaviour construction and activation remain PC adapters.
 */
class NativeIntegrationActors {
public:
    NativeIntegrationActors(NativeRenderer &,const std::filesystem::path &game,
        const std::filesystem::path &assets,const UCLevel &,
        const uint32_t *selected_entities,size_t count,
        const uint8_t *mission_flags=nullptr,size_t flag_count=0);
    ~NativeIntegrationActors();
    NativeIntegrationActors(const NativeIntegrationActors&)=delete;
    NativeIntegrationActors&operator=(const NativeIntegrationActors&)=delete;
    void step(NativeRenderer &,float delta);
    void draw(NativeRenderer &,const float view_projection[16],float inverse_exposure=1,float fraction=1,NativeModelLighting *lighting=nullptr,const NativeLightingFrame *frame=nullptr);
    void collect_shadows(NativeRenderer &,NativeModelLighting &,const NativeLightingFrame &,class NativeModelShadowCasters &,float fraction=1);
    bool raycast(const float from[3],const float to[3],NativeActorHit &) const;
    // Exact ordinary E0470 publication only. It overwrites pending amount;
    // DFC60 -> DFBD0 -> character-type DDCA0/response is still unconnected.
    bool damage(uint32_t actor,float amount);
    size_t size() const;
    const NativeActorView &actor(size_t index) const;
private:
    struct Impl;std::unique_ptr<Impl> impl;
};
