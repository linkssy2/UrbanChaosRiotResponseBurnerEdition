#include "character_threat.h"
#include <cmath>
#include <cstring>
namespace {
void copy(float*a,const float*b){std::memcpy(a,b,12);}
float angle(float value){const double pi=3.1415927410125732421875,tau=6.283185482025146484375;
    return value>0?float(std::fmod(double(value)+pi,tau)-pi):float(-(std::fmod(pi-double(value),tau)-pi));}
int query(int value){return value<0?-1:value!=0;}
}
int native_character_threat_begin(NativeCharacterThreat&s,const NativeCharacterThreatServices&h,bool head,bool sides){
    if(!h.position||!h.direction||!h.random)return 1;
    s.head=head;s.side_test=sides;s.current=-1;s.accuracy=0;s.near_line=false;s.near_time=0;s.away_time=0;s.shield_time=0;
    s.reserved84=false;s.in_cone=false;s.cone_time=0;s.ever_in_cone=false;s.sides_clear=false;s.visible=false;s.unseen_time=0;s.blocked=false;s.facing=false;
    s.phase=float(h.random(h.owner,0,5));float forward[3];if(h.direction(h.owner,false,forward))return 1;
    for(unsigned i=0;i<3;i++)s.last_clear[i]=h.position[i]+forward[i]*500.f;s.initialized=true;return 0;
}
int native_character_threat_shot(NativeCharacterThreat&s,const NativeCharacterThreatServices&h,bool fired){
    if(!fired)return 0;if(!h.difficulty)return 1;
    const double accuracy=double(s.accuracy)-h.difficulty(h.owner,.9f,.05f);s.accuracy=accuracy<0?0:float(accuracy);return 0;
}
int native_character_threat_step(NativeCharacterThreat&s,const NativeCharacterThreatServices&h,bool expensive){
    if(!s.initialized||!h.target||!h.resolve||!std::isfinite(h.delta)||h.delta<0)return 1;
    s.phase=float(double(s.phase)+h.delta);
    if(h.target(h.owner)!=s.current){s.current=h.target(h.owner);s.ever_in_cone=false;s.changed=true;s.unseen_time=s.seen_time=s.cone_time=0;}
    float position[3];const int found=h.resolve(h.owner,s.current,position);if(found<0)return 1;if(!found)s.current=-1;
    if(s.current==-1){s.near_line=false;s.near_time=s.away_time=s.shield_time=0;s.reserved84=false;s.in_cone=false;s.cone_time=0;
        s.ever_in_cone=false;s.visible=false;s.unseen_time=s.accuracy=0;s.blocked=false;return 0;}
    if(!h.position||!h.body_forward||!h.direction||!h.property||!h.point||!h.visible||!h.near_line||!h.recent||!h.segment_clear||!h.difficulty)return 1;
    copy(s.target,position);if(s.changed)copy(s.anchor,position);
    const float dx=s.anchor[0]-s.target[0],dz=s.anchor[2]-s.target[2];
    int32_t shield=0;const int property=h.property(h.owner,s.current,"shield active",&shield);if(property<0)return 1;
    if(property&&uint8_t(shield)){const double value=h.delta+s.shield_time;s.shield_time=value<3.5?float(value):3.5f;}
    else {const double value=double(s.shield_time)-h.delta;s.shield_time=value<0?0:float(value);}
    s.moved=s.changed||std::sqrt(double(dx)*dx+double(dz)*dz)>200.;if(s.moved)copy(s.anchor,s.target);
    for(unsigned i=0;i<3;i++)s.direction[i]=s.target[i]-h.position[i];
    s.heading=angle(float(std::atan2(double(s.direction[0]),double(s.direction[2]))));s.direction[1]=0;
    const float square=(s.direction[0]*s.direction[0]+s.direction[1]*s.direction[1])+s.direction[2]*s.direction[2];
    s.distance=float(std::sqrt(double(square)));const double inv=1./std::sqrt(double(square));
    for(float&v:s.direction)v=float(inv*v);
    float forward[3];if(h.body_forward(h.owner,h.actor,forward))return 1;
    float norm=float(1./std::sqrt(double(forward[2]*forward[2]+forward[0]*forward[0])));
    const float fx=norm*forward[0],fy=norm*0.f,fz=norm*forward[2];
    s.facing_dot=(s.direction[2]*fz+s.direction[1]*fy)+s.direction[0]*fx;s.facing=s.facing_dot>.87f;
    if(h.body_forward(h.owner,s.current,forward))return 1;
    norm=float(1./std::sqrt(double(forward[2]*forward[2]+forward[0]*forward[0])));
    s.side_dot=((0.f-s.direction[2])*(norm*forward[0])+(0.f-s.direction[0])*(0.f-norm*forward[2]))+(0.f-s.direction[1])*0.f;
    const int point=h.point(h.owner,s.current,"target position",s.aim);if(point<0)return 1;
    if(!point){copy(s.aim,s.target);s.aim[1]+=1.5f;}
    const float oscillation=float(std::cos(double(s.phase)+s.phase+1.5f));float right[3];if(h.direction(h.owner,true,right))return 1;
    const float wave=oscillation*13.f,distance=s.distance*.002f;
    for(unsigned i=0;i<3;i++)s.jitter[i]=(right[i]*wave)*distance;
    s.jitter[1]=float((std::sin(double(s.phase))*distance)*20.f);
    for(unsigned i=0;i<3;i++)s.aim[i]+=s.jitter[i];
    const float zero[3]{};
    if(expensive){int result=query(h.visible(h.owner,s.current,-1,s.head,zero,true));if(result<0)return 1;s.visible=result!=0;
        if(s.visible){
            if(s.side_test){if(h.direction(h.owner,true,right))return 1;for(float&v:right)v*=.1f;
                result=query(h.visible(h.owner,s.current,-1,s.head,right,false));if(result<0)return 1;
                if(result){for(float&v:right)v=0.f-v;result=query(h.visible(h.owner,s.current,.71f,s.head,right,true));if(result<0)return 1;}
                s.sides_clear=result!=0;}
            result=query(h.visible(h.owner,s.current,.71f,s.head,zero,true));if(result<0)return 1;s.in_cone=result!=0;
        }else s.in_cone=false;
    }else if(s.changed)s.visible=s.in_cone=false;
    if(s.in_cone&&!s.changed){s.ever_in_cone=true;s.cone_time=float(h.delta+s.cone_time);}else s.cone_time=0;
    if(s.visible){s.seen_time=float(h.delta+s.seen_time);s.unseen_time=0;}else{s.seen_time=0;s.unseen_time=float(h.delta+s.unseen_time);}
    const float threshold=float(h.difficulty(h.owner,.5f,0));const int near_line=query(h.near_line(h.owner,s.current,100));if(near_line<0)return 1;
    float cap;
    if(near_line&&s.visible){s.near_line=true;s.near_time=float(h.delta+s.near_time);s.away_time=0;
        if(s.seen_time>threshold){const float rate=float(h.difficulty(h.owner,.2f,1));s.accuracy=float((h.delta*rate)*.4f+s.accuracy);}
        cap=float(h.difficulty(h.owner,.4f,.9f));
    }else{s.near_line=false;const double decay=double(s.near_time)-(h.delta+h.delta);s.near_time=decay<0?0:float(decay);s.away_time=float(h.delta+s.away_time);
        if(s.seen_time>threshold){const float rate=float(h.difficulty(h.owner,.2f,1));s.accuracy=float((h.delta*rate)*.25f+s.accuracy);}
        cap=float(h.difficulty(h.owner,.3f,.8f));}
    if(!(cap>s.accuracy))s.accuracy=cap;
    const float timeout=float(h.difficulty(h.owner,.1f,4));float stamp=0;const int recent=query(h.recent(h.owner,&stamp));if(recent<0)return 1;
    if(recent&&h.time>stamp&&1.f>h.time-stamp){s.unseen_time=timeout;s.seen_time=0;}
    if(s.unseen_time>=timeout)s.accuracy=0;
    if(h.difficulty(h.owner,0,1)>.6f){cap=float(h.difficulty(h.owner,0,.3f));if(!(cap>s.accuracy))s.accuracy=cap;}
    float from[3];copy(from,h.position);from[1]+=150;
    const int origin=h.point(h.owner,h.actor,s.head?"head pos":"target position",from);if(origin<0)return 1;
    s.blocked=false;if(s.visible){const int clear=query(h.segment_clear(h.owner,from,s.aim));if(clear<0)return 1;s.blocked=!clear;}
    if(s.visible&&!s.blocked)copy(s.last_clear,s.aim);s.changed=false;return 0;
}
