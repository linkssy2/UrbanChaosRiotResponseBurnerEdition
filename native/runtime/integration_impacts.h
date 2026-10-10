#pragma once
#include "assets.h"
extern "C" {
#include "../collision_asset.h"
#include "../collision_asset_query.h"
#include "../game_random.h"
#include "../vector_random.h"
#include "../level_asset.h"
#include "../player_footstep.h"
#include "../bullet_impact.h"
}
struct NativeImpactCounts {
    uint64_t hits=0,particle_requests=0,decal_requests=0,decal_marks=0,decal_rejected=0;
    uint64_t explosion_visuals=0;
    uint32_t live_systems=0,live_particles=0,marks=0,particle_draws=0,decal_draws=0;
};
struct NativeCharacterWorld;
struct NativeBulletImpactServices {
    void *owner=nullptr;
    const uint8_t *blood_enabled=nullptr,*session_mode=nullptr;
    int (*camera)(void *,float frame[16],uint8_t *present)=nullptr;
    int (*sound)(void *,const char *bank,const char *event,uint32_t material,const float point[3])=nullptr;
};
/* PC visual-only connection of a real static ray hit to retail impact recipes,
 * particle lifetimes and decal geometry. No Bullet/audio/damage equivalence.
 * Physics metres XYZ become render centimetres XZY; normals only swap Y/Z.
 * Borrows the session's game RNG and separate original vector RNG per call.
 * Explicit PC parents: neutral decal ambient, zero wind, blood enabled,
 * ordinary thermal/fog off, CRT table seed1, read-only depth. Source fade0
 * marks persist in a bounded overwriting ring. World/camera matrices are
 * row-major. Draw restores the main shader/layout/constants and parent states.
 * Single thread; errors throw and require session teardown, never retry. */
class NativeIntegrationImpacts {
public:
    NativeIntegrationImpacts(NativeRenderer&,const std::filesystem::path &game,
        const std::filesystem::path &catalogs,const std::filesystem::path &shaders);
    ~NativeIntegrationImpacts();
    NativeIntegrationImpacts(const NativeIntegrationImpacts&)=delete;
    NativeIntegrationImpacts&operator=(const NativeIntegrationImpacts&)=delete;
    void hit(const UCCollisionAsset&,const UCCollisionRayHit&,uint32_t item,UCGameRandom&,UCVectorRandom&);
    // Real EB900 static Bullet packet/state, render-centimetre coordinates.
    // Retains the same PC visual parents above; audio/profile/camera are actual
    // completing caller services. Offline session only; no dynamic-target damage.
    int bullet_impact(const NativeCharacterWorld &,const UCEntityFreshView &,UCBulletState &,
        const UCBulletImpact &,UCGameRandom &,UCVectorRandom &,const NativeBulletImpactServices &);
    // Source supplied-tangent footprint geometry and four-corner admission into
    // this SAME PC decal ring. Uses the live scenery/prop query; neutral ambient.
    void footprint(const NativeCharacterWorld &,const UCPlayerFootprint &);
    //Actual E9130 plus shared NULL-tangent decal path. Vector RNG belongs to
    //the session; no mark/geometry draw is consumed when the source ray misses.
    int explosion_scorch(const NativeCharacterWorld &,const float position[3],UCVectorRandom &);
    int scorch(const NativeCharacterWorld &,const float point[3],const float normal[3],
        const float size[2],uint32_t mask,UCVectorRandom &);
    int surface_decal(const NativeCharacterWorld &,const char *shader,const float point[3],const float normal[3],
        const float size[2],uint32_t mask,uint32_t flags,UCVectorRandom &);
    /* E98E0's four standard particle requests only, render-centimetre position.
     * Reuses this owner's lifetime/registry and caller's subsequent shared RNG.
     * No RNG at submission. Not an Explosion factory: damage/audio/physics and
     * camera/warp child effects must be completed by the eventual caller. */
    void explosion_particles(const float position[3]);
    // One actual E98E0/F8210 fixed-source request (Std, Gas or Molotov).
    // Does not emit the separate warp/camera/flash children. Shared owner/RNG.
    int fixed_particle(const char *asset,const float frame[16]);
    /* Explicit PC inspection of an authored standard Explosion. Does not
     * activate a mission slot or claim damage/audio/physics completion. */
    void preview_explosion(const UCLevelEntity &);
    void step(float delta,UCGameRandom&);
    void draw(NativeRenderer&,const float view[16],const float projection[16],float inverse_exposure);
    const NativeImpactCounts&counts()const;
private:
    struct Impl;std::unique_ptr<Impl> impl;
};
