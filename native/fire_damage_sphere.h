#ifndef UC_FIRE_DAMAGE_SPHERE_H
#define UC_FIRE_DAMAGE_SPHERE_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCFireDamageSphere {float center[3],remaining,radius_squared;int32_t id,owner,session,unique;} UCFireDamageSphere;
typedef struct UCPlayerFireDamage {int32_t source,kind,zero;float direction[3];int32_t owner;float center[3];int32_t other,session,unique;uint8_t flag[2],padding[2];} UCPlayerFireDamage;
typedef struct UCFirePlayer {const float*position;void*actor;uint16_t group;} UCFirePlayer;
typedef struct UCFireDamageServices {
 void*owner;const uint32_t*session_mode;
 /* Source4ACC0 is reread after each player, including callbacks.4ACD0/lookup
  * projection returns1 live,0 missing,-1 failure; preserve actual registry IDs. */
 int32_t(*player_count)(void*);
 int(*player)(void*,uint32_t,UCFirePlayer*);
 int(*multiplayer)(void*); /* Actual Game virtual20, called only for inside target. */
 int(*blocked)(void*,const float from[3],const float to[3],void*actor,uint16_t group);
 int(*damage)(void*,void*actor,float amount,const UCPlayerFireDamage*); /* actualPlayer+6C */
 double(*delta)(void*);
 int(*remove)(void*);
} UCFireDamageServices;
/* Full1A6000/1A5C60 plus the actual offline quiet1A5EB0 branch. Online NPC list
 * event54 remains outside this fresh-session domain, never silently skipped.
 * Finite coordinates/delta, max4 players and bounded64 dynamic count reads;
 * source coincident-center NaN direction rejects explicitly if damage reached.
 * Damage and timer effects are committed in source order.0 complete,1 invalid,
 *2 service/effect failure. Damage always50; recipient owns actual damage rules. */
int uc_fire_damage_step(UCFireDamageSphere*,const UCFireDamageServices*);
/* Full236D0 box recipe and22430 center/boundingradius, before actual navigation
 * registration/group merging. This is not an invented Havok box or renderer. */
typedef struct UCNavigationBox {float points[4][3],center[3],radius,height;int32_t id;uint8_t flag8,allow_inside;} UCNavigationBox;
int uc_navigation_box(int32_t id,const float center[3],const float size[3],uint8_t allow_inside,UCNavigationBox*);
/* Whole221A0 merge predicate for the four-point236D0 domain.0/1 result,
 * -1 invalid. Height is deliberately irrelevant; source uses polygon Y. */
int uc_navigation_boxes_overlap(const UCNavigationBox*,const UCNavigationBox*);
#ifdef __cplusplus
}
#endif
#endif
