#ifndef UC_NPC_FLIGHT_H
#define UC_NPC_FLIGHT_H
#include "npc_thrown_weapon.h"
#include "bullet_step.h"
#include "bullet_history.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCNpcFlightServices {
 void *owner;
 const float *delta,*time;
 /* Real3F6E0 query/impact, returning correction before21D750 finish. */
 int (*displacement)(void*,const UCEntityFreshView*,UCBulletState*,float[3]);
 int (*tint)(void*,float[4],unsigned write);
 int (*camera)(void*,float[16]); /*actual214F90 published viewport*/
 int (*light)(void*,void*,const float[16],float attenuation);
 int (*multiplayer)(void*,unsigned*); /*actualsession virtual20*/
 int (*intercept)(void*,int32_t,float,unsigned,float[3],unsigned*); /*2C720*/
 double (*random)(void*,float,float); /*shared2D3780*/
 /* Actualcleaver template/descriptor mode5 factory, addchild andflags1.
  * All state/world/bank/variation inputs remain in the same borrowed owners. */
 int (*deflect)(void*,const UCEntityFreshView*,UCNpcThrownWeapon*,const float[3],const float[3]);
} UCNpcFlightServices;
/* DC330 suffix fromDC6E6 throughDCCB4. Outer owner must already complete fuse
 * expiry and parent attachment; offlineDBFC0 follows separately. Finite RN,
 * real completing services, no reentry; return0 done,1 invalid,2 committed
 * service failure. Actual collision and event effects cannot be replayed. */
int uc_npc_flight_late(UCNpcThrownWeapon*,UCBulletState*,const UCEntityFreshView*,const UCNpcFlightServices*);
/* Actual2C720/2C290 with caller-resolved same-registry entity position and real
 * local-player id. Missing position returns0. Strict radius test, history order,
 * source self/owner exclusion, optional direction publication. Return0 nohit,
 * 1 hit,-1 invalid; no fabricated histories. */
int uc_npc_flight_intercept(const UCBulletHistory*,int32_t,const float position[3],
 const uint32_t *player_id,float radius,unsigned player_only,float direction[3]);
#ifdef __cplusplus
}
#endif
#endif
