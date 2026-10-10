#include "character_hazards.h"
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
struct Region {uint32_t id,kind,shape;float position[3]{},radius=0,bounds[12]{};};
struct UCCharacterHazards {std::vector<Region> regions;};
static bool finite(const float*p,size_t n){if(!p)return false;for(size_t i=0;i<n;i++)if(!std::isfinite(p[i]))return false;return true;}
int uc_character_hazards_create(const UCLevel*l,UCCharacterHazards**out){
    if(!l||!out||(l->entity_count&&!l->entities))return 1;
    for(uint32_t i=0;i<l->entity_count;i++){const char*n=l->entities[i].class_name;if(!n||!strcmp(n,"SmokeRoom")||!strcmp(n,"SphereCollisionable")||!strcmp(n,"ExpandingSmoke"))return 1;}
    try{*out=new UCCharacterHazards;return 0;}catch(...){return 1;}
}
void uc_character_hazards_close(UCCharacterHazards*s){delete s;}
static int append(UCCharacterHazards*s,const Region&r){if(!s||s->regions.size()>=4096)return 1;try{s->regions.push_back(r);return 0;}catch(...){return 1;}}
int uc_character_hazards_sphere(UCCharacterHazards*s,uint32_t id,const float*p,float radius,uint32_t kind){
    if(!finite(p,3)||!std::isfinite(radius))return 1;Region r{id,kind,0};std::memcpy(r.position,p,12);r.radius=radius;return append(s,r);
}
int uc_character_hazards_box(UCCharacterHazards*s,uint32_t id,const float*b,uint32_t kind){
    if(!finite(b,12))return 1;Region r{id,kind,1};std::memcpy(r.bounds,b,48);return append(s,r);
}
int uc_character_hazards_move_sphere(UCCharacterHazards*s,uint32_t id,const float*p,float radius){
    if(!s||!finite(p,3)||!std::isfinite(radius))return 1;for(auto&r:s->regions)if(r.id==id){std::memcpy(r.position,p,12);r.radius=radius;break;}return 0;
}
int uc_character_hazards_move_box(UCCharacterHazards*s,uint32_t id,const float*b){
    if(!s||!finite(b,12))return 1;for(auto&r:s->regions)if(r.id==id){std::memcpy(r.bounds,b,48);break;}return 0;
}
int uc_character_hazards_remove(UCCharacterHazards*s,uint32_t id){
    if(!s)return 1;auto i=std::find_if(s->regions.begin(),s->regions.end(),[id](const Region&r){return r.id==id;});if(i!=s->regions.end())s->regions.erase(i);return 0;
}
static bool contains(const Region&r,const float*p){
    if(r.shape==1)return p[0]>=r.bounds[0]&&p[0]<=r.bounds[3]&&p[1]>=r.bounds[1]&&p[1]<=r.bounds[4]&&p[2]>=r.bounds[2]&&p[2]<=r.bounds[5];
    float x=p[0]-r.position[0],y=p[1]-r.position[1],z=p[2]-r.position[2];float d=z*z;d=d+y*y;d=d+x*x;return r.radius*r.radius>d;
}
int uc_character_hazards_query(const UCCharacterHazards*s,const float*p,uint32_t kind,uint8_t*hit){
    if(!s||!hit||!finite(p,3))return 1;
    for(const auto&r:s->regions)if(r.kind==kind&&contains(r,p)){*hit=1;return 0;}*hit=0;return 0;
}
static bool segment(const Region&r,const float*from,const float*to){
    float d[3]={to[0]-from[0],to[1]-from[1],to[2]-from[2]};
    if(r.shape==1){
        float lower=-std::numeric_limits<float>::max(),upper=std::numeric_limits<float>::max();
        for(unsigned i=0;i<3;i++){
            if(d[i]>-1.e-8f&&d[i]<1.e-8f){if(r.bounds[i]>from[i]||from[i]>r.bounds[i+3])return false;}
            else{const float inverse=1.f/d[i],a=(r.bounds[i]-from[i])*inverse,b=(r.bounds[i+3]-from[i])*inverse;
                const float lo=b>a?a:b,hi=b>a?b:a;if(lo>lower)lower=lo;if(!(hi>upper))upper=hi;
                if(lower>upper||0>upper)return false;}
        }return !(lower>1.f);
    }
    if(contains(r,from)||contains(r,to))return true;
    float c[3]={r.position[0]-from[0],r.position[1]-from[1],r.position[2]-from[2]};
    float numerator=d[2]*c[2];numerator+=d[1]*c[1];numerator+=d[0]*c[0];
    float denominator=d[2]*d[2];denominator+=d[1]*d[1];denominator+=d[0]*d[0];
    const float t=numerator/denominator;if(!(t>0&&t<1))return false;
    const float closest[3]={d[0]*t+from[0],d[1]*t+from[1],d[2]*t+from[2]};return contains(r,closest);
}
int uc_character_hazards_segment_clear(const UCCharacterHazards*s,const float*from,const float*to,uint32_t kind,uint8_t*clear){
    if(!s||!clear||!finite(from,3)||!finite(to,3))return 1;
    for(const auto&r:s->regions)if(r.kind==kind&&segment(r,from,to)){*clear=0;return 0;}*clear=1;return 0;
}
