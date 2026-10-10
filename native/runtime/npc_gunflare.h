#pragma once
#include "mission_world.h"
#include "model.h"
extern "C" {
#include "../gunflare.h"
#include "../particle_registry.h"
}

/* Actual187680 third-person, base-only dynamic GunFlare. Constructs its three
 * real Westside model groups, embedded sprite and event subscriptions under
 * the current Gun parent, then moves both records to priority2. The owner must
 * outlive world.close(): a dynamic deleting callback releases its resources,
 * while the enclosing Gun/host retains this allocation. Event52 admits ordinary
 * pistol2 through the shared particle service; alternate51 remains unsupported.
 * Update/late preserve original counter, model and shared-RNG behavior. */
class NativeNpcFlare {
    struct Impl;std::unique_ptr<Impl> p_;
public:
    NativeMissionEntity entity;
    NativeNpcFlare();~NativeNpcFlare();
    NativeNpcFlare(const NativeNpcFlare&)=delete;
    NativeNpcFlare&operator=(const NativeNpcFlare&)=delete;
    int construct(NativeRenderer &,NativeMissionWorld &,int32_t parent,
        const UCArchive *,const uint8_t hash[32],const std::filesystem::path &catalog,
        UCArchiveLookup &,UCGameRandom &,UCParticleRegistry &,
        const UCGunFlareServices *particle_service=nullptr);
    int update();int late();int remove();int close_detached();
    bool ready()const;
    const UCGunFlare &state()const;
    const UCGunFlareModels *groups()const;
    const UCGunFlareSpriteSource &sprite()const;
    uintptr_t sprite_token()const;
    void draw(NativeRenderer &,const float view_projection[16],float exposure);
};
