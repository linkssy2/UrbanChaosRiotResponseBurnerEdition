#ifndef UC_CHARACTER_DAMAGE_H
#define UC_CHARACTER_DAMAGE_H
#include "explosion_sequence.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCCharacterInjury {uint32_t reactions;float damage;int32_t kind,owner;} UCCharacterInjury;
/*36800: merge queued flags, retaining only the first authored reaction class;
 * other flags and queued damage are preserved. No queue mutation.*/
int uc_character_injury_read(const UCCharacterInjury*,uint32_t *flags,float *damage);
/*DE590: front/back/side reaction bit from the actual actor forward and hit
 * direction. Zero hit returns0x100; nonzero hit requires nonzero horizontal
 * actor forward. Finite double-backed x87/SSE arithmetic, no guessed heading.*/
int uc_character_hit_direction(const float world[16],const float direction[3],uint8_t force_front_back,uint32_t *flags);
typedef struct UCCharacterInjuryServices {
 void *owner;
 int(*root_id)(void*,int32_t*);
 int(*enemy)(void*,int32_t,uint8_t*); /*1F7AA0(root,"enemy of player",1)*/
 int(*shield)(void*,const float point[3],uint8_t*); /*completeDF9D0*/
 UCCharacterInjury*(*resolve)(void*,int32_t); /*207D70, retained root state*/
} UCCharacterInjuryServices;
/*DFBD0 admission and DDCA0 publication.0 complete;1 invalid;2 committed
 * service failure. accepted is the original Boolean result. No health mutation
 * is invented: root38A70 consumes the queued reaction separately.*/
int uc_character_injure(const UCCharacterInjuryServices*,uint32_t flags,float damage,
 const float point[3],int32_t kind,int32_t owner,uint8_t check_shield,uint8_t *accepted);
/* DF380 event3F admission and ordered foreground replacement. The callbacks
 * own the real RestrainedBehaviour factory and32CB0 controller handoff. */
typedef struct UCCharacterRestraintServices {
 void *owner;
 int (*create)(void *,uint8_t on_floor,int32_t *id);
 int (*activate)(void *,int32_t id);
} UCCharacterRestraintServices;
int uc_character_restrain(uint32_t *status,int32_t *cause,uint8_t on_floor,uint8_t by_player,const UCCharacterRestraintServices *);

typedef struct UCCharacterBlastResponse {float direction[3],impulse[28];} UCCharacterBlastResponse;
typedef struct UCCharacterBlastServices {
 void *owner;const float *position;int32_t id;uint8_t controller;
 int(*eligible)(void*,const UCExplosionBlast*,uint8_t*); /*E9610(80,140)*/
 int(*player)(void*,int32_t*);
 int(*root)(void*,int32_t*,uint32_t*,uint8_t*); /*1F8FC0(controller1C)*/
 int(*order)(void*,int32_t*); /*1F7AF0(root,"order id",-1)*/
 int(*property)(void*,const char*,uint8_t*); /*20A690(actor,name,0)*/
 int(*notify)(void*,int32_t actor,uint8_t zero,uint8_t one); /*event3F/DF380*/
 int(*injure)(void*,uint32_t,float,const float*,int32_t,int32_t,uint8_t);
 int(*root_blast)(void*,const UCExplosionBlast*,int32_t sender); /*36BA0*/
} UCCharacterBlastServices;
/*DF110/2A3400 leaves source-unwritten padding intact.*/
void uc_character_blast_initialize(UCCharacterBlastResponse*);
/*WholeE1570, including repeated eligibility and final root delivery. Body
 * coordinates are render cm. Response retains original physics XZY metres.
 * Coincident horizontal direction stays zero instead of publishing 0/0;
 * damage and the source torso force still proceed (native safety guard).
 * Typed callbacks complete original services; no no-op injury/death policy.*/
int uc_character_blast(UCCharacterBlastResponse*,const UCExplosionBlast*,int32_t sender,
 const UCCharacterBlastServices*);
/* 36450 and the non-avoidance branch of36F80. Caller retains original
 * enemy/damage-enabled/scripted-admission predicates and live sender lookup.
 * This state is separate from the threat controller's recent-shot timestamp. */
typedef struct UCCharacterBlastNotice {
 float point[3],radius;int32_t sender;float remaining,ratio;
} UCCharacterBlastNotice;
int uc_character_blast_notice(UCCharacterBlastNotice *,const float actor[3],const float point[3],float radius,int32_t sender,float duration);
int uc_character_blast_notice_step(UCCharacterBlastNotice *,const float actor[3],const float *sender_position,double delta);

typedef struct UCCharacterBullet {
 int32_t item,kind,owner;float position[3];
} UCCharacterBullet;
/* Actual collision packet fields consumed by DFC60. This is not the generic
 * 56-byte damage message, nor a scenery packet with invented leading words. */
typedef struct UCCharacterBulletImpact {
 int32_t source;uint32_t material;int32_t kind;
 float point[3],momentum[3],distance,parameters[9];
} UCCharacterBulletImpact;
typedef struct UCCharacterBloodHit {float point[3],normal[3];uint16_t material;uint8_t actor;} UCCharacterBloodHit;
typedef struct UCCharacterBulletServices {
 void*owner;const float*world;int32_t id,root_id;uint16_t group;uint8_t faction,controller;float damage;
 int(*heavy_weapon)(void*,int32_t,uint8_t*); /*431A0*/
 int(*boolean)(void*,int32_t,const char*,uint8_t,uint8_t*);
 int(*resolve)(void*,int32_t,uint8_t*); /*1F8FC0*/
 int(*player_index)(void*,int32_t,int32_t*); /*entity58->4A840->4A6E0*/
 int(*source_owner)(void*,int32_t,int32_t*,uint8_t*); /*virtual8("owner")*/
 int(*friendly)(void*,int32_t,uint8_t*); /*39900*/
 int(*target)(void*,int32_t); /*32E30*/
 int(*effect)(void*,int32_t,const char*,const char*); /*208C50/219740*/
 int(*health)(void*,float,int32_t); /*actor150=kind,virtual64(value)*/
 int(*profile)(void*,uint32_t,uint8_t*); /*643E0 bytes185/32*/
 int(*burn)(void*,int32_t*); /*DF690 exact {0.9,1,0,3,0,1,1}, owner in/out*/
 int(*injure)(void*,uint32_t,float,const float*,int32_t,int32_t,uint8_t,uint8_t*);
 int(*ray)(void*,const float*,const float*,uint32_t,uint16_t,uint32_t,float,UCCharacterBloodHit*,uint8_t*);
 int(*random)(void*,int32_t,int32_t,int32_t*);
 int(*decal)(void*,const char*,const UCCharacterBloodHit*,float,uint32_t,uint32_t);
} UCCharacterBulletServices;
/* Whole DFC60, including source ownership, actual body-part reaction, impulse,
 * optional profile fire and blood projection. All effect services are required
 * when reached.0 complete;1 invalid;2 committed service failure. */
int uc_character_bullet(UCCharacterBlastResponse*,const UCCharacterBullet*,const UCCharacterBulletImpact*,const UCCharacterBulletServices*);
#ifdef __cplusplus
}
#endif
#endif
