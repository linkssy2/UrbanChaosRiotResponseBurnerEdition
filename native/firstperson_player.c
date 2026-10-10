#include "firstperson_player.h"
#include <string.h>

/* Adapt the existing qualified lifecycle subset without duplicating its math. */
static void transfer_motion(UCFirstpersonPlayerMotion *s,UCFirstpersonMotion *m,int publish) {
#define FIELD(a,b) do { if(publish)memcpy(&(a),&(b),sizeof(a));else memcpy(&(b),&(a),sizeof(a)); } while(0)
    FIELD(s->weapon.angular,m->weapon_angle);
    FIELD(s->view.angle,m->view_angle);
    FIELD(s->input.recoil,m->recoil);
    FIELD(s->bob.speed,m->movement);
    FIELD(s->view.translation,m->view_translation);
    FIELD(s->weapon.translation,m->weapon_translation);
#undef FIELD
}
UCMotionStatus uc_firstperson_player_construct(UCFirstpersonPlayerMotion *s,const float clocks[4]) {
    UCFirstpersonMotion motion;UCFirstpersonPlayerMotion next;UCMotionStatus status;
    if(!s||!clocks)return UC_MOTION_ARGUMENT;
    next=*s;transfer_motion(&next,&motion,0);
    status=uc_motion_construct(&motion,clocks);if(status)return status;
    transfer_motion(&next,&motion,1);
    for(unsigned i=0;i<2;++i)uc_motion_smoother_construct(&next.input.movement[i]);
    next.input.angular_input=0;
    uc_firstperson_bob_construct(&next.bob);
    memset(next.weapon.phase,0,sizeof(next.weapon.phase));
    (void)uc_motion_smoother_reset(&next.weapon.lowering,0,0,5);
    (void)uc_motion_smoother_reset(&next.pitch_limit,1.256637096405029296875f,1.256637096405029296875f,5);
    *s=next;return UC_MOTION_OK;
}
void uc_firstperson_player_activate(UCFirstpersonPlayerMotion *s) {
    UCFirstpersonMotion motion;if(!s)return;
    transfer_motion(s,&motion,0);uc_motion_activate(&motion);transfer_motion(s,&motion,1);
    uc_firstperson_bob_activate(&s->bob);
    for(unsigned i=0;i<2;++i)(void)uc_motion_smoother_reset(&s->input.movement[i],0,0,6);
}
void uc_firstperson_player_reset_movement(UCFirstpersonPlayerMotion *s) {
    UCFirstpersonMotion motion;if(!s)return;
    transfer_motion(s,&motion,0);uc_motion_reset_movement(&motion);transfer_motion(s,&motion,1);
    uc_firstperson_bob_reset_movement(&s->bob);
    for(unsigned i=0;i<2;++i){s->input.movement[i].current=0;s->input.movement[i].target=0;}
}
