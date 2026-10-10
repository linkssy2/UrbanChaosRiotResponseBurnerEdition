#pragma once
#include "assets.h"
extern "C" {
#include "../game_random.h"
#include "../particle_asset.h"
}
class NativeMissionWorld;
class NativeNpcThrownWeapon;
class NativeRigidProp;
class NativeMissionCharacter;
struct NativeMissionEntity;
/* Real retained219740 source plus45C2BC custom trigger, authored Molotov
 * Trail1/2/3 assets and original particle lifetimes. Borrowed world/weapons
 * remain allocated until this owner is destroyed; registry removal ends births.
 * Step receives the same game RNG and original world particle-phase delta.
 * PC renderer uses the established native particle geometry and render states.*/
class NativeNpcThrownParticles {
 struct Impl;std::unique_ptr<Impl> p_;
public:
 NativeNpcThrownParticles(NativeRenderer&,const UCArchive*,NativeMissionWorld&,const std::filesystem::path&shaders);
 ~NativeNpcThrownParticles();
 int append(const char*asset,NativeNpcThrownWeapon&,bool attached_custom);
 // Same source219740, ordinary asset trigger, used by RigidBody135840/
 //135AE0/135BA0. Borrowed prop remains allocated through system retirement.
 int append(const char*asset,NativeRigidProp&,const char*bone,uint8_t dead_when_hidden);
 //1C3130: live Bip01 Head219740 source, hidden=0, then219A50 row0/2
 //exchange. Uses the authored ordinary trigger; borrowed actor survives
 //until this particle owner is destroyed, even after registry removal.
 int append_cold(NativeMissionCharacter&);
 // EE0A0's code-driven burst and219530 world source. Copies the immutable
 // definition into the retained source; Ring.tga shares this renderer/heat pass.
 int append_ring(NativeMissionEntity&,const UCParticleAsset&);
 //18EB40/219450/2192D0: immutable world matrix copied at each source birth.
 //Only the four authored Shield-bullethitspark families are admitted.
 int append_shield(const char*asset,const float world[16]);
 void step(float,UCGameRandom&);
 void draw(NativeRenderer&,const float view[16],const float projection[16]);
 uint32_t systems()const;uint32_t particles()const;
};
