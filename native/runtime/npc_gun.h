#pragma once
#include "mission_character.h"
#include "npc_gunflare.h"
extern "C" {
#include "../npc_gun.h"
#include "../npc_gun_shot.h"
}
class NativeNpcGun;
struct NativeNpcGunShotServices {
    void *owner=nullptr;
    int (*bullet)(void *,const UCWeaponBulletRequest &,int32_t parent)=nullptr;
    int (*sound)(void *,NativeNpcGun &,const char *bank,const char *event,unsigned *found)=nullptr;
    int (*lightflash)(void *,const UCShotLightFlashRequest &)=nullptr;
    float flash_position[3]{}; // Source globals51BE10/08/0C, fresh image defaults.
    int32_t player_id=-1;
};
// Concrete27D40/DA120 police handgun. The parent, clock and shared effect
// registry outlive this owner. Shooting/drop events require their real path.
class NativeNpcGun {
    struct Impl;std::unique_ptr<Impl> p_;
public:
    NativeMissionEntity entity;
    NativeNpcGun();~NativeNpcGun();
    int construct(NativeRenderer &,NativeMissionWorld &,NativeMissionCharacter &,
        const UCArchive *,const uint8_t hash[32],const std::filesystem::path &,
        UCArchiveLookup &,UCGameRandom &,UCParticleRegistry &,
        const UCGunFlareServices *particle_service=nullptr);
    int update();int late();int remove();int close_detached();
    bool ready()const;
    void draw(NativeRenderer &,const float view_projection[16],float exposure,float fraction,NativeModelLighting *lighting=nullptr,const NativeLightingFrame *frame=nullptr);
    void draw_effects(NativeRenderer &,const float view_projection[16],float exposure);
    const UCNpcGun &state()const;
    void set_shot_services(const NativeNpcGunShotServices &);
    int fire_position(float out[3]);
    int gun_matrix(float out[16]);
    int reload_sound();
    int shoot(const float direction[3],float scale,unsigned hit);
};
