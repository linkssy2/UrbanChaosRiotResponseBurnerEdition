#ifndef UC_NPC_THROWN_WEAPON_H
#define UC_NPC_THROWN_WEAPON_H
#include "entity_fresh.h"
#include "bullet_construct.h"
#ifdef __cplusplus
extern "C" {
#endif
/* DD770 suffix. The base is the existing real3F5F0/UCBulletState, not a
 * duplicate transform/entity. Source offsets name retained scalar fields. */
typedef struct UCNpcThrownWeapon {
    const char *asset1D0,*bank238;void *bank23C,*template244,*effect248,*physics250;
    int32_t weapon1EC,material214,variation240,group280,group284;
    uint32_t word208;
    uint8_t armed1F0,launched1F8,exploded200,hit201,settled202,hidden203,flag204;
    uint8_t dropped210,secondary218,flag219,import254,import255;
    float fuse1F4,delay1FC,spin20C,velocity21C[3],angular228[3],light_time24C;
    float vector258[3],vector264[3],previous270[3],elapsed27C;
} UCNpcThrownWeapon;
typedef union UCNpcThrownProperty {int32_t integer;float number;const char *string;} UCNpcThrownProperty;
typedef struct UCNpcThrownServices {
    void *owner;
    int (*base)(void *); /* complete3F5F0 */
    int (*listener_reset)(void *);
    int (*coroutine_initialize)(void *); /*1FB780, before properties */
    int (*property)(void *,unsigned,const char *,UCNpcThrownProperty *,unsigned *found);
    int (*bank)(void *,const char *,void **);
    int (*physics_construct)(void *); /* DBE10, sees already published suffix */
    int (*coroutine_schedule)(void *); /*1FB9D0, after physics creation */
    int (*damage)(void *,float); /*218130 */
    int (*listener_register)(void *,int32_t);
    int (*listener_bind)(void *,unsigned); /*217/DC240,221/DC2A0,218/D8520 */
    int (*find_template)(void *,const char *,void **); /* names FIRST element, resolve */
    int (*name_add)(void *,const char *,int32_t);
    int (*physics_ignite)(void *,void *,float); /*28CBC0 then bodyD4=1 */
    int (*physics_source_owned)(void *,void *,unsigned *); /*28FF80/body8: source authority, not hand attachment */
    int (*physics_launch)(void *,void *,const float[3],float); /*28CB80,28CBF0,D4=1 */
    int (*dropped_template)(void *); /* DB230 actual descriptor/template factory */
    int (*set_flags)(void *,uint32_t); /*20A140 */
} UCNpcThrownServices;
int uc_npc_thrown_construct(UCNpcThrownWeapon *,UCBulletState *,const UCEntityFreshView *,const UCNpcThrownServices *);
float uc_npc_thrown_fuse(const UCNpcThrownWeapon *);
uint8_t uc_npc_thrown_settled(const UCNpcThrownWeapon *); /*DA730 */
uint8_t uc_npc_thrown_exploded(const UCNpcThrownWeapon *); /*DA740 */
void uc_npc_thrown_set_fuse(UCNpcThrownWeapon *,float);
int uc_npc_thrown_ignite(UCNpcThrownWeapon *,float,const UCNpcThrownServices *);
int uc_npc_thrown_launch(UCNpcThrownWeapon *,const float[3],float,float,const UCNpcThrownServices *);
int uc_npc_thrown_drop(UCNpcThrownWeapon *,const UCNpcThrownServices *);
#ifdef __cplusplus
}
#endif
#endif
