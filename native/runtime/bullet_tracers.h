#pragma once
#include "particle_quads.h"
#include "../archive_lookup.h"
#include "../bullet_tracers.h"
#include <filesystem>
/*Actual world+910 collection, texture and9A8A0 triangle submission. Audio
 * services borrow the shared mission owner. Lifecycle follows55610, one fade
 * step per positive tick. Output uses the existing PC particle shader route.*/
class NativeBulletTracers {
public:
    void open(NativeRenderer &,UCArchiveLookup &,const std::filesystem::path &shaders,const UCBulletTracersServices &);
    void append(const float from[3],const float to[3],const float rgba[4],float width);
    void step(float delta);
    void draw(NativeRenderer &,const float view[16],const float projection[16]);
    uint32_t count()const{return state_.count;}
private:
    UCBulletTracers state_{};
    UCBulletTracersServices services_{};
    NativeParticleQuads triangles_;
    NativeRenderer *renderer_=nullptr;
    ComPtr<ID3D11ShaderResourceView> texture_;
    ComPtr<ID3D11SamplerState> sampler_;
};
