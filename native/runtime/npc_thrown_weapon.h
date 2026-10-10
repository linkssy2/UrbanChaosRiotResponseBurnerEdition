#pragma once
#include "mission_character.h"
#include "npc_throw_handler.h"
extern "C" {
#include "../npc_thrown_weapon.h"
}
class NativeNpcThrownWeapon;
/* Actual external projectile28CC20/DBFC0 owner, never preview throwables.
 * construct returns a real retained body, or NULL only when2918A0 says the
 * shared source physics manager is absent. Every requested effect completes
 * or returns failure. Actor and suffix storage remain owned by this class. */
struct NativeNpcThrownServices {
    void *owner=nullptr;
    int(*physics_construct)(void *,NativeNpcThrownWeapon &)=nullptr;
    int(*physics_ignite)(void *,void *body,float)=nullptr;
    int(*physics_source_owned)(void *,void *body,unsigned *)=nullptr;
    int(*physics_launch)(void *,void *body,const float[3],float)=nullptr;
    int(*physics_sync)(void *,NativeNpcThrownWeapon &)=nullptr; //wholeDBFC0
    int(*physics_close)(void *,NativeNpcThrownWeapon &)=nullptr; //DC190
    int(*release_light)(void *,void *point_light)=nullptr; //DD620 first, beforephysics/coroutine
    int(*template_property)(void *,const UCEntityFreshView *,const char *,const char **,unsigned *)=nullptr;
    int(*ignition)(void *,NativeNpcThrownWeapon &)=nullptr; //wholeDB550
    int(*flight)(void *,NativeNpcThrownWeapon &)=nullptr; //wholeDBA40
    int(*explosion)(void *,NativeNpcThrownWeapon &)=nullptr; //wholeDCCD0
    int(*flight_late)(void *,NativeNpcThrownWeapon &)=nullptr; //DC330 launched suffix, before DBFC0
    int(*dropped_template)(void *,NativeNpcThrownWeapon &)=nullptr; //DB230 unarmed template branch
    int(*sound_active)(void *,NativeNpcThrownWeapon &,unsigned *)=nullptr;
    int(*land_sound)(void *,NativeNpcThrownWeapon &)=nullptr; //216C60(material,LandOn)
    int(*owner_fuse_expired)(void *,NativeNpcThrownWeapon &)=nullptr; //39110(owner,event54), DF690 held-Molotov burn request.
    // PC DroppedWeapon adapter: real model/item, vertical scenery fall and
    // bounded inventory admission. Original articulated pickup body unported.
    int(*pickup_floor)(void *,const float from[3],const float to[3],float point[3],uint32_t *material)=nullptr; //-1 failure,0 miss,1 hit
    int(*pickup)(void *,NativeNpcThrownWeapon &,unsigned *taken)=nullptr;
};
struct NativeNpcThrownDescriptor {
    float world[16]{}; //actual2E5D0 mode5 creation matrix, supplied by caller
    int32_t weapon=11,team=0,owner=-1,variation=0;float damage=50;
    bool secondary=false;const char *bank=nullptr;
};
class NativeNpcThrownWeapon {
    struct Impl;std::unique_ptr<Impl> p_;
public:
    NativeMissionEntity entity;UCBulletState base{};UCNpcThrownWeapon state{};
    NativeNpcThrownWeapon();~NativeNpcThrownWeapon();
    int construct(NativeRenderer &,NativeMissionWorld &,NativeMissionCharacter &,
        const UCArchive *,const uint8_t[32],const std::filesystem::path &,
        UCArchiveLookup &,const NativeNpcThrownDescriptor &,const NativeNpcThrownServices &);
    int construct(NativeRenderer &,NativeMissionWorld &,int32_t parent,
        const UCArchive *,const uint8_t[32],const std::filesystem::path &,
        UCArchiveLookup &,const NativeNpcThrownDescriptor &,const NativeNpcThrownServices &);
    int update();int late();int remove();int close_detached();
    int ignite(float);void set_fuse(float);int launch(const float[3],float,float);int drop();
    int set_flags(uint32_t);
    NativeNpcThrownView view()const;
    bool ready()const;
    bool model_tint(float out[4])const; //21B0C0; absent model returnsfalse
    int set_model_tint(const float[4]); //21B0B0, retained by actual draw
    int32_t bone_index(const char*)const;
    bool posed_bone(int32_t,float[16])const;
    void draw(NativeRenderer &,const float[16],float exposure,float fraction,NativeModelLighting *lighting=nullptr,const NativeLightingFrame *frame=nullptr);
};
