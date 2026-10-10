#include "cop_overlay.h"
#include <math.h>
#include <string.h>
void uc_cop_overlay_init(UCCopOverlay*s,uint8_t relax){
    if(!s)return;memset(s,0,sizeof(*s));s->allow_relax=relax!=0;s->last_target=-1;s->aim_delay=-100;
}
int uc_cop_overlay_event(UCCopOverlay*s,uint32_t event){
    if(!s)return 1;
    if(event==0x4c){s->suppress_time=5;return 0;}
    if(event==0x49){s->suppress_time=.2f;s->event_pending=1;return 0;}
    return 1;
}
int uc_cop_overlay_quiet_update(UCCopOverlay*s,float delta,uint8_t eligible,const UCCopOverlayServices*h){
    if(!s||!h||!h->player_target||!h->target||!h->acquire||!h->aim_stop||!h->stance_busy||!h->weapon_mode||!h->stance||
       !isfinite(delta)||delta<0||delta>1||!isfinite(s->unseen_time)||!isfinite(s->compliance_time)||!isfinite(s->shot_delay)||!isfinite(s->suppress_time))return 1;
    int32_t player=h->player_target(h->owner);
    if(eligible){uint8_t prefer=0;
        if(player!=-1&&h->target(h->owner)==player)prefer=1;
        else if(!s->can_see&&s->unseen_time>2)prefer=1;
        if(h->acquire(h->owner,prefer,1))return 2;
    }
    if(h->target(h->owner)!=-1||s->compliance||s->target_changed||s->special)return 2;
    s->can_shoot=0;s->can_see=0;s->last_target=h->target(h->owner);
    s->compliance_time=(float)((double)s->compliance_time-delta);
    if(h->target(h->owner)!=-1)return 2; //1BA670's actual target query.
    /*No visible target and no child PreserveAim property: this is the source
     *1BAE97 path. Crossing2 seconds cancels aim2AC2B0 before unseen publication. */
    if(s->unseen_time<2&&(double)s->unseen_time+delta>=2&&h->aim_stop(h->owner))return 2;
    s->unseen_time=(float)((double)s->unseen_time+delta);
    uint8_t busy=0;if(h->stance_busy(h->owner,&busy))return 2;
    if(!busy){int32_t mode=0;
        if(!(s->unseen_time>15&&s->allow_relax)){int32_t weapon;
            if(h->target(h->owner)!=-1)return 2;
            if(h->weapon_mode(h->owner,&weapon))return 2;mode=weapon==2?0:4;}
        if(h->stance(h->owner,mode,1,0))return 2;
    }
    s->aim_locked=0; //Absent child float property uses default0, below5.
    if(s->suppress_time<=0&&h->target(h->owner)!=-1)return 2;
    if(s->suppress_time>0)s->suppress_time=(float)((double)s->suppress_time-delta);
    s->shot_delay=(float)((double)s->shot_delay-delta);return 0;
}

