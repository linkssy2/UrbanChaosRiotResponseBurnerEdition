#include "character_attack_position.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace {
void require(bool ok,const char *s){if(!ok)throw std::runtime_error(s);}
void copy(float *a,const float *b){std::memcpy(a,b,12);}
float angle(float v){const double pi=3.1415927410125732421875,tau=6.283185482025146484375;
    return v>0?float(std::fmod(double(v)+pi,tau)-pi):float(-(std::fmod(pi-double(v),tau)-pi));}
double square(const float *v){return (double(v[0])*v[0]+double(v[1])*v[1])+double(v[2])*v[2];}
double reverse_square(const float *v){return (double(v[2])*v[2]+double(v[1])*v[1])+double(v[0])*v[0];}
void radial(float *out,const float *center,float heading,float radius){
    out[0]=float(-std::sin(double(heading))*radius+center[0]);out[1]=center[1];
    out[2]=float(-std::cos(double(heading))*radius+center[2]);
}
}
NativeCharacterAttackPosition::NativeCharacterAttackPosition(const UCLevelLogic &logic,
    NativeCharacterNavigationManager &navigation,Services services,int32_t actor,uint16_t region,float depth)
    :logic_(logic),navigation_(navigation),services_(services){
    require(services.target&&services.position&&services.random&&services.ray&&std::isfinite(depth),"Attack-position services");
    require(services.position(services.owner,actor)!=nullptr,"Attack-position actor missing");
    auto&s=state_;s.actor=actor;s.region=region;s.ground_depth=depth;s.initialized=true;
    job_.owner=this;
    job_.initialize=[](auto&j){auto&p=*static_cast<NativeCharacterAttackPosition*>(j.owner);p.initialize_route();j.complete=p.route_status_>=3;};
    job_.step=[](auto&j){auto&p=*static_cast<NativeCharacterAttackPosition*>(j.owner);p.step_route();j.complete=p.route_status_>=3;};
    reset();
}
NativeCharacterAttackPosition::~NativeCharacterAttackPosition(){cancel_route();}
void NativeCharacterAttackPosition::ranges(float minimum,float maximum){
    require(std::isfinite(minimum)&&std::isfinite(maximum),"Attack-position ranges");state_.range_min=minimum;state_.range_max=maximum;
}
void NativeCharacterAttackPosition::defend(int32_t source,float minimum,float maximum){
    require(std::isfinite(minimum)&&std::isfinite(maximum),"Attack-position defend ranges");state_.defend=source;state_.defend_min=minimum;state_.defend_max=maximum;
}
bool NativeCharacterAttackPosition::refresh(){
    auto&s=state_;const float *target=services_.position(services_.owner,s.target),*actor=services_.position(services_.owner,s.actor);
    if(!target||!actor)return false;copy(s.target_position,target);copy(s.position,actor);
    for(unsigned j=0;j<3;j++)s.direction[j]=s.target_position[j]-s.position[j];
    s.distance=float(std::sqrt(square(s.direction)));s.heading=angle(float(std::atan2(double(s.direction[0]),double(s.direction[2]))));return true;
}
void NativeCharacterAttackPosition::reset(){
    auto&s=state_;s.attempts=s.rejected=0;int32_t target=services_.target(services_.owner);
    if(target!=s.target){s.target=target;if(target!=-1)refresh();}
    if(s.target!=-1){copy(s.anchor,s.target_position);copy(s.chosen,s.position);s.status=1;cancel_route();}
    else {const float*p=services_.position(services_.owner,s.actor);require(p!=nullptr,"Attack-position reset actor");
        copy(s.position,p);copy(s.anchor,p);copy(s.target_position,p);s.status=0;}
}
void NativeCharacterAttackPosition::clear(){state_.status=0;state_.attempts=0;}
void NativeCharacterAttackPosition::refresh_defend(){
    auto&s=state_;if(s.defend==-1)return;
    require(services_.defend_position!=nullptr,"Attack-position defend source resolver");s.defend_valid=false;
    const float*p=services_.defend_position(services_.owner,s.defend);if(!p)return;copy(s.defend_position,p);s.defend_valid=true;
    if(s.target!=-1){
        for(unsigned j=0;j<3;j++)s.defend_target_direction[j]=s.target_position[j]-s.defend_position[j];
        const double distance=std::sqrt(reverse_square(s.defend_target_direction));s.defend_target_distance=float(distance);
        if(distance>0){const double inv=1./std::sqrt(square(s.defend_target_direction));for(float&v:s.defend_target_direction)v=float(double(v)*inv);}
        s.defend_heading=angle(float(std::atan2(-double(s.defend_target_direction[0]),-double(s.defend_target_direction[2]))));
    }
    for(unsigned j=0;j<3;j++)s.defend_actor_direction[j]=s.position[j]-s.defend_position[j];
    const double distance=std::sqrt(reverse_square(s.defend_actor_direction));s.defend_actor_distance=float(distance);
    if(distance>double(.1f)){const double inv=1./std::sqrt(square(s.defend_actor_direction));for(float&v:s.defend_actor_direction)v=float(double(v)*inv);}
    else {s.defend_actor_direction[0]=s.defend_actor_direction[2]=0;s.defend_actor_direction[1]=1;}
    s.min_square=40000;
}
bool NativeCharacterAttackPosition::allowed(const float*p)const{
    const auto&s=state_;if(!s.defend_valid)return true;
    const float x=p[0]-s.defend_position[0],y=p[1]-s.defend_position[1],z=p[2]-s.defend_position[2];
    const float value=(z*z+x*x)+y*y;return !(s.defend_min*s.defend_min>value||value>s.defend_max*s.defend_max);
}
int NativeCharacterAttackPosition::ray(const float *from,const float *to,uint32_t mask,float *out)const{
    int hit=services_.ray(services_.owner,from,to,0x20,state_.region,mask,0,out);require(hit>=0&&hit<=1,"Attack-position collision query");return hit;
}
bool NativeCharacterAttackPosition::clear_segment(const float *point)const{
    float from[3],to[3];copy(from,point);copy(to,state_.target_position);from[1]+=150;to[1]+=150;
    const double x=double(to[0])-from[0],z=double(to[2])-from[2];if(std::sqrt(x*x+z*z)<double(.1f))return true;
    float out[3];return ray(from,to,0x4000,out)==0;
}
void NativeCharacterAttackPosition::sample(){
    auto&s=state_;if(++s.attempts>150){s.mode=0;s.attempts=150;}
    const float spread=(float(s.attempts)*0.006666666828095913f)*3.f;
    const bool base_allowed=allowed(s.position);
    auto random=[&](float a,float b){return float(services_.random(services_.owner,a,b));};
    float heading,radius;
    if(s.defend_valid){
        const float dot=(s.defend_actor_direction[2]*s.defend_target_direction[2]+s.defend_actor_direction[1]*s.defend_target_direction[1])+s.defend_actor_direction[0]*s.defend_target_direction[0];
        if(dot>.95f||s.defend_max>s.defend_target_distance)radius=random(s.defend_min,std::max(s.defend_min+100.f,s.defend_target_distance-100.f));
        else radius=random(s.defend_min,s.defend_max);
        const float offset=dot>.95f?random(-.2f,.2f):random(-.2f-spread,spread+.2f);
        heading=angle(s.defend_heading+angle(offset));radial(s.candidate,s.defend_position,heading,radius);
    }else{
        float offset=0;
        switch(s.mode){case 0:offset=random(-.3f-spread,spread+.3f);break;case 1:offset=random(-.75f-spread,.1f);break;case 2:offset=random(.1f,spread+.75f);break;}
        heading=angle(s.heading+angle(offset));
        if(s.attempts>=150)radius=random(200,5000);
        else if(s.range_min>s.distance)radius=std::min(random(s.distance+s.range_min*.5f,s.distance+s.range_max*.75f),s.range_max-50.f);
        else if(s.range_max>s.distance)radius=random(s.range_min+50.f,s.range_max-50.f);
        else radius=std::max(random(s.distance-1000.f,s.distance-500.f),s.range_min+50.f);
        radial(s.candidate,s.target_position,heading,radius);
    }
    if(allowed(s.candidate)){
        float from[3],to[3],hit[3];copy(from,s.candidate);copy(to,s.candidate);from[1]+=200;to[1]+=0.f-s.ground_depth;
        if(ray(from,to,0x2180,hit)){
            copy(s.candidate,hit);
            if((s.defend_valid&&!base_allowed)||s.position[1]+50.f>s.candidate[1]){
                const float x=s.candidate[0]-s.position[0],y=s.candidate[1]-s.position[1],z=s.candidate[2]-s.position[2];
                if(s.rejected>=25||((z*z+y*y)+x*x>s.min_square&&clear_segment(s.candidate))){copy(s.chosen,s.candidate);s.status=2;return;}
            }
        }
    }
    if(!base_allowed)++s.rejected;else s.rejected=0;
}
void NativeCharacterAttackPosition::cancel_route(){
    if(!route_owned_)return;navigation_.search_manager().remove(job_);route_.reset();route_owned_=false;route_status_=0;
}
void NativeCharacterAttackPosition::begin_route(){
    require(!route_owned_,"Attack-position duplicate route");copy(route_from_,state_.position);copy(route_to_,state_.chosen);
    route_status_=0;route_owned_=true;job_.complete=false;navigation_.search_manager().add(job_);
}
void NativeCharacterAttackPosition::initialize_route(){
    //1DE80 resolves both endpoints at scheduler initialization, not allocation.
    if(!services_.position(services_.owner,state_.actor)){route_status_=3;return;}
    const auto q=navigation_.query(state_.actor);int32_t from=-1,to=-1;
    require(!uc_character_navigation_endpoint(&logic_,route_from_,q.region,q.ray,q.owner,&from),"Attack-position start node");
    if(from<0){route_status_=3;return;}
    require(!uc_character_navigation_endpoint(&logic_,route_to_,q.region,q.ray,q.owner,&to),"Attack-position goal node");
    if(to<0){route_status_=3;return;}
    route_=std::make_unique<NativeCharacterRoute>(logic_,from,to,route_from_,route_to_);route_status_=route_->status();
}
void NativeCharacterAttackPosition::step_route(){require(route_!=nullptr,"Attack-position route not initialized");route_->step();route_status_=route_->status();}
void NativeCharacterAttackPosition::request(int iterations){
    auto&s=state_;if(services_.target(services_.owner)!=s.target)s.target=services_.target(services_.owner);
    if(s.target==-1||!refresh())return;refresh_defend();
    const float x=s.target_position[0]-s.anchor[0],y=s.target_position[1]-s.anchor[1],z=s.target_position[2]-s.anchor[2];
    if((x*x+y*y)+z*z>250000.f)reset();
    for(int i=0;i<iterations;i++){
        switch(s.status){
        case 1:sample();if(s.status==2)return;break;
        case 2:
            if(!route_owned_)begin_route();
            else if(route_status_==4){cancel_route();s.status=3;return;}
            else if(route_status_==3){cancel_route();s.status=1;continue;}
            break;
        case 3:s.attempts=0;break;
        default:reset();break;
        }
        if(s.status!=1)return;
    }
}
