#ifndef UC_NPC_GUN_SHOT_H
#define UC_NPC_GUN_SHOT_H
#include "npc_gun.h"
#include "weapon_bullet.h"
#include "shot_lightflash.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCNpcGunShotServices {
    void *owner;
    int (*bullet)(void *,const UCWeaponBulletRequest *,int32_t parent);
    int (*sound)(void *,void *bank,const char *event,unsigned *found);
    int (*flare)(void *,int32_t target,int32_t sender,int32_t weapon,uint8_t secondary);
    int (*lightflash)(void *,const UCShotLightFlashRequest *);
    const float *flash_position; /* live globals51BE10/08/0C, read after flare */
} UCNpcGunShotServices;
/* Complete D9670 orchestration, preserving typed completing service order.
 * Bone lookup may leave its identity output unchanged exactly as source does.
 * External EBF10,216BD0,event52 and LightFlash owners must complete; failure
 * returns2 after any prior effects. Finite RN/gradual inputs, disjoint storage;
 * callbacks may mutate gun fields but must not destroy/reenter this owner. */
int uc_npc_gun_shoot(UCNpcGun *,const UCEntityFreshView *,const float direction[3],
    unsigned secondary,unsigned effects,unsigned bullet,float offset,float scale,unsigned hit,
    const UCNpcGunServices *,const UCNpcGunShotServices *);
/* Full D85D0 gun-matrix branch. found=0 for missing parent; other names are
 * handled by caller. Uses current parent pose rather than stale gun transform. */
int uc_npc_gun_matrix(const UCNpcGun *,const UCEntityFreshView *,const UCNpcGunServices *,float[16],unsigned *found);
/*20A3B0 spatial branch used by D8530. Localpoint comes actual21CAC0 named
 * model query; false/missing model falls back to entity translation. */
int uc_npc_gun_fire_position(const float world[16],const float local[3],unsigned found,float out[3]);
int uc_npc_gun_reload_sound(const UCNpcGun *,const UCNpcGunShotServices *);
#ifdef __cplusplus
}
#endif
#endif
