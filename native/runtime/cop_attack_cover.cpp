#include "cop_attack_cover.h"
#include <cstring>
#include <stdexcept>
namespace {void require(bool v,const char*m){if(!v)throw std::runtime_error(m);}}
NativeCopAttackCover::NativeCopAttackCover(NativeMissionWorld&w,NativeCharacterCoverPoints&p,
    NativeCharacterNavigationManager&m,NativeCharacterNavigator&n,int32_t actor,int32_t behavior,int32_t authored)
    :world_(w),points_(p),manager_(m),navigator_(n),actor_(actor),sender_(behavior),authored_(authored){
    require(actor>0&&behavior>0,"Cop cover owner inputs");
}
void NativeCopAttackCover::release(int32_t id){
    if(id==-1)return;int32_t vacant=-1;uint32_t sender=uint32_t(sender_);uint8_t handled=0;
    //1F90A0 silently completes a missing/stale selected cover; no forced create.
    require(!uc_entity_event_send(&world_.registry(),id,32,&vacant,4,&sender,&handled),"Cop cover release event");
}
void NativeCopAttackCover::claim(UCCopAttack&s){
    auto*p=points_.find(s.cover);int32_t occupant=-1;int found=p?p->property("occupant",&occupant):0;
    require(found>=0,"Cop cover occupant property");
    if(!found){s.cover=-1;s.state=2;return;}if(occupant!=-1)return;
    require(!p->reserve(actor_,sender_),"Cop cover reserve event");
    //Same real query as controller21D20; no direct teleport or synthetic arrival.
    navigator_.retarget(p->entity.transform+12,manager_.query(actor_));s.state=1;
}
void NativeCopAttackCover::step(UCCopAttack&s){
    require(s.actor_world!=nullptr,"Cop cover actor world");
    if(authored_!=-1){require(!uc_level_entity_lookup(&world_.slots(),uint32_t(authored_),&s.cover),"Cop authored cover resolution");claim(s);return;}
    if(search_){
        if(search_->status()==4){release(s.cover);s.cover=search_->result();search_.reset();claim(s);}
        else if(search_->status()==3){search_.reset();s.state=2;}
        return;
    }
    float direction[3];const float*m=s.actor_world;
    for(unsigned i=0;i<3;i++)direction[i]=(m[i]*0.f+m[4+i]*0.f)+(m[8+i]*1.f+0.f);direction[1]=0;
    search_=std::make_unique<NativeCharacterCoverSearch>(points_,manager_,actor_,m+12,direction);
}
void NativeCopAttackCover::close(UCCopAttack&s){search_.reset();release(s.cover);}
