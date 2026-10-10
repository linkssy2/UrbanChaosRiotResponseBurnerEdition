#ifndef UC_MEDIC_ROOT_POLICY_H
#define UC_MEDIC_ROOT_POLICY_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* The original Medic suffix; shared root/actor storage stays with its owner.
 * Members map to11C,120,128,130/134,139/13A,140..154,158,15C. */
typedef struct UCMedicRootPolicy {
 uintptr_t support;
 int32_t secondary,initial;
 float dialogue_stamp,damage_stamp;
 uint8_t put_down,pick_up,alternate;
 const char*cues[6];const char*bank;
} UCMedicRootPolicy;
typedef struct UCMedicRootServices {
 void*owner;
 uintptr_t*actor_support;const int32_t*character_type;const int32_t*order;
 const float*delta; /*actual world+14;169670 reads it again after dialogue*/
 int(*base_enter)(void*,void*controller); /*36700*/
 int(*bank)(void*,unsigned,const char*); /*21A030(mode1)*/
 int(*body)(void*,unsigned,const char*,uint8_t,uint8_t); /*DE380*/
 int(*initial)(void*,int32_t,void*controller); /*2084E0->210CF0->32470*/
 int(*support_create)(void*,uintptr_t*); /*2A100 with actual actor/body/events/bank*/
 int(*support_attach)(void*,uintptr_t); /*2B180 then support virtual24*/
 int(*set_health)(void*,float); /*actor virtual64*/
 int(*integer)(void*,const char*,int32_t,int32_t*); /*20A6D0*/
 int(*tag)(void*,const char*,uint8_t add); /*same live actor, UCEntityNames*/
 int(*secondary_create)(void*,const char*,int32_t*); /*366E0,empty159C0(0,-1)*/
 int(*start)(void*,int32_t,void*controller); /*32470*/
 int(*watch)(void*,uint8_t*,const char*); /*actor124->1FAAE0(flag,name,rootID)*/
 int(*stop)(void*,int32_t); /*324A0, including sentinel/missing IDs*/
 int(*clear_children)(void*); /*39240*/
 int(*flags)(void*,uint32_t); /*root20A170*/
 int(*release_support)(void*,uintptr_t,uint8_t); /*virtual20(1)*/
 int(*unwatch)(void*,uint8_t*); /*1F9750*/
 int(*base_exit)(void*); /*36720*/
 int(*dialogue)(void*,const char*,int32_t); /*21AAE0*/
 int(*damage)(void*,uint32_t,float); /*whole371B0*/
 int(*authored_secondary)(void*,int32_t source,void*controller,int32_t*id); /*2084E0->210CF0->32470*/
} UCMedicRootServices;
/* Whole169700/1695C0/169670.0complete,1invalid,2requiredservice failure.
 * Services operate actual owners; failure may retain earlier source effects.
 * Exit preserves source's released support field; actor_support becomes0. */
int uc_medic_root_enter(UCMedicRootPolicy*,void*controller,const UCMedicRootServices*);
int uc_medic_root_exit(UCMedicRootPolicy*,const UCMedicRootServices*);
int uc_medic_root_damage(UCMedicRootPolicy*,uint32_t,float,const UCMedicRootServices*);
/* Plain Medic170E80/170BC0: authored speech plus CheckHealPlayer, without
 * team orders, SELECTABLEITEMS registration or169670 damage policy. */
int uc_medic_plain_enter(UCMedicRootPolicy*,int32_t speech_source,int32_t *speech,
 void*controller,const UCMedicRootServices*);
int uc_medic_plain_exit(UCMedicRootPolicy*,int32_t speech,const UCMedicRootServices*);
#ifdef __cplusplus
}
#endif
#endif
