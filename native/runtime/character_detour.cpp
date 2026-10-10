#include "character_navigation.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace {void require(bool ok,const char *s){if(!ok)throw std::runtime_error(s);}}
int NativeCharacterDetour::path_clearance(void *owner,const float *from,const float *to,float radius,uint32_t region,uint8_t *clear){
    auto&q=*static_cast<const UCCharacterCandidateQuery*>(owner);int32_t entity=0;
    return uc_character_clearance(from,to,radius,region,0,q.ray,q.owner,clear,&entity);
}
bool NativeCharacterDetour::world_blocked(const UCCharacterCandidateQuery&q,const float *from,const float *to,bool block_scripted){
    uint8_t blocked;require(!uc_character_world_blocked(from,to,q.radius,q.region,0,(uint8_t)block_scripted,q.ray,q.scripted,q.owner,&blocked),"Obstacle path world query");return blocked!=0;
}
bool NativeCharacterDetour::path_clear(const UCCharacterCandidateQuery&q,const std::vector<Point>&path){
    for(size_t i=1;i<path.size();i++){
        const float *a=path[i-1].data(),*b=path[i].data();float d[3];for(unsigned j=0;j<3;j++)d[j]=b[j]-a[j];
        if((d[0]*d[0]+d[1]*d[1])+d[2]*d[2]<.0001f)continue;
        uint8_t clear;int32_t entity;require(!uc_character_clearance(a,b,q.radius,q.region,0,q.ray,q.owner,&clear,&entity),"Detour clearance");
        if(!clear&&(!entity||!q.scripted(q.owner,entity)))return false;
    }return true;
}
void NativeCharacterDetour::reset(){blocking_=nullptr;paths_[0].clear();paths_[1].clear();selected_=-1;cursor_=0;}
int NativeCharacterDetour::step(UCCharacterAgentSteering&s,const UCCharacterCandidateQuery&q,const float goal[3],
    float margin,const Obstacle *obstacles,uint32_t count){
    require(q.position&&q.ray&&q.scripted&&goal&&(!count||obstacles)&&count<=4096&&std::isfinite(margin)&&margin>=0,"Detour inputs");
    for(unsigned j=0;j<3;j++)require(std::isfinite(goal[j])&&fabsf(goal[j])<=1e6f,"Detour goal");
    std::vector<UCCharacterObstacle> shapes;shapes.reserve(count);
    for(uint32_t i=0;i<count;i++){require(obstacles[i].identity&&obstacles[i].detour&&obstacles[i].detour_count<=8192,"Detour obstacle ownership");shapes.push_back(obstacles[i].collision);}
    auto casts=[&](const float*a,const float*b,UCCharacterObstacleHit*out){int hit=uc_character_obstacles_cast(q.height,q.position,a,b,shapes.data(),count,out);require(hit>=0,"Detour obstacle cast");return hit!=0;};
    UCCharacterObstacleHit hit;
    if(!casts(q.position,s.waypoint,&hit)){reset();return 1;}
    const auto&o=obstacles[hit.index];
    auto blocked=[&](const float*p){std::copy_n(p,3,s.waypoint);s.status=3;return 0;};
    if(blocking_!=o.identity){
        blocking_=o.identity;s.blocker=0;s.changed=1;attempts_=0;cursor_=0;
        float x=goal[0]-o.collision.center[0],z=goal[2]-o.collision.center[2];
        if(o.collision.bounding_radius>sqrt((double)(x*x+z*z)))return blocked(hit.position);
        uint32_t sizes[2]{};for(auto&p:paths_)p.resize(o.detour_count+2);
        int found=uc_character_obstacle_detours(q.position,s.waypoint,o.detour,o.detour_count,
            reinterpret_cast<float(*)[3]>(paths_[0].data()),reinterpret_cast<float(*)[3]>(paths_[1].data()),o.detour_count+2,sizes);
        require(found>=0,"Detour construction");for(unsigned i=0;i<2;i++)paths_[i].resize(sizes[i]);
        if(!found)return blocked(hit.position);
        auto length=[](const std::vector<Point>&p){double total=0;for(size_t i=1;i<p.size();i++){float d[3];for(unsigned j=0;j<3;j++)d[j]=p[i-1][j]-p[i][j];total+=sqrt(((double)d[2]*d[2]+(double)d[1]*d[1])+(double)d[0]*d[0]);}return total;};
        forward_=length(paths_[1])>(float)length(paths_[0]);selected_=forward_?0:1;
        if(!path_clear(q,paths_[selected_])){++attempts_;forward_=!forward_;selected_=forward_?0:1;if(!path_clear(q,paths_[selected_]))return blocked(q.position);}
    }
    require(selected_>=0&&!paths_[selected_].empty()&&cursor_<paths_[selected_].size(),"Detour active cursor");auto&path=paths_[selected_];
    //201D0 points are all untagged. Shortcut only after both source checks.
    if(cursor_+1<path.size()&&!world_blocked(q,q.position,path[cursor_+1].data(),true)&&!casts(q.position,path[cursor_+1].data(),nullptr)){++cursor_;s.changed=1;}
    uint8_t active;require(!uc_character_path_advance(reinterpret_cast<const float(*)[3]>(path.data()),(uint32_t)path.size(),&cursor_,q.position,margin,100,50,q.region,
        path_clearance,const_cast<UCCharacterCandidateQuery*>(&q),&s.changed,&active,s.waypoint),"Detour path cursor");
    if(!active){reset();s.blocker=0;s.changed=1;return 1;}
    if(s.changed){
        s.blocker=0;
        if(world_blocked(q,q.position,s.waypoint,false)||casts(q.position,s.waypoint,nullptr)){
            s.changed=1;if(++attempts_==2)return blocked(q.position);
            std::vector<Point> prefix(path.begin(),path.begin()+cursor_);std::reverse(prefix.begin(),prefix.end());
            const auto&other=paths_[forward_?1:0];prefix.insert(prefix.end(),other.begin(),other.end());
            path=std::move(prefix);cursor_=0;forward_=!forward_;
        }
    }return 1;
}
