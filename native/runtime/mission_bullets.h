#pragma once
#include "mission_world.h"
extern "C" {
#include "../bullet_construct.h"
#include "../bullet_scenery.h"
#include "../bullet_step.h"
}
/*EBF10's one cached Bullet, created lazily in the actual world registry. Each
 * EBDD0 request performs its complete synchronous EBA90 step; retail Bullet
 * virtual30/34 are11030, so there is no invented second frame update.
 * The caller supplies completing collision/effect/tracer owners and keeps
 * them, the world and progression byte alive through world.close().*/
class NativeMissionBullets {
    struct Impl;std::unique_ptr<Impl> p_;
public:
    NativeMissionBullets(NativeMissionWorld &,const uint8_t &progression181,
        const UCBulletScenery &,const UCBulletTracerServices &);
    ~NativeMissionBullets();
    int fire(const UCWeaponBulletRequest &,uint32_t owner);
    uint32_t shots()const;
};
