#ifndef UC_NPC_GUN_CONTROLLER_H
#define UC_NPC_GUN_CONTROLLER_H
#include "entity_fresh.h"
#include "game_random.h"
#ifdef __cplusplus
extern "C" {
#endif
/*292F0/2EE50 retained controller, independent of concrete Gun ownership.
 * Constructor preserves source-unwritten fields; zero initialization is not
 * used as a substitute for later source writes. */
typedef struct UCNpcGunController {
    int32_t weapon,type;uint8_t detached,hidden;
    int32_t selected,last_target,actor;void *body;
    int32_t profile,target;uint8_t reloading,shooting,force_miss,force_hit;
    int32_t ammo;float cooldown,unused34;uint8_t position_mode,head;
    float accuracy,scale,elapsed,delay,interval;
    uint8_t enabled,burst_finished,held;
    float idle,offset[3];int32_t obstruction,capacity_override;float range_scale;
} UCNpcGunController;
typedef struct UCNpcGunControllerServices {
    void *owner;UCGameRandom *random;
    int (*difficulty)(void *,double *); /* actual167C0;16960 scales by1.25 */
    int (*resolve)(void *,int32_t,const UCEntityFreshView **);
    int (*boolean)(void *,int32_t,const char *,unsigned *);
    int (*vector)(void *,const UCEntityFreshView *,const char *,float[3],unsigned *);
    int (*weapon_type)(void *,int32_t,int32_t *); /* real property, not controller cache */
    int (*matrix)(void *,int32_t,float[16]); /* required gun matrix property */
    int (*fire_position)(void *,int32_t,float[3]); /* D8530 */
    int (*query)(void *,const float[3],const float[3],uint16_t,const UCEntityFreshView **,unsigned *);
    int (*segment_clear)(void *,const float[3],const float[3],unsigned *);
    int (*shot)(void *,int32_t,const float[3],float,unsigned); /*D9670(dir,0,1,1,0,scale,hit) */
    int (*reload_sound)(void *,int32_t); /*D8580 */
    int (*emote)(void *,int32_t,unsigned,unsigned); /*DED50(actor,code,priority) */
    int (*remove_weapon)(void *,int32_t); /* actual20A170 */
    double (*delta)(void *); /* source1F9010, separately read at each call */
} UCNpcGunControllerServices;
/*0 success,1 invalid before effects,2 committed service/admission failure.
 * Required services are completing real owners. Borrowed state/registry/RNG
 * remain live and disjoint; finite RN/gradual inputs, nondegenerate aim vectors.
 * Source bool results are separate from status. No concrete gun effects claim. */
int uc_npc_gun_controller_construct(UCNpcGunController *,int32_t actor,void *body,
    int32_t capacity,float range_scale,const UCNpcGunControllerServices *);
int uc_npc_gun_controller_bind(UCNpcGunController *,int32_t weapon,const UCNpcGunControllerServices *);
int uc_npc_gun_controller_update(UCNpcGunController *,const UCNpcGunControllerServices *);
int uc_npc_gun_controller_eligible(UCNpcGunController *,int32_t target,uint32_t flags,
    float vertical_offset,const UCNpcGunControllerServices *,unsigned *);
int uc_npc_gun_controller_fire(UCNpcGunController *,int32_t target,uint32_t flags,
    float accuracy,float scale,const UCNpcGunControllerServices *,unsigned *);
int uc_npc_gun_controller_reload(UCNpcGunController *,unsigned speak,unsigned alternate,
    const UCNpcGunControllerServices *);
int uc_npc_gun_controller_status(const UCNpcGunController *,unsigned virtual_offset,int32_t *);
#ifdef __cplusplus
}
#endif
#endif