int uc_cop_overlay_update(UCCopOverlay*s,float dt,uint8_t expensive,const UCCopOverlayCombatServices*h){
    if(!s||!h||!isfinite(dt)||dt<0||dt>1)return 1;
    const UCCopOverlayServices*c=&h->common;void*o=c->owner;
    if(!c->player_target||!c->target||!c->acquire||!c->aim_stop||!c->stance_busy||!c->weapon_mode||!c->stance||
       !h->player||!h->position||!h->integer||!h->boolean||!h->number||!h->point||!h->can_shoot||!h->visible||!h->facing||
       !h->task_status||!h->gun_status||!h->reload||!h->target_contact||!h->melee_response||!h->aim||!h->player_in_line||
       !h->fire||!h->say||!h->random||!h->coin||!h->clear_target||!h->base)return 1;
#define DO(x) do{if((x))return 2;}while(0)
#define BOOL_TO(v,x) do{int r_=(x);if(r_<0)return 2;(v)=(uint8_t)(r_!=0);}while(0)
    int32_t pt=c->player_target(o);
    if(expensive){uint8_t prefer=0;
        if(pt!=-1&&c->target(o)==pt)prefer=1;
        else if(!s->can_see&&s->unseen_time>2)prefer=1;
        DO(c->acquire(o,prefer,1));}
    if(c->target(o)!=-1){
        BOOL_TO(s->can_shoot,h->can_shoot(o,c->target(o)));
        if(s->can_shoot)s->can_see=1;
        else BOOL_TO(s->can_see,h->visible(o,c->target(o)));
        if(c->target(o)!=s->last_target){
            s->target_weapon=h->integer(o,c->target(o),"weapon kind",-1);s->target_changed=1;
            if(!s->can_see&&!s->can_shoot)s->unseen_time=4;
            s->special=h->boolean(o,c->target(o),"cop dont shoot",0);
            if(s->special)s->compliance=1;s->aim_delay=-100;
        }
    }else{s->can_shoot=0;s->can_see=0;}
    if(s->can_see)s->unseen_time=0;
    s->last_target=c->target(o);
    s->compliance_time=(float)((double)s->compliance_time-dt);
    if((s->target_changed&&s->can_see&&s->can_shoot)||(s->compliance&&s->compliance_time<=0)){
        if((h->compliance_allowed&&h->integer(o,c->target(o),"last weapon target",0)==-1)||s->special){
            DO(h->say(o,43,50));s->compliance=1;s->compliance_time=(float)h->random(o,5,10);}
        s->target_changed=0;
    }
    if(s->compliance&&!s->special){
        if(h->integer(o,c->target(o),"last weapon target",-1)!=-1)s->compliance=0;
        if(s->target_weapon==2){float target[3],self[3];int found=h->position(o,c->target(o),target);if(found<0)return 2;
            if(found){if(h->position(o,h->actor,self)!=1)return 2;
                float x=target[0]-self[0],y=target[1]-self[1],z=target[2]-self[2];
                if(90000.f>((z*z+y*y)+x*x))s->compliance=0;}}
    }
    uint8_t suppressed=s->suppress_time>0;
    if(suppressed&&s->event_pending&&c->target(o)!=-1){
        int32_t player=h->player(o);
        if(player!=-1){float pc[3],target[3],self[3];int p=h->position(o,player,pc),t=h->position(o,c->target(o),target);
            if(p<0||t<0)return 2;
            if(p&&t){if(h->position(o,h->actor,self)!=1)return 2;float a[3],b[3];
                for(unsigned i=0;i<3;i++){a[i]=pc[i]-self[i];b[i]=target[i]-self[i];}
                float ar=(float)(1./sqrt(((double)a[2]*a[2]+(double)a[1]*a[1])+(double)a[0]*a[0]));
                float br=(float)(1./sqrt(((double)b[2]*b[2]+(double)b[1]*b[1])+(double)b[0]*b[0]));
                for(unsigned i=0;i<3;i++){a[i]*=ar;b[i]*=br;}
                if((b[2]*a[2]+b[1]*a[1])+b[0]*a[0]>0)suppressed=0;}
            s->event_pending=0;
        }
    }
    float facing=(float)h->facing(o);int task=h->task_status(o);uint8_t busy=0;
    if(task!=0){
        if(!suppressed&&s->can_see){DO(c->stance_busy(o,&busy));
            if(!busy){if(facing>.6f)DO(c->stance(o,1,1,0));
                else{int32_t weapon;DO(c->weapon_mode(o,&weapon));DO(c->stance(o,weapon==2?0:4,1,0));}}}
        else if(h->child==-1||!h->boolean(o,h->child,"controlling attitude",0)){
            if(s->unseen_time<2&&(double)s->unseen_time+dt>=2)DO(c->aim_stop(o));
            double unseen=(double)s->unseen_time+dt;s->unseen_time=(float)unseen;
            DO(c->stance_busy(o,&busy));
            if(!busy){
                if(unseen>15&&s->allow_relax)DO(c->stance(o,0,1,0));
                else{int relax=1;
                    if(c->target(o)!=-1){
                        if(!(s->unseen_time>3&&facing>=.5f)&&!suppressed&&!(.5f>facing))relax=0;}
                    if(relax){int32_t weapon;DO(c->weapon_mode(o,&weapon));DO(c->stance(o,weapon==2?0:4,1,0));}}
            }
        }
    }
    double standing=h->number(o,h->child,"standing time",0);
    if(5>standing)s->aim_locked=0;
    if(!suppressed&&c->target(o)!=-1){
        float target[3];int found=h->position(o,c->target(o),target);if(found<0)return 2;
        if(!found)DO(h->clear_target(o));
        else{
            if(pt==c->target(o)){if(s->aim_delay==-100)s->aim_delay=2;else s->aim_delay=(float)((double)s->aim_delay-dt);}
            else s->aim_delay=-100;
            int a=h->gun_status(o,0x38);if(a<0)return 2;
            if(a){int b=h->gun_status(o,0x3c);if(b<0)return 2;
                if(!b){int d=h->gun_status(o,0x34);if(d<0)return 2;
                    if(!d){DO(h->say(o,7,50));DO(h->reload(o));goto tail;}}}
            DO(h->target_contact(o,c->target(o)));
            if(s->can_see){int response=h->melee_response(o);if(response<0)return 2;if(response)return 0;}
            /*1BB02C reuses the former facing stack slot for standing time.*/
            if(s->unseen_time<2||!(standing<5)&&!s->aim_locked){
                if(standing>=5)s->aim_locked=1;
                float point[3];int has=h->point(o,c->target(o),"target position",point);if(has<0)return 2;
                if(!has&&h->position(o,c->target(o),point)!=1)return 2;DO(h->aim(o,point));}
            if(s->unseen_time<2&&s->can_shoot&&s->aim_delay<=0&&!s->compliance){
                DO(c->stance_busy(o,&busy));if(busy)goto tail;
                int reloading=h->gun_status(o,0x34);if(reloading<0)return 2;if(reloading||h->task_status(o)==0)goto tail;
                int line=h->player_in_line(o);if(line<0)return 2;if(line||s->shot_delay>0)goto tail;
                float scale=h->integer(o,h->root,"order id",-1)!=-1?2.f:1.f;
                DO(h->fire(o,c->target(o),scale));
                if(h->coin(o))s->shot_delay=(float)h->random(o,1,1.5f);
                else s->shot_delay=(float)h->random(o,.33f,.5f);
            }
        }
    }else if(s->last_target!=-1)DO(c->aim_stop(o));
 tail:
    if(s->suppress_time>0)s->suppress_time=(float)((double)s->suppress_time-dt);
    s->shot_delay=(float)((double)s->shot_delay-dt);DO(h->base(o,expensive));
#undef DO
#undef BOOL_TO
    return 0;
}
