#pragma once
#include "mission_character.h"
extern "C" {
#include "../medic_weapon.h"
}
struct NativeMedicWeaponParent {
    NativeMissionEntity *entity=nullptr;void*owner=nullptr;
    int32_t(*bone)(void*,const char*)=nullptr;
    bool(*pose)(void*,int32_t,float[16])=nullptr;
};
// Source2B180 creates this real type34 MeleeWeapon, with collision disabled.
// Registry/body/model lifetimes are shared with the actual owning character.
class NativeMedicWeapon {
    struct Impl;std::unique_ptr<Impl> p_;
public:
    NativeMissionEntity entity;
    NativeMedicWeapon();~NativeMedicWeapon();
    int construct(NativeRenderer &,NativeMissionWorld &,NativeMissionCharacter &,
        const UCArchive *,const uint8_t hash[32],const std::filesystem::path &,UCArchiveLookup &,uint32_t type=34,const float *pillbox_world=nullptr);
    int construct(NativeRenderer &,NativeMissionWorld &,const NativeMedicWeaponParent &,
        const UCArchive *,const uint8_t hash[32],const std::filesystem::path &,UCArchiveLookup &,uint32_t type=34,const float *pillbox_world=nullptr);
    int update();int late();int remove();int close_detached();
    int set_flags(uint32_t);int property(const char*,int32_t&)const;
    bool ready()const;
    const UCMedicWeapon&state()const;
    void player_provider(void*owner,int(*get)(void*,int32_t*));
    void draw(NativeRenderer &,const float view_projection[16],float exposure,float fraction,NativeModelLighting *lighting=nullptr,const NativeLightingFrame *frame=nullptr);
};
inline int NativeMedicWeapon::construct(NativeRenderer&r,NativeMissionWorld&w,NativeMissionCharacter&p,
 const UCArchive*a,const uint8_t hash[32],const std::filesystem::path&catalog,UCArchiveLookup&lookup,uint32_t type,const float*pillbox_world){
    NativeMedicWeaponParent parent{&p.entity,&p,[](void*v,const char*n){return static_cast<NativeMissionCharacter*>(v)->bone_index(n);},
        [](void*v,int32_t n,float*out){return static_cast<NativeMissionCharacter*>(v)->posed_bone_local(n,out);}};
    return construct(r,w,parent,a,hash,catalog,lookup,type,pillbox_world);
}
