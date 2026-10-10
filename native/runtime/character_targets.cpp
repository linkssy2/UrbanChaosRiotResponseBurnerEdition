#include "character_targets.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {
int checked(int n){if(n<0)throw std::runtime_error("NPC target service unsupported");return n;}
bool resolve(const NativeCharacterTargetServices &s,int32_t id,float p[3]){
    if(!s.resolve)throw std::runtime_error("NPC target registry owner missing");return checked(s.resolve(s.owner,id,p))!=0;}
bool property(const NativeCharacterTargetServices &s,int32_t id,const char *name,int32_t &v){
    if(!s.property)throw std::runtime_error("NPC target property owner missing");return checked(s.property(s.owner,id,name,&v))!=0;}
bool visible(const NativeCharacterTargetServices &s,int32_t id){
    if(!s.visible)throw std::runtime_error("NPC target visibility owner missing");return checked(s.visible(s.owner,id))!=0;}
int32_t player(const NativeCharacterTargetServices &s){
    if(!s.player)throw std::runtime_error("NPC target player owner missing");return s.player(s.owner);}
bool contains(const std::vector<int32_t>&v,int32_t n){return std::find(v.begin(),v.end(),n)!=v.end();}
void say(const NativeCharacterTargetServices&s,uint32_t event,unsigned priority){
    if(!s.dialogue||s.dialogue(s.owner,event,priority))throw std::runtime_error("NPC target dialogue owner failed");}
}
int32_t NativeCharacterFactions::key(int32_t id){
    if(id<0)return -1;
    if(id<100){const auto n=id%10;return n<4?0x38+n*4:n>=5?0x14+n*4:-1;}
    if(id>=200&&id<=203)return 0x28+(id-200)*4;
    switch(id){case 100:return 0x18;case 110:return 0x14;case 120:return 0x38;
    case 130:return 0x1c;case 160:return 0x20;case 190:return 0x24;default:return -1;}
}
NativeCharacterFactions::Group &NativeCharacterFactions::group(int32_t id){
    auto i=groups_.find(key(id));if(i==groups_.end())throw std::runtime_error("Unsupported authored NPC faction");return i->second;}
const NativeCharacterFactions::Group &NativeCharacterFactions::group(int32_t id)const{
    auto i=groups_.find(key(id));if(i==groups_.end())throw std::runtime_error("Unsupported authored NPC faction");return i->second;}
