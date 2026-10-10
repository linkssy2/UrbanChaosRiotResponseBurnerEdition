#pragma once
#include "assets.h"
#include "character_navigation.h"
#include "particle_billboards.h"
extern "C" {
#include "../explosion.h"
#include "../explosion_debris.h"
#include "../archive_lookup.h"
}
#include <array>
//Actual world918: eight retained batches, real shared RNG and collision world.
//Render registration order survives manager swap-removal. Ordinary alpha mode2,
//no fog and read-only depth are the same explicit PC parents as other particles.
class NativeExplosionDebris {
 struct Batch {UCExplosionDebris state{};std::vector<UCExplosionDebrisParticle> particles;ComPtr<ID3D11ShaderResourceView> texture;uint64_t order=0;};
 std::array<std::unique_ptr<Batch>,8> batches_{};unsigned count_=0;uint64_t serial_=0;
 NativeRenderer*renderer_=nullptr;NativeCharacterWorld*world_=nullptr;UCArchiveLookup*lookup_=nullptr;UCGameRandom*random_=nullptr;
 NativeParticleBillboards sprites_;ComPtr<ID3D11SamplerState>sampler_;ComPtr<ID3D11BlendState>blend_;
public:
 void open(NativeRenderer&,NativeCharacterWorld&,UCArchiveLookup&,UCGameRandom&,const std::filesystem::path&shaders);
 int append(uint32_t count,const float position[3],float size,float lifetime,float power,const char*texture);
 int explosion(const UCExplosionParameters&,const float position[3],uint32_t count,float scale,float power,const char*texture);
 void step(float);void draw(NativeRenderer&,const float view[16],const float projection[16]);
 unsigned batches()const{return count_;}
 unsigned particles()const;
};
