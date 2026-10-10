#pragma once
#include "renderer.h"
#include <filesystem>
#include <memory>
extern "C" {
#include "../gunflare.h"
}
struct NativeFirstperson;
struct NativeWeaponEffectsCounts {
    uint32_t shots=0,particle_requests=0,live_systems=0,live_particles=0;
    uint32_t model_draws=0,particle_draws=0;
};
/* PC preview scheduling around the existing GunFlare/model/particle owners.
 * Borrow the same simulation RNG; queue once from the completed real shot
 * callback, step AFTER the player completes weapon placement, draw AFTER the
 * first-person gun. No entity factory or full original firing-chain claim. */
class NativeIntegrationWeaponEffects {
public:
    NativeIntegrationWeaponEffects(NativeRenderer &,const std::filesystem::path &game,
        const std::filesystem::path &assets,const std::filesystem::path &shaders);
    ~NativeIntegrationWeaponEffects();
    NativeIntegrationWeaponEffects(const NativeIntegrationWeaponEffects&)=delete;
    NativeIntegrationWeaponEffects&operator=(const NativeIntegrationWeaponEffects&)=delete;
    bool shot(uint32_t item);
    void step(const NativeFirstperson &,float delta,UCGameRandom &);
    void draw(NativeRenderer &,const NativeFirstperson &,const float object_view[16],const float camera_world[16],
        float aspect,float inverse_exposure=1,float fraction=1,bool particles=true);
    // Shared world particle owner, also used by original third-person GunFlare.
    UCGunFlareServices particle_service();
    void draw_world(NativeRenderer &,const float view[16],const float projection[16],const float camera_world[16]);
    const NativeWeaponEffectsCounts &counts()const;
private:
    struct Impl;std::unique_ptr<Impl> impl;
};
