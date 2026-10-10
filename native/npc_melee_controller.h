#ifndef UC_NPC_MELEE_CONTROLLER_H
#define UC_NPC_MELEE_CONTROLLER_H
#include <stdint.h>
#include "fire_damage_sphere.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCNpcMeleeController {
    int32_t weapon,type;uint8_t detached,hidden;
    int32_t selected,last_target,actor;void *body,*bank,*events;
    int32_t profile,target,count;uint8_t watching,start,end,active,enabled,option;
} UCNpcMeleeController;
typedef struct UCNpcMeleeServices {
    void *owner;
    int (*resolve)(void *,int32_t); /*1 live,0 absent,-1 failure*/
    int (*weapon_type)(void *,int32_t,int32_t *); /*missingproperty retains0*/
    int (*retire)(void *,int32_t); /*20A170: delete by ID, not a flags mask.*/
    int (*watch)(void *,uint8_t *,const char *,int32_t);
    int (*unwatch)(void *,uint8_t *);
    int (*eligible)(void *,int32_t,uint8_t *); /*2BDC0*/
    int (*strike)(void *); /*2BF60*/
} UCNpcMeleeServices;
/*2A100 leaves start/end/count/option unwritten; successful watch and target
 * setup supply them before any read. Callers retain their initialization state.*/
void uc_npc_melee_construct(UCNpcMeleeController *,int32_t,void*,void*,void*);
int uc_npc_melee_bind(UCNpcMeleeController *,int32_t,const UCNpcMeleeServices *);
int uc_npc_melee_close(UCNpcMeleeController *,const UCNpcMeleeServices *);
int uc_npc_melee_update(UCNpcMeleeController *,const UCNpcMeleeServices *);
void uc_npc_melee_target(UCNpcMeleeController *,int32_t,int32_t,uint8_t);
void uc_npc_melee_cancel(UCNpcMeleeController *);
double uc_npc_melee_range(const UCNpcMeleeController *);
double uc_npc_melee_facing(const UCNpcMeleeController *);
/*2BDC0 geometry after live target/actor lookup;2BF60 exact damage packet.
 * Runtime still owns registry lookup, body forward, target property, virtual6C
 * damage delivery and the weapon's Impact sound, in that order. */
int uc_npc_melee_contact(int32_t profile,const float actor[3],const float target[3],const float forward[3],uint8_t *);
int uc_npc_melee_hit(const UCNpcMeleeController *,const float actor[3],const float target[3],float *amount,UCPlayerFireDamage *);
#ifdef __cplusplus
}
#endif
#endif
