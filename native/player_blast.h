#ifndef UC_PLAYER_BLAST_H
#define UC_PLAYER_BLAST_H
#include "player_damage.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCPlayerConcussion {
 float strength,strength_step,attenuation,attenuation_step;
 uint32_t active;float seconds;
} UCPlayerConcussion;
typedef struct UCPlayerBlastServices {
 void *owner;const void *target;UCExplosionObstruction obstruction;
 int(*pain)(void*); /*69180 actual actor1C8 secondary channel*/
 int(*damage)(void*,float,const UCPlayerFireDamage*); /*actual virtual6C*/
 int(*sound)(void*,const char*); /*216BD0 Player bank*/
 int(*parameter)(void*,const char*event,const char*parameter,float); /*2160E0*/
 int(*bus_effect)(void*,const char*,float,float,float,float); /*world6C virtual5C*/
 double(*delta)(void*);
} UCPlayerBlastServices;
typedef struct UCPlayerConcussionServices {
 void*owner;float*shared_attenuation;float*feedback;
 int(*group_pitch)(void*,const char*,float); /*247A80 mode2, actual pitch ratio.*/
 double(*unscaled_delta)(void*); /*1F9020 world18*/
 double(*clock)(void*); /*1F9030, returned result discarded only at final completion*/
}UCPlayerConcussionServices;
/* WholeC4980 with the actual shared4E7D38 value and664 feedback-scalar owner. */
int uc_player_concussion_step(UCPlayerConcussion*,const UCPlayerConcussionServices*);
/* Complete C2150 in actual offline Mainplayer domain (2918A0 singletonNULL,
 * Gamevirtual20false). Retains the real six concussion fields; callbacks must
 * complete real damage/audio/pain owners. Invalid/coincident normalization
 * rejects explicitly. A callback failure may follow committed effects.
 *0 complete,1 invalid,2 committed failure. No death or fade consumer claim. */
int uc_player_blast_receive(UCPlayerConcussion*,const float position[3],int32_t sender,
 const UCExplosionBlast*,const void *network_owner,uint8_t multiplayer,
 const UCPlayerBlastServices*);
#ifdef __cplusplus
}
#endif
#endif
