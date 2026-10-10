#pragma once
#include "assets.h"
extern "C" {
#include "../level_asset.h"
#include "../game_random.h"
}
class NativeMissionWorld;
struct NativeCharacterWorld;
struct UCSolidBlockGeometry;
class NativePlayerAudio;
class NativeModelLighting;
struct NativeLightingFrame;
struct NativeLevelParticleCounts {
    uint32_t authored=0,initial=0,deferred=0,active=0,live_particles=0,draws=0;
    uint32_t fire_authored=0,fire_initial=0,fire_deferred=0,fire_active=0,fire_live_particles=0;
    uint32_t prop_authored=0,prop_supported=0,prop_active=0,prop_live_particles=0;
    uint64_t prop_markers=0,prop_restarts=0,prop_source_updates=0,helicopter_group_resets=0;
    uint32_t vehicle_active=0,vehicle_pending=0;
    uint64_t vehicle_pending_ticks=0;
    uint32_t deferred_prop_supported=0,deferred_prop_active=0,deferred_prop_pending=0;
    uint64_t deferred_prop_pending_ticks=0;
    uint64_t ticks=0;
};
/* Authored ParticleSystem count0/on-flag-count0 stationary self attachments.
 * Owns immutable target transforms and real native particle lifetimes. Initial
 * descriptor active0 starts immediately; active1 waits for activate(index).
 * That method must be called by the real mission activation owner. Optional
 * FXFire supplies six authored visual emitters to NativeMissionFire, which
 * owns damage, sound and routing for its admitted mission properties. The regular/fire
 * targets are stationary permanent projections. Optional visual AnimatingProps
 * use real same-world IDs, controller poses and current bone attachment; their
 * generic physics/audio body, forced destruction and class coroutine are not
 * claimed. Actual registry detachment stops their emitter births.
 * The fixed-source projection equals 219530/2194B0 while these targets remain
 * alive and stationary. All transforms are retail render centimetres XYZ.
 * PC presentation parents: zero global wind, fog/thermal off, read-only depth;
 * authored blend/color/size/velocity/gravity and textures are preserved.
 * Borrows level strings/transforms until destruction; shared RNG per step. */
class NativeLevelParticles {
public:
    NativeLevelParticles(NativeRenderer&,const std::filesystem::path &game,
        const std::filesystem::path &shaders,const UCLevel&,bool include_fire=false);
    ~NativeLevelParticles();
    NativeLevelParticles(const NativeLevelParticles&)=delete;
    NativeLevelParticles&operator=(const NativeLevelParticles&)=delete;
    bool activate(uint32_t descriptor);
    bool deactivate(uint32_t descriptor);
    int construct_particle(NativeMissionWorld&,uint32_t descriptor);
    /* Prepare the six windysmoke/mist models and optionally two firebarrels;
     * no entity is activated. Firebarrels use their three real Idle markers.
     * Optional deferred courier/end-scene resources retain active1; only an
     * actual same-world factory request creates their IDs, poses and draws.
     * The real mission factory calls construct_prop while its active descriptor
     * is published. Same world owns IDs/records/flags and drives animation.
     * World must close before this owner; models/sources remain alive until then.
     * Explicit PC visual class projection: physics/body/audio helper unconnected. */
    void prepare_props(NativeRenderer&,const std::filesystem::path &assets,NativeMissionWorld&,bool include_firebarrels=false,bool include_vehicles=false,bool include_deferred=false);
    void prepare_helicopters(NativePlayerAudio&,const std::filesystem::path &assets);
    int construct_prop(NativeMissionWorld&,uint32_t descriptor);
    void draw_props(NativeRenderer&,const float view_projection[16],float inverse_exposure=1,uint32_t descriptor=UINT32_MAX,NativeModelLighting *lighting=nullptr,const NativeLightingFrame *frame=nullptr);
    /* Actual truck/ambulance/courier/end-scene conditions borrow the SAME world flags. Non-Idle
     * actions except the courier rollover remain explicitly pending before animation/audio/event effects;
     * their streaming spatial sound and scripted collision are not completed.
     * Returns -1 for no pending action; otherwise source action index0..7. */
    int32_t pending_prop_action(uint32_t descriptor)const;
    void step(float delta,UCGameRandom&);
    void append_blocks(std::vector<const UCSolidBlockGeometry*>&)const;
    void append_collision(NativeCharacterWorld&)const;
    bool collision_owner(int32_t id)const;
    /* Optional descriptor selects one real emitter family for inspection. */
    void draw(NativeRenderer&,const float view[16],const float projection[16],uint32_t descriptor=UINT32_MAX);
    const NativeLevelParticleCounts&counts()const;
private:
    struct Impl;std::unique_ptr<Impl> impl;
};
