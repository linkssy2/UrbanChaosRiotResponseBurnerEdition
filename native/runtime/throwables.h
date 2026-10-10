#pragma once
#include "assets.h"
class NativeMissionWorld;
class NativeModelLighting;
struct NativeLightingFrame;
extern "C" {
#include "../collision_asset_query.h"
#include "../game_random.h"
#include "../throwable.h"
}
struct NativeThrowableCounts {
    uint64_t throws=0,hits=0,expired=0,particle_requests=0;
    uint32_t live_projectiles=0,live_systems=0,live_particles=0,model_draws=0,particle_draws=0;
};
/* Explicit PC static-world Molotov preview: original launch arithmetic,
 * projectile model and authored burst particles; straight segment collision
 * each fixed tick and gravity -980cm/s² replace the unconnected rigid-body
 * owner. Caller eye frame supplies the launch origin, not original tag6.
 * No dynamic hits, NPC damage, spreading fire, scorch or explosion audio claim.
 * One borrowed world and session game RNG; no reseeding or save changes. */
class NativeIntegrationThrowables {
public:
    NativeIntegrationThrowables(NativeRenderer&,const std::filesystem::path&game,
        const std::filesystem::path&catalogs,const std::filesystem::path&shaders,
        const UCCollisionAsset&world);
    ~NativeIntegrationThrowables();
    NativeIntegrationThrowables(const NativeIntegrationThrowables&)=delete;
    NativeIntegrationThrowables&operator=(const NativeIntegrationThrowables&)=delete;
    // Returns false before effects for capacity/invalid launch; caller debits
    // its SAME inventory only after true. No callbacks during submission.
    bool throw_molotov(const float eye_frame[16],float pitch,float charge);
    void step(float delta,UCGameRandom&);
    void draw(NativeRenderer&,const float view[16],const float projection[16],float inverse_exposure,float fraction=1,NativeModelLighting *lighting=nullptr,const NativeLightingFrame *frame=nullptr);
    const NativeThrowableCounts&counts()const;
private:
    struct Impl;std::unique_ptr<Impl> impl;
};
// Authored F62D0/F69B0 throw controller for Westside's type0, node-target,
// nonrandom, non-avoidance profile. Heap lifetime belongs to the mission world.
using NativeScriptedMolotovCreate=int32_t(*)(void *,int32_t parent,const float frame[16],const float target[3],float time,float &delay);
int native_construct_scripted_thrower(NativeMissionWorld &,uint32_t,UCGameRandom &,void *,NativeScriptedMolotovCreate);
