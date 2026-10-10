#include "entity_names.h"
#include <map>
#include <vector>
#include <cstring>
#include <algorithm>
struct Less {bool operator()(const char*a,const char*b)const{return std::strcmp(a,b)<0;}};
struct UCEntityNames {std::map<const char*,std::vector<int32_t>,Less> names;};
static bool valid(const char*p){return p&&strnlen_s(p,1024)<1024;}
int uc_entity_names_create(UCEntityNames**out){if(!out)return 1;try{*out=new UCEntityNames;return 0;}catch(...){return 1;}}
int uc_entity_names_bind(UCEntityNames*s,const char*name,int32_t id){
    if(!s||!valid(name))return 1;
    try{
        auto it=s->names.find(name);if(it==s->names.end()){if(s->names.size()>=4096)return 1;it=s->names.try_emplace(name).first;}
        auto&ids=it->second;if(std::find(ids.begin(),ids.end(),id)!=ids.end())return 0;
        if(ids.size()>=65536)return 1;ids.push_back(id);return 0;
    }catch(...){return 1;}
}
int uc_entity_names_view(const UCEntityNames*s,const char*name,const int32_t**ids,uint32_t*count){
    if(!s||!valid(name)||!ids||!count)return 1;auto it=s->names.find(name);
    *ids=it==s->names.end()?nullptr:it->second.data();*count=it==s->names.end()?0:static_cast<uint32_t>(it->second.size());return 0;
}
void uc_entity_names_close(UCEntityNames*s){delete s;}
int uc_entity_names_remove(UCEntityNames*s,const char*name,int32_t id){
    if(!s||!valid(name))return 1;
    auto it=s->names.find(name);if(it==s->names.end())return 0;
    auto&ids=it->second;auto found=std::find(ids.begin(),ids.end(),id);
    if(found!=ids.end())ids.erase(found);return 0;
}
