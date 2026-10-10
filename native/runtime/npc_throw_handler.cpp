#include "npc_throw_handler.h"
#include <cmath>
#include <stdexcept>
namespace {void need(bool value,const char*text){if(!value)throw std::runtime_error(text);}}
void native_npc_throw_matrix(const float*local,const float*world,float*out){
    need(local&&world&&out,"NPC restock matrix");float result[16];
    for(unsigned r=0;r<4;r++){const float*b=local+4*(r==0?2:r==2?0:r);
        for(unsigned c=0;c<4;c++)result[r*4+c]=(b[0]*world[c]+b[1]*world[c+4])+(b[2]*world[c+8]+b[3]*world[c+12]);}
    for(unsigned i=0;i<16;i++){need(std::isfinite(result[i]),"NPC restock matrix range");out[i]=result[i];}
}
const char *native_npc_throw_bank(int32_t weapon){return weapon==11?"Banks\\Objects\\Molotov.amp":weapon==12?"Banks\\HavokObjects\\PipeBombs.amp":nullptr;}
NativeNpcThrowHandler::NativeNpcThrowHandler(int32_t actor,int32_t kind,const NativeNpcThrowServices&h):services_(h){
    actor_id=actor;selection=kind==11?0:kind==12?1:-1;
    need(selection!=-1,"NPC thrown weapon descriptor");weapon=kind;state=3;
}
bool NativeNpcThrowHandler::lookup(int32_t id,NativeNpcThrownView&out)const{
    need(services_.projectile,"NPC thrown projectile registry");int found=services_.projectile(services_.owner,id,&out);
    need(found>=0,"NPC thrown projectile query");return found!=0;
}
float NativeNpcThrowHandler::fuse()const{NativeNpcThrownView item;return primary!=-1&&lookup(primary,item)?item.fuse:-1.f;}
bool NativeNpcThrowHandler::ready()const{NativeNpcThrownView item;return primary==-1||!lookup(primary,item)||item.fuse<0;}
bool NativeNpcThrowHandler::can_throw(int32_t id,float velocity)const{
    if(selection==-1||velocity<=0)return false;
    need(services_.can_throw,"NPC throw eligibility owner");int result=services_.can_throw(services_.owner,actor_id,id,velocity,selection);
    need(result>=0,"NPC throw eligibility query");return result!=0;
}
void NativeNpcThrowHandler::target(int32_t id,float velocity,float seconds,uint32_t){
    target_id=id;speed=velocity;delay=seconds;target_requested=1;ignite_requested=1;
}
void NativeNpcThrowHandler::throw_point(const float*p,float velocity,uint32_t bits){target_id=-1;for(unsigned i=0;i<3;i++)point[i]=p[i];fixed_target=1;speed=velocity;options=bits;launch_requested=1;}
void NativeNpcThrowHandler::pause_fuses(double delta){
    need(std::isfinite(delta)&&delta>=0,"NPC thrown fuse clock");
    for(auto&r:records)if(r.state==1){NativeNpcThrownView item;if(lookup(r.id,item)){
        need(services_.set_fuse&&!services_.set_fuse(services_.owner,r.id,float(double(item.fuse)+delta)),"NPC thrown fuse pause");}}
}
bool NativeNpcThrowHandler::update_record(Record&r){
    NativeNpcThrownView item;if(!lookup(r.id,item))return false;
    if(r.state==1){if(item.exploded)r.state=4;return true;}
    if(r.state<2||r.state>3)return true;
    if(item.bounced&&r.state==2)r.state=3;
    if(item.exploded){float position[3];need(services_.position,"NPC thrown target position owner");int found=services_.position(services_.owner,r.target,true,position);
        need(found>=0,"NPC thrown target position query");bool close=false;
        if(found){double x=double(position[0])-item.position[0],z=double(position[2])-item.position[2];close=100.>std::sqrt(z*z+x*x);}
        r.state=(r.state!=2?2:0)+(close?5:6);
    }return true;
}
void NativeNpcThrowHandler::restock(bool other){
    int32_t&slot=other?secondary:primary;if(slot!=-1)return;
    float position[3];need(services_.position,"NPC thrown actor registry");int exists=services_.position(services_.owner,actor_id,false,position);
    need(exists>=0,"NPC thrown actor position");if(!exists)return;
    need(services_.restock,"NPC thrown item factory");slot=services_.restock(services_.owner,actor_id,weapon,other);
    need(slot!=-1,"NPC thrown item construction");records.push_front({slot,-1,1});if(!other)enabled=0;
}
void NativeNpcThrowHandler::ignite(bool other){
    if(other){NativeNpcThrownView item;if(lookup(secondary,item)){
        NativeNpcThrownView main;float seconds=lookup(primary,main)?main.fuse:3.f;
        need(services_.ignite&&!services_.ignite(services_.owner,secondary,seconds),"NPC secondary ignition");}return;}
    if(primary==-1)return;NativeNpcThrownView item;
    if(lookup(primary,item)&&!(item.fuse<0))return;
    float position[3];need(services_.position,"NPC thrown target owner");int found=services_.position(services_.owner,target_id,(options&4)!=0,position);
    need(found>=0,"NPC ignition target query");if(!found&&!fixed_target){drop();return;}
    need(lookup(primary,item),"NPC ignition primary lifetime");
    if(found)next=target_id;else for(unsigned i=0;i<3;i++)position[i]=point[i];
    double x=double(position[0])-item.position[0],z=double(position[2])-item.position[2];
    float seconds=float(std::sqrt(z*z+x*x)/speed+delay);
    need(services_.ignite&&!services_.ignite(services_.owner,primary,seconds),"NPC primary ignition");
    need(services_.sound&&!services_.sound(services_.owner,primary,"Ignite2"),"NPC ignition sound");
}
void NativeNpcThrowHandler::drop(){
    if(primary==-1){drop_requested=1;restock(false);}if(primary==-1||enabled)return;
    NativeNpcThrownView item;if(!lookup(primary,item))return;
    need(services_.drop&&!services_.drop(services_.owner,primary),"NPC thrown drop");
    if(lookup(secondary,item))need(!services_.drop(services_.owner,secondary),"NPC secondary drop");
    need(!records.empty(),"NPC dropped projectile record");if(records.front().state==1){records.front().target=-1;records.front().state=2;}
    enabled=1;launch_requested=target_requested=drop_requested=0;
}
void NativeNpcThrowHandler::release(){
    NativeNpcThrownView item;if(primary==-1||!lookup(primary,item)||item.fuse<0)return;
    float position[3];need(services_.position,"NPC thrown target owner");int found=services_.position(services_.owner,target_id,(options&4)!=0,position);
    need(found>=0,"NPC throw target query");if(!found&&!fixed_target){drop();return;}
    if(found)next=target_id;else for(unsigned i=0;i<3;i++)position[i]=point[i];
    need(services_.launch&&!services_.launch(services_.owner,primary,position,options,speed,selection),"NPC thrown launch");
    need(!records.empty(),"NPC launched projectile record");records.front().target=target_id;records.front().state=2;primary=-1;enabled=1;
}
void NativeNpcThrowHandler::update(){
    if(selection==-1)return;NativeNpcThrownView item;if(primary!=-1&&!lookup(primary,item))primary=-1;
    for(auto i=records.begin();i!=records.end();)if(!update_record(*i))i=records.erase(i);else ++i;
    if(!records.empty()&&records.front().state==4){primary=-1;enabled=1;}
    for(unsigned other=0;other<2;other++)if(flags[other?2:0]){
        need(services_.incapacitated,"NPC restock actor property");int busy=services_.incapacitated(services_.owner,actor_id);need(busy>=0,"NPC incapacitated query");if(!busy)restock(other!=0);}
    if(flags[1]||ignite_requested){ignite(false);ignite_requested=0;}
    if(flags[3]){ignite(true);ignite_requested=0;}
    if(flags[4])release();
    if(flags[6]&&lookup(primary,item))need(services_.sound&&!services_.sound(services_.owner,primary,"Ignite1"),"NPC lighter sound");
}
void NativeNpcThrowHandler::close(){
    selection=-1;records.clear();if(primary!=-1){NativeNpcThrownView item;if(lookup(primary,item)){
        need(services_.remove&&!services_.remove(services_.owner,primary),"NPC thrown primary retirement");}
        primary=state=weapon=-1;enabled=0;}
}
