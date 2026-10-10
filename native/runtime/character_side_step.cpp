#include "character_side_step.h"
#include <cmath>
#include <cstring>
#include <stdexcept>
namespace {
void require(bool ok,const char*why){if(!ok)throw std::runtime_error(why);}
//214490: ray/sphere, including its inside-sphere exit and float intermediates.
bool sphere(const float origin[3],const float direction[3],const float center[3],float point[3]){
    float d[3]={center[0]-origin[0],center[1]-origin[1],center[2]-origin[2]};
    float dot=(direction[2]*d[2]+direction[0]*d[0])+d[1]*direction[1];
    float squared=(d[2]*d[2]+d[0]*d[0])+d[1]*d[1];
    if(dot<0&&squared>10000)return false;
    float perpendicular=squared-dot*dot;if(perpendicular>10000)return false;
    float root=(float)std::sqrt(10000.-(double)perpendicular);
    float distance=squared>10000?dot-root:root+dot;
    for(unsigned i=0;i<3;i++)point[i]=direction[i]*distance+origin[i];return true;
}
bool in_way(const float actor[3],const float target[3],const float movement[3],float delta){
    float d[3]={target[0]-actor[0],target[1]-actor[1],target[2]-actor[2]};
    if(100.>std::sqrt(((double)d[2]*d[2]+(double)d[1]*d[1])+(double)d[0]*d[0]))return true;
    float squared=(movement[2]*movement[2]+movement[1]*movement[1])+movement[0]*movement[0];if(squared==0)return false;
    require(std::isfinite(delta)&&delta>0,"Side step requires positive source delta");
    float inverse=(float)(1./delta),velocity[3],direction[3],point[3];for(unsigned i=0;i<3;i++)velocity[i]=movement[i]*inverse;
    inverse=(float)(1./std::sqrt(((double)velocity[2]*velocity[2]+(double)velocity[1]*velocity[1])+(double)velocity[0]*velocity[0]));
    for(unsigned i=0;i<3;i++)direction[i]=velocity[i]*inverse;if(!sphere(target,direction,actor,point))return false;
    for(float&v:velocity)v=(float)((double)v*.30000001192092896);
    double x=(double)point[0]-target[0],y=(double)point[1]-target[1],z=(double)point[2]-target[2];
    return std::sqrt(((double)velocity[2]*velocity[2]+(double)velocity[1]*velocity[1])+(double)velocity[0]*velocity[0])>
        std::sqrt((z*z+y*y)+x*x);
}
}
struct NativeCharacterSideStep::Completion {
    NativeCharacterAnimationCallback callback;unsigned status=0;bool released=false;
    Completion():callback{this,[](void*p,bool cancelled){auto*c=static_cast<Completion*>(p);c->status=cancelled?2:1;if(c->released)delete c;}}{}
    void release(){released=true;if(status)delete this;}
};
NativeCharacterSideStep::~NativeCharacterSideStep(){close();}
void NativeCharacterSideStep::close(){if(completion_){completion_->release();completion_=nullptr;}}
void NativeCharacterSideStep::reset(int32_t target,float threshold){
    require(target>=0&&std::isfinite(threshold)&&threshold>=0,"Side step binding");target_=target;threshold_=threshold;
}
bool NativeCharacterSideStep::force_requested(const Services&h)const{
    require(target_>=0&&h.actor_world&&h.busy&&h.world,"Forced side step real services");
    if(h.busy(h.owner))return false;
    const float*target=h.world(h.owner,target_);if(!target||h.child!=-1)return false;
    const double x=(double)h.actor_world[12]-target[12],y=(double)h.actor_world[13]-target[13],z=(double)h.actor_world[14]-target[14];
    return 300.>std::sqrt((z*z+y*y)+x*x);
}
void NativeCharacterSideStep::update(const Services&h){
    require(target_>=0&&threshold_==7.5f&&h.actor_world&&h.busy&&h.player&&h.world&&h.movement&&h.reset_navigation&&h.ray&&h.approach,"Side step real services");
    if(h.busy(h.owner)||(completion_&&!completion_->status)||h.child!=-1)return;
    require(target_==h.player(h.owner),"Side step non-player target3B910 is unconnected");
    const float*target=h.world(h.owner,target_);require(target,"Side step target lifetime");const float*actor=h.actor_world+12;
    float movement[3]{};float difference[3]={target[12]-actor[0],target[13]-actor[1],target[14]-actor[2]};
    bool nearby=100.>std::sqrt(((double)difference[2]*difference[2]+(double)difference[1]*difference[1])+(double)difference[0]*difference[0]);
    if(!nearby){require(h.movement(h.owner,target_,movement)==1,"Side step target movement property");if(!in_way(actor,target+12,movement,h.delta))return;}
    h.reset_navigation(h.owner);target=h.world(h.owner,target_);require(target,"Side step target lifetime after reset");
    require(h.movement(h.owner,target_,movement)==1,"Side step target movement property");
    if(movement[0]<=.1f&&movement[2]<=.1f)for(unsigned i=0;i<3;i++)movement[i]=(target[i]*0.f+target[4+i]*0.f)+(target[8+i]*10.f+0.f);
    float future[3],line[3],relative[3];for(unsigned i=0;i<3;i++){future[i]=target[12+i]+movement[i];line[i]=future[i]-target[12+i];relative[i]=actor[i]-target[12+i];}
    float fraction=(line[2]*relative[2]+line[0]*relative[0])/(line[2]*line[2]+line[0]*line[0]);
    float x=actor[0]-(line[0]*fraction+target[12]),z=actor[2]-(line[2]*fraction+target[14]);
    float inverse=(float)(1./std::sqrt((double)x*x+(double)z*z));
    float point[3]={actor[0]+(x*inverse)*80.f,actor[1]+(inverse*0.f)*80.f,actor[2]+(z*inverse)*80.f};
    auto clear=[&](const float to[3]){uint8_t visible;int32_t blocker;require(!uc_character_clearance(actor,to,100,h.region,0,h.ray,h.owner,&visible,&blocker),"Side step clearance");return visible||blocker==target_;};
    if(!clear(point)){
        inverse=(float)(1./std::sqrt(((double)line[2]*line[2]+(double)line[1]*line[1])+(double)line[0]*line[0]));
        for(unsigned i=0;i<3;i++)point[i]=actor[i]+(line[i]*inverse)*80.f;if(!clear(point))return;
    }
    //2AF690 takes a duration; actual3B3B0 supplies1. Normalize source forward
    //with its original float squared sum and x87 reciprocal, including fallback.
    float direction[3];float squared=h.actor_world[8]*h.actor_world[8]+h.actor_world[10]*h.actor_world[10];
    if(.009999999776482582>std::sqrt((double)squared)){direction[0]=1;direction[1]=direction[2]=0;}
    else{double reciprocal=1./std::sqrt((double)squared);direction[0]=(float)((double)h.actor_world[8]*reciprocal);direction[1]=(float)(0.*reciprocal);direction[2]=(float)((double)h.actor_world[10]*reciprocal);}
    close();completion_=new Completion;try{h.approach(h.owner,point,direction,&completion_->callback,1.f);}catch(...){
        //A failing service may already have queued the callback; preserve its
        //source lifetime until that actual graph retires rather than free early.
        close();throw;
    }
}