NativeCharacterFactions::NativeCharacterFactions(){
    for(auto k:{0x10,0x5c,0x14,0x18,0x1c,0x20,0x24,0x28,0x2c,0x30,0x34,0x38,0x3c,0x40,0x44})groups_[k];
    for(auto k:{0x14,0x18,0x1c,0x20,0x24})for(auto other:{0x14,0x18,0x1c,0x20,0x24})if(k!=other)groups_[k].friends.push_back(other);
    groups_[0x14].friends.push_back(0x28);groups_[0x14].friends.push_back(0x30);
    for(auto k:{0x38,0x3c,0x40,0x44})for(auto other:{0x38,0x3c,0x40,0x44})if(k!=other)groups_[k].friends.push_back(other);
    groups_[0x28].enemies={0x2c};groups_[0x2c].enemies={0x28};
    groups_[0x30].enemies={0x34};groups_[0x34].enemies={0x30};
    groups_[0x14].enemies={0x2c,0x34,0x38,0x3c,0x40};
    for(auto k:{0x18,0x1c,0x20})groups_[k].enemies={0x38,0x3c,0x40};
    groups_[0x38].enemies={0x14};groups_[0x3c].enemies={0x14,0x18};
    groups_[0x40].enemies={0x14,0x18,0x1c,0x20};groups_[0x44].enemies={0x14};
}
void NativeCharacterFactions::add(int32_t faction,int32_t actor){group(faction).members.push_back(actor);}
void NativeCharacterFactions::remove(int32_t faction,int32_t actor){auto &m=group(faction).members;auto i=std::find(m.begin(),m.end(),actor);if(i!=m.end())m.erase(i);}
bool NativeCharacterFactions::opponent(int32_t faction,int32_t actor)const{
    const auto&g=group(faction);if(contains(g.members,actor))return false;
    for(auto k:g.enemies)if(contains(groups_.at(k).members,actor))return true;return false;
}
bool NativeCharacterFactions::friendly(int32_t faction,int32_t actor)const{
    const auto&g=group(faction);if(contains(g.members,actor))return true;
    for(auto k:g.friends)if(contains(groups_.at(k).members,actor))return true;return false;
}
const std::vector<int32_t>&NativeCharacterFactions::opponents(int32_t faction)const{return group(faction).opponents;}
void NativeCharacterFactions::refresh(const NativeCharacterTargetServices&s){
    // Original construction list order, which differs from numeric map order.
    for(auto k:{0x10,0x5c,0x14,0x18,0x1c,0x20,0x24,0x28,0x2c,0x30,0x34,0x38,0x3c,0x40,0x44}){
        auto &g=groups_.at(k);g.opponents.clear();if(g.members.empty())continue;
        for(auto enemy:g.enemies)for(auto id:groups_.at(enemy).members){float p[3];int32_t v=0;
            if(resolve(s,id,p)&&(!property(s,id,"incapacitated",v)||!uint8_t(v)))g.opponents.push_back(id);}
        g.attackers.clear();for(auto id:g.members){int32_t target=0;float p[3];
            if(property(s,id,"attacking",target)&&target!=-1&&resolve(s,target,p))g.attackers[target].push_back(id);}
    }
}
int32_t NativeCharacterFactions::attackers(int32_t faction,int32_t actor,const NativeCharacterTargetServices&s)const{
    int32_t target=0;float p[3];if(!property(s,actor,"attacking",target)||target==-1||!resolve(s,target,p))return 0;
    const auto&v=group(faction).attackers;auto i=v.find(target);return i==v.end()?0:static_cast<int32_t>(i->second.size());
}
int32_t NativeCharacterFactions::attacker(int32_t faction,int32_t actor,uint32_t index,const NativeCharacterTargetServices&s)const{
    //39BC0 repeats the live attacking-target query; stale cached actors remain
    //visible until the faction manager's normal refresh boundary.
    int32_t target=0;float p[3];if(!property(s,actor,"attacking",target)||target==-1||!resolve(s,target,p))return -1;
    const auto&v=group(faction).attackers;auto i=v.find(target);
    if(i==v.end()||index>=i->second.size())throw std::runtime_error("Faction attacker index invariant");return i->second[index];
}
bool native_character_target_valid(const NativeCharacterTargetState&t,const NativeCharacterTargetActor&a,
    const NativeCharacterFactions&f,const NativeCharacterTargetServices&s,int32_t id){float p[3];int32_t v=0;
        if(id==-1||!resolve(s,id,p))return false;
        if(id!=t.forced&&a.faction!=-1&&!f.opponent(a.faction,id))return false;
        return t.allow_incapacitated||!property(s,id,"incapacitated",v)||!uint8_t(v);
}
int32_t native_character_current_target(NativeCharacterTargetState&t,const NativeCharacterTargetActor&a,
    const NativeCharacterFactions&f,const NativeCharacterTargetServices&s){
    if(!native_character_target_valid(t,a,f,s,t.current))t.current=-1;return t.current;
}
int32_t native_character_select_target(const NativeCharacterTargetState&t,const NativeCharacterTargetActor&a,
    const NativeCharacterFactions&f,const NativeCharacterTargetServices&s,bool prefer,bool check){
    auto eligible=[&](int32_t id){return native_character_target_valid(t,a,f,s,id);};
    if(eligible(t.forced))return t.forced;
    if(eligible(t.preferred)&&(!check||visible(s,t.preferred)))return t.preferred;
    if(!prefer&&eligible(t.current))return t.current;
    if(a.faction==-1)return player(s);
    std::vector<int32_t> candidates;
    for(auto id:f.opponents(a.faction))if((!prefer||id!=t.current)&&eligible(id))candidates.push_back(id);
    if(candidates.empty())return prefer&&eligible(t.current)?t.current:-1;
    if(candidates.size()==1)return check&&!visible(s,candidates[0])?t.current:candidates[0];
    std::vector<int32_t> seen;for(auto id:candidates)if(visible(s,id))seen.push_back(id);
    if(seen.size()==1)return seen[0];
    const bool front=!seen.empty();if(front)candidates=std::move(seen);else if(check)return t.current;
    std::vector<int32_t> ranges[6];
    const float forward_sum=a.forward[2]*a.forward[2]+a.forward[0]*a.forward[0];
    const float inv_forward=static_cast<float>(1.0/std::sqrt(static_cast<double>(forward_sum)));
    const float fx=inv_forward*a.forward[0],fz=inv_forward*a.forward[2],fy=inv_forward*0.0f;
    for(auto id:candidates){float p[3];if(!resolve(s,id,p))continue;int32_t avoid=0;
        const bool avoiding=property(s,id,"avoid targetting",avoid)&&uint8_t(avoid);
        const float dx=p[0]-a.position[0],dz=p[2]-a.position[2];
        const float sum=dz*dz+dx*dx;float distance=static_cast<float>(std::sqrt(static_cast<double>(sum)));
        if(avoiding)distance+=1800.0f;
        const double inv=1.0/std::sqrt(static_cast<double>(sum));
        const float nx=static_cast<float>(dx*inv),ny=static_cast<float>(0.0*inv),nz=static_cast<float>(dz*inv);
        const float dot=(ny*fy+nz*fz)+nx*fx;
        const bool preferred=!avoiding&&(!front||dot>0.0f);
        //9C,50,A8,90,38,B4: source checks the first five; B4 is only
        //retained as the original list when no earlier bucket was populated.
        const unsigned bucket=preferred?(distance<700?0:distance>1800?3:1):(distance<700?2:distance>1800?5:4);
        ranges[bucket].push_back(id);
    }
    for(unsigned i=0;i<5;++i)if(!ranges[i].empty()){if(ranges[i].size()==1)return ranges[i][0];candidates=std::move(ranges[i]);break;}
    std::vector<int32_t> weapons[3];
    for(auto id:candidates){int32_t kind=0;if(property(s,id,"weapon kind",kind)){
        const int index=kind==0?0:kind==2?1:kind==3?2:-1;if(index>=0)weapons[index].push_back(id);}}
    for(auto &v:weapons)if(!v.empty()){if(v.size()==1)return v[0];candidates=std::move(v);break;}
    std::vector<int32_t> least;int32_t minimum=-1;bool has_player=false;
    for(auto id:candidates){if(id==player(s))has_player=true;int32_t n=0;
        if(property(s,id,"number of attackers",n)){
            if(minimum==-1||minimum>n){least.clear();minimum=n;least.push_back(id);}
        }else if(minimum==-1)least.push_back(id);
    }
    if(has_player)return player(s);
    if(!least.empty())candidates=std::move(least);
    if(candidates.empty())throw std::runtime_error("Original NPC target candidate invariant");
    return candidates[0];
}
bool native_character_acquire_target(NativeCharacterTargetState&t,const NativeCharacterTargetActor&a,
    const NativeCharacterFactions&f,const NativeCharacterTargetServices&s,bool prefer,bool check){
    int32_t selected=native_character_select_target(t,a,f,s,prefer,check);const int32_t pc=player(s);bool emitted=false;
    if(selected==pc||selected==t.forced){if(visible(s,pc))t.player_seen=2.0f;}
    else if(a.faction==-1||f.opponent(a.faction,pc)){
        float pp[3],tp[3];const bool pl=resolve(s,pc,pp),tl=resolve(s,selected,tp);
        if(pl&&tl){auto distance=[&](const float p[3]){const float x=p[0]-a.position[0],y=p[1]-a.position[1],z=p[2]-a.position[2];
                const float sum=(x*x+y*y)+z*z;return static_cast<float>(std::sqrt(static_cast<double>(sum)));};
            const float pd=distance(pp),td=distance(tp);
            if(pd<500.0f||pd<td*0.5f){if(visible(s,pc))t.player_seen=2.0f;
                if(t.player_seen>0){selected=pc;if(pc!=t.current){say(s,0x80001100,25);emitted=true;}}}}
    }
    const bool changed=selected!=t.current;
    if(changed&&!emitted){float p[3];if(resolve(s,selected,p)){
        if(a.faction==-1)throw std::runtime_error("Original target dialogue requires faction owner");
        if(f.attackers(a.faction,a.id,s)>0){const float dy=p[1]-a.position[1];say(s,dy>500?0x80004b00:dy< -500?0x80004c00:0x80004300,dy>500||dy< -500?50:25);}
        else say(s,0x80001200,25);}}
    t.current=selected;return changed;
}
