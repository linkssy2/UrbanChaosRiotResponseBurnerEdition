#include "character_ai_maintenance.h"
#include <cmath>
#include <cstring>
#include <new>
int NativeCharacterAIMaintenance::add(const Services&s,int32_t id){
    if(stepping_||id<0||!s.actor)return 1;uint8_t found=0,controller=0;uint32_t status=0;
    if(s.actor(s.owner,id,&found,&status,&controller)||found>1||controller>1)return 2;
    if(found)try{alive_.push_back({id,-1,0,0});}catch(const std::bad_alloc&){return 2;}return 0;
}
int NativeCharacterAIMaintenance::erase_dead(const Services&s,int32_t id){
    uint8_t found=0,controller=0,empty=0;uint32_t status=0;
    if(s.actor(s.owner,id,&found,&status,&controller)||found>1||controller>1)return 2;
    if(found&&controller){if(!s.tasks_empty||s.tasks_empty(s.owner,id,&empty)||empty>1)return 2;
        if(empty&&(!s.retire||s.retire(s.owner,id)))return 2;}
    for(auto i=dead_.begin();i!=dead_.end();++i)if(i->actor==id){dead_.erase(i);break;}return 0;
}
int NativeCharacterAIMaintenance::step(const Services&s,uint32_t frame,float dt){
    if(stepping_||!s.actor||!std::isfinite(dt)||dt<0)return 1;
    struct Scope{bool&b;Scope(bool&v):b(v){b=true;}~Scope(){b=false;}}scope(stepping_);
    for(auto i=alive_.begin();i!=alive_.end();){uint8_t found=0,controller=0;uint32_t status=0;
        if(s.actor(s.owner,i->actor,&found,&status,&controller)||found>1||controller>1)return 2;
        if(!found)i=alive_.erase(i);
        else if((status&5)&&!(status&0x1000)){try{dead_.push_back({i->actor,(int32_t)frame,0,0});}catch(const std::bad_alloc&){return 2;}i=alive_.erase(i);}
        else ++i;
    }
    for(auto i=dead_.begin();i!=dead_.end();){uint8_t found=0,controller=0,empty=0,unseen=0;uint32_t status=0;
        if(s.actor(s.owner,i->actor,&found,&status,&controller)||found>1||controller>1)return 2;
        if(!found){i=dead_.erase(i);continue;}
        i->age=float(double(i->age)+dt);
        if(!controller||!s.tasks_empty||s.tasks_empty(s.owner,i->actor,&empty)||empty>1)return 2;
        if(empty){if(!s.unobserved||s.unobserved(s.owner,i->actor,&unseen)||unseen>1)return 2;
            if(unseen){i->idle=float(double(i->idle)+dt);if(i->idle>3.f){int32_t id=i++->actor;if(erase_dead(s,id))return 2;continue;}}}
        ++i;
    }
    uint32_t dead=(uint32_t)dead_.size(),total=dead+(uint32_t)alive_.size(),remove=dead>5?dead-5:0;
    if(total-remove>(uint32_t)limit_)remove=total-(uint32_t)limit_;if(remove>dead)remove=dead;
    while(remove--&&!dead_.empty()){int32_t earliest=(int32_t)frame,id=-1;
        for(const auto&e:dead_)if(e.frame<earliest&&e.age>3.f){id=e.actor;earliest=e.frame;}
        if(id!=-1&&erase_dead(s,id))return 2;
    }
    return 0;
}
int NativeCharacterAIMaintenance::record(int32_t actor,int32_t kind,const float start[3],const float end[3],int32_t player){
    if(!start||!end)return 1;for(unsigned i=0;i<3;i++)if(!std::isfinite(start[i])||!std::isfinite(end[i]))return 1;
    Shot shot{};shot.owner_id=uint32_t(actor);shot.bullet_id=uint32_t(kind);std::memcpy(shot.from,start,12);std::memcpy(shot.to,end,12);
    uint32_t id=uint32_t(player);return uc_bullet_history_append(&history_,&shot,player==-1?nullptr:&id);
}
int NativeCharacterAIMaintenance::finish(float dt){if(!std::isfinite(dt)||dt<0)return 1;return uc_bullet_history_advance(&history_,dt);}
int NativeCharacterAIMaintenance::bullet_by(const ShotServices&s,int32_t id,bool check,uint32_t*out)const{
    if(!out||!s.actor)return 1;*out=0;
    for(uint32_t i=0;i<history_.count;i++){
        const auto&shot=history_.entries[i];if(shot.owner_id==uint32_t(id)||shot.bullet_id==uint32_t(id))continue;
        float position[3],forward[3];int found=s.actor(s.owner,id,position,forward);if(found<0||found>1)return 2;if(!found)continue;
        float p[3],d[3];for(unsigned j=0;j<3;j++){p[j]=position[j]-shot.from[j];d[j]=shot.to[j]-shot.from[j];}
        float t=((d[2]*p[2]+d[0]*p[0])+d[1]*p[1])/((d[2]*d[2]+d[0]*d[0])+d[1]*d[1]);if(t>1)t=1;
        for(unsigned j=0;j<3;j++)p[j]=(shot.from[j]+d[j]*t)-position[j];
        double distance=std::sqrt(double(p[2])*p[2]+double(p[0])*p[0]);
        if(!(distance<250)||!(p[1]<400))continue;
        if(check&&shot.owner_id!=UINT32_MAX){uint8_t handled=0,result=0;
            if(!s.relation||s.relation(s.owner,id,int32_t(shot.owner_id),&handled,&result)||handled>1||result>1)return 2;
            if(handled&&!result)continue;
        }
        const float x=0.f-forward[2],z=forward[0];
        const float inv=float(1.0/std::sqrt(double(x)*x+double(z)*z)),near_inv=1.f/float(distance);
        const float dot=((inv*z)*(p[2]*near_inv)+(inv*x)*(near_inv*p[0]))+(inv*0.f)*(near_inv*0.f);
        *out=(shot.owner_id==uint32_t(s.first_player)?4u:0u)|(dot>0?1u:2u);return 0;
    }
    return 0;
}
