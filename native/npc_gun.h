#ifndef UC_NPC_GUN_H
#define UC_NPC_GUN_H
#include "entity_fresh.h"
#include "game_random.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct UCNpcGun {
    const char *assetC8,*bankD0;
    void *bankD4;
    int32_t flareCC,weaponD8,teamE0,owner11C,tags[3];
    float damageDC,elapsedE8,heatEC,deadline110,unattached114,old_y118;
    uint32_t removeF0,droppedF4,wordF8,wordFC;
    uint8_t handE4;
} UCNpcGun;
typedef union UCNpcGunProperty {int32_t integer;float number;const char *string;} UCNpcGunProperty;
typedef struct UCNpcGunServices {
    void *owner;
    int (*health_base)(void *); /* actual218550, before all suffix effects */
    int (*listener_reset)(void *); /* actual empty B8/BC/C0 and NULL context */
    int (*property)(void *,unsigned type,const char *name,UCNpcGunProperty *,unsigned *found);
    int (*bank)(void *,const char *,void **);
    int (*resolve)(void *,int32_t,const UCEntityFreshView **);
    int (*asset)(void *,const char **);
    int (*model_load)(void *,const char *);
    int (*flare)(void *,int32_t weapon,int32_t *id); /* First Person=false, mode1 child GunFlare */
    int (*tag)(void *,const UCEntityFreshView *,const char *,int32_t *);
    int (*listener_register)(void *,int32_t); /* publish gun context then1F9120 */
    int (*listener_bind)(void *,unsigned event); /*0:4FA028/D8900,1:4FA034/D8520; third word0 */
    int (*priority)(void *,int32_t,unsigned);
    int (*delta)(void *,float *);
    int (*model_heat)(void *,void *,float); /*21CB90 */
    int (*health_update)(void *); /* full218680, including real aux clear */
    int (*pose_tag)(void *,const UCEntityFreshView *,int32_t,float[16],unsigned *found);
    int (*set_world)(void *,const float[16]); /* actual gun8A050 */
    int (*time)(void *,float *); /* source global world+8 */
    int32_t (*player_count)(void *);
    int32_t (*player_id)(void *); /*4ACD0(0), queried only when count is1 */
    int (*remove)(void *); /*D8B6E/D8B8A actual virtual24 */
} UCNpcGunServices;
/* DA120 complete suffix and ordered external ownership frontiers. The view
 * borrows the actual entity, with a real same-registry resolver. Required
 * callbacks must perform their named operation, return0, preserve all borrowed
 * storage/FP mode, and retain resolved views through the transaction. No stub
 * model, bank, child, listener or priority completion is admitted. No rollback:
 * 0 completed,1 invalid before effects,2 committed service/admission failure.
 * health_base owns registration/placement/aux; gun suffix is not218A60 actor.
 * Guest vtable/listener raw pointer layout is replaced by typed native owners. */
int uc_npc_gun_construct(UCNpcGun *,const UCEntityFreshView *,const UCNpcGunServices *);
/* D9C20: two separately observed delta calls, EC clamp[.05,1], optional model
 * heat and real218680 tail. Finite values, delta[0,1], nearest/gradual FP. */
int uc_npc_gun_update(UCNpcGun *,const UCEntityFreshView *,const UCNpcGunServices *);
/* Full D8A70 admitted F0=F4=0, including the missing-parent removal path.
 * Posed parent tag must exist. Flare tag absence copies gunworld; a missing
 * flare is source success. Actual flare world is written directly. Source
 * pairwise SSE composition, Y/Z axis swap, oldY/deadline ordering retained.
 * No dropped physics or fading claim. */
int uc_npc_gun_attached(UCNpcGun *,const UCEntityFreshView *,const UCNpcGunServices *);
/*27D40 descriptor data after real owner/model lookup. Caller obtains actual
 * gundummy and actor virtual28 matrix first. Uses the shared source RNG once,
 * range[0,3), and swaps dummy ROW0/ROW2 before multiplying by actor world. */
typedef struct UCNpcGunDescriptor {
    float world[16],damage;
    int32_t weapon,team,owner;
    uint8_t hand;
    const char *asset,*bank;
} UCNpcGunDescriptor;
int uc_npc_gun_descriptor(const float dummy[16],const float world[16],int32_t owner,
    UCGameRandom *,UCNpcGunDescriptor *);
#ifdef __cplusplus
}
#endif
#endif
