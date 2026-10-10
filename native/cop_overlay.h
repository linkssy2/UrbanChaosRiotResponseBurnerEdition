#ifndef UC_COP_OVERLAY_H
#define UC_COP_OVERLAY_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/*1BB8B0 scalar state. Actual322D0 entity/child/listener/callback ownership is
 * external. This admitted update is the complete target-free1BA920 path;
 * target acquisition must run, and a real acquired target returns2 instead of
 * being discarded. Services perform actual stance/aim operations. */
typedef struct UCCopOverlay {
    uint8_t allow_relax,can_shoot,can_see,compliance,target_changed,special,aim_locked,event_pending;
    int32_t last_target;
    float compliance_time,unseen_time,aim_delay,shot_delay,suppress_time;
    int32_t target_weapon;
} UCCopOverlay;
typedef struct UCCopOverlayServices {
    void *owner;
    int32_t (*player_target)(void *);
    int32_t (*target)(void *);
    int (*acquire)(void *,uint8_t prefer,uint8_t check_player);
    int (*aim_stop)(void *);
    int (*stance_busy)(void *,uint8_t *);
    int (*weapon_mode)(void *,int32_t *);
    int (*stance)(void *,int32_t,uint8_t,void *completion);
} UCCopOverlayServices;
void uc_cop_overlay_init(UCCopOverlay *,uint8_t allow_relax);
/*1BA630 and1BA650 subscribed event effects; ignores no unknown event. */
int uc_cop_overlay_event(UCCopOverlay *,uint32_t event);
/*No secondarychild/task callback exists in the admitted state. Returns1 for
 * invalid input,2 for unsupported combat state or service failure. Effects
 * preceding failure remain committed; never retry or report success. Actor
 * world copy32680 is the enclosing owner's last operation after success. */
int uc_cop_overlay_quiet_update(UCCopOverlay *,float delta,uint8_t eligible,
    const UCCopOverlayServices *);
/* Complete1BA920 policy. Typed services retain the actual external owners;
 * callbacks either finish the requested action or report failure. The scalar
 * query callbacks may throw through the C++ caller when a live owner is absent.
 * Unlike quiet_update, target acquisition is never an unsupported state. */
typedef struct UCCopOverlayCombatServices {
    UCCopOverlayServices common;
    uint8_t compliance_allowed;
    int32_t actor,child,root;
    int32_t (*player)(void *);
    int (*position)(void *,int32_t,float[3]); /*1 found,0 missing,-1 failure*/
    int32_t (*integer)(void *,int32_t,const char *,int32_t fallback);
    uint8_t (*boolean)(void *,int32_t,const char *,uint8_t fallback);
    double (*number)(void *,int32_t,const char *,float fallback);
    int (*point)(void *,int32_t,const char *,float[3]);
    int (*can_shoot)(void *,int32_t); /*Gun+24(target,0,0)*/
    int (*visible)(void *,int32_t); /*E05C0(target,-1,0,zero,0)*/
    double (*facing)(void *); /*1BA670, preserves x87 return*/
    int (*task_status)(void *); /*-1 no110 task,0 pending,1/2 completed*/
    int (*gun_status)(void *,unsigned offset); /*+34,+38,+3C*/
    int (*reload)(void *); /*stance+4(NULL,1), Gun+44(0,1)*/
    int (*target_contact)(void *,int32_t); /*11B030 event4F9DA0*/
    int (*melee_response)(void *); /*1BA7A0:1 transitions,0 stays*/
    int (*aim)(void *,const float[3]); /*2AC250(point,NULL,0)*/
    int (*player_in_line)(void *); /*1BA400*/
    int (*fire)(void *,int32_t,float); /*stance+8(task,NULL),Gun+28(target,0,1,scale)*/
    int (*say)(void *,int32_t,int32_t);
    double (*random)(void *,float,float);
    uint8_t (*coin)(void *);
    int (*clear_target)(void *); /*32DE0*/
    int (*base)(void *,uint8_t); /*32680, omitted after melee transition*/
} UCCopOverlayCombatServices;
int uc_cop_overlay_update(UCCopOverlay *,float delta,uint8_t expensive,
    const UCCopOverlayCombatServices *);
#ifdef __cplusplus
}
#endif
#endif
