#include "character_cover.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
namespace {
void require(bool v,const char*m){if(!v)throw std::runtime_error(m);}
float planar(const float*a,const float*b,float&x,float&z){
    x=float(double(a[0])-b[0]);double dz=double(a[2])-b[2];z=float(dz);
    return float(std::sqrt(dz*z+double(x)*x)); //1BA10/1BC10 keep z subtraction live
}
}
NativeCharacterCoverPoints::NativeCharacterCoverPoints(const UCLevelLogic&l):logic_(l),nodes_(l.node_count){require(l.node_count&&l.nodes,"Cover graph missing");}
int32_t NativeCharacterCoverPoints::add(NativeMissionCoverPoint&p){
    require(!find(p.entity.id),"Duplicate CoverPoint");int32_t selected=0;float best=0;
    for(uint32_t i=0;i<logic_.node_count;i++){const float*a=logic_.nodes[i].position,*b=p.entity.transform+12;
        float x=a[0]-b[0],y=a[1]-b[1],z=a[2]-b[2],d=(z*z+y*y)+x*x;
        require(std::isfinite(d),"Cover node coordinates");if(!i||d<best){best=d;selected=int32_t(i);}}
    nodes_[selected].push_back(&p);return selected;
}
void NativeCharacterCoverPoints::remove(NativeMissionCoverPoint&p){
    if(p.node()<0)return;auto&list=nodes_.at(size_t(p.node()));auto at=std::find(list.begin(),list.end(),&p);if(at!=list.end())list.erase(at);
}
NativeMissionCoverPoint*NativeCharacterCoverPoints::find(int32_t id)const{
    for(const auto&list:nodes_)for(auto*p:list)if(p->entity.id==id)return p;return nullptr;
}
NativeMissionCoverPoint::NativeMissionCoverPoint(){entity.view.actor=nullptr;entity.view.aux84=nullptr;}
int NativeMissionCoverPoint::construct(NativeMissionWorld&w,uint32_t source,NativeCharacterCoverPoints&points){
    if(world_||source>=w.source_count())return 1;const auto&m=w.metadata(source);const auto*d=m.definition;
    if(!d||std::strcmp(d->class_name,"CoverPoint")||m.asset_present||*w.active_descriptor()!=&m)return 1;
    const uint8_t*cursor=*w.property_cursor(source);float values[3];uint8_t crouching=1;int found;
    for(float&v:values)if(uc_entity_stream_get(d,&cursor,2,&v,&found)||!found||!std::isfinite(v))return 1;
    if(uc_entity_stream_get(d,&cursor,3,&crouching,&found)||!found||!crouching)return 1; //standing20FE90 not yet owned
    world_=&w;points_=&points;radius_=values[0];height_=values[1];fov_=values[2];crouching_=crouching!=0;
    if(uc_entity_base_construct_authored(&w.context(),&entity.view,w.active_descriptor()))return failure_=2;
    *w.property_cursor(source)=cursor;
    cosine_=float(std::cos(double(fov_)*.5*double(0.01745329238474369f)));
    node_=points.add(*this);left_=right_=false;
    if(w.retirement().remove_update_record(w.retirement().owner,entity.id))return failure_=2;
    forward_[0]=entity.transform[8];forward_[1]=0;forward_[2]=entity.transform[10];
    double length=std::sqrt(double(forward_[2])*forward_[2]+double(forward_[0])*forward_[0]);
    if(!(length>0))return failure_=2;double inverse=1./length;for(float&v:forward_)v=float(double(v)*inverse);
    if(uc_entity_subscriptions_create(this,&subscriptions_))return failure_=2;int inserted=0;
    if(uc_entity_listener_register(&w.registry(),entity.id,entity.id,uc_entity_subscriptions_listener(subscriptions_),w.listeners(),&inserted)||!inserted)return failure_=2;
    UCEntityEventSubscription callbacks[]={
        {31,[](void*p,const void*,uint32_t*){return static_cast<NativeMissionCoverPoint*>(p)->failure_=2;}},
        {30,[](void*p,const void*,uint32_t*){return static_cast<NativeMissionCoverPoint*>(p)->failure_=2;}},
        {32,[](void*p,const void*v,uint32_t*){auto&s=*static_cast<NativeMissionCoverPoint*>(p);if(!v)return s.failure_=2;std::memcpy(&s.occupant_,v,4);return 0;}},
        {36,[](void*p,const void*,uint32_t*){static_cast<NativeMissionCoverPoint*>(p)->disabled_=true;return 0;}},
        {1,[](void*p,const void*,uint32_t*){static_cast<NativeMissionCoverPoint*>(p)->disabled_=false;return 0;}},
        {9,[](void*p,const void*v,uint32_t*){auto&s=*static_cast<NativeMissionCoverPoint*>(p);if(!v)return s.failure_=2;auto*a=static_cast<const float*>(v);auto*b=s.entity.transform+12;
            float x=a[0]-b[0],y=a[1]-b[1],z=a[2]-b[2];if((z*z+x*x)+y*y<40000)s.danger_until_=s.world_->time()+5.f;return 0;}}};
    for(const auto&c:callbacks)if(uc_entity_subscriptions_append(subscriptions_,&c))return failure_=2;return 0;
}
int NativeMissionCoverPoint::property(const char*name,int32_t*out)const{
    if(!name||!out||failure_)return -1;
    if(!std::strcmp(name,"occupant"))*out=occupant_;
    else if(!std::strcmp(name,"can stand and shoot"))*out=crouching_;
    else if(!std::strcmp(name,"can lean left"))*out=left_;
    else if(!std::strcmp(name,"can lean right"))*out=right_;
    else return 0;return 1;
}
int NativeMissionCoverPoint::reserve(int32_t actor,int32_t sender){
    if(!world_||failure_)return 1;uint8_t handled=0;uint32_t id=uint32_t(sender);
    return uc_entity_event_send(&world_->registry(),entity.id,32,&actor,4,&id,&handled)||!handled?2:0;
}
int NativeMissionCoverPoint::close_detached(){
    if(!world_)return 0;if(uc_entity_event_lookup(&world_->registry(),entity.id))return 1;
    points_->remove(*this);node_=-1;
    if(entity.identity.listeners[0]&&uc_entity_listener_list_remove_first(world_->listeners(),entity.id))return 1;
    for(auto&l:entity.identity.listeners)l=nullptr;uc_entity_subscriptions_close(subscriptions_);subscriptions_=nullptr;world_=nullptr;return 0;
}
NativeCharacterCoverSearch::NativeCharacterCoverSearch(NativeCharacterCoverPoints&p,NativeCharacterNavigationManager&m,
    int32_t actor,const float*origin,const float*direction):points_(p),manager_(m),actor_(actor){
    require(origin&&direction&&(!p.logic().trigger_count||p.logic().triggers)&&actor>0,"Cover search inputs");
    for(unsigned i=0;i<3;i++){require(std::isfinite(origin[i])&&std::isfinite(direction[i]),"Cover search coordinates");origin_[i]=origin[i];direction_[i]=direction[i];}
    direction_[1]=0;double length=std::sqrt((double(direction_[2])*direction_[2]+double(direction_[1])*direction_[1])+double(direction_[0])*direction_[0]);
    require(length>0,"Cover search heading");double inverse=1./length;for(float&v:direction_)v=float(double(v)*inverse);
    job_.owner=this;job_.initialize=[](auto&j){static_cast<NativeCharacterCoverSearch*>(j.owner)->initialize();};
    job_.step=[](auto&j){static_cast<NativeCharacterCoverSearch*>(j.owner)->step();};
    job_.cancel=[](auto&j){auto&s=*static_cast<NativeCharacterCoverSearch*>(j.owner);s.status_=3;s.open_.clear();j.complete=true;};
    manager_.search_manager().add(job_);
}
NativeCharacterCoverSearch::~NativeCharacterCoverSearch(){manager_.search_manager().remove(job_);}
int32_t NativeCharacterCoverSearch::record(int32_t node,int32_t cover,int32_t parent){
    require(node==-1||(node>=0&&uint32_t(node)<points_.logic().node_count),"Cover neighbor outside authored graph");
    for(size_t i=0;i<records_.size();i++)if(node!=-1?records_[i].node==node:records_[i].cover==cover)return int32_t(i);
    require(records_.size()<1000000,"Cover search record limit");records_.push_back({node,cover,parent});return int32_t(records_.size()-1);
}
const float*NativeCharacterCoverSearch::position(int32_t i)const{
    const auto&r=records_.at(size_t(i));if(r.node!=-1)return points_.logic().nodes[r.node].position;
    auto*p=points_.find(r.cover);require(p!=nullptr,"Cover removed during original search");return p->entity.transform+12;
}
void NativeCharacterCoverSearch::enqueue(int32_t id){
    auto at=open_.begin();while(at!=open_.end()&&!(records_[*at].score>records_[id].score))++at;
    //Revisiting is false in1BF90; no record can be enqueued twice.
    require(records_[id].state==0,"Cover duplicate open record");open_.insert(at,id);records_[id].state=1;
}
void NativeCharacterCoverSearch::initialize(){
    status_=1;auto q=manager_.query(actor_);int32_t node=-1;
    require(!uc_character_navigation_endpoint(&points_.logic(),origin_.data(),q.region,q.ray,q.owner,&node),"Cover initial node query");
    if(node<0){status_=3;job_.complete=true;return;}
    int32_t id=record(node,-1,-1);float x,z,d=planar(position(id),origin_.data(),x,z),dot=1;
    if(d>0){float inv=1.f/d;dot=(direction_[2]*(z*inv)+direction_[1]*(inv*0.f))+direction_[0]*(x*inv);}
    records_[id].score=float(((1.-dot)*.5+double(.1f))*d);enqueue(id);
}
bool NativeCharacterCoverSearch::admit(int32_t i)const{
    const auto&r=records_[i];if(r.node!=-1){uint32_t key=points_.logic().nodes[r.node].key&0xFFF0FFFFu;if(key>=points_.logic().trigger_count||!(points_.logic().triggers[key].flags&16))return false;}
    auto*p=position(i);double x=double(p[0])-origin_[0],z=double(p[2])-origin_[2];return std::sqrt(z*z+x*x)<=1000.;
}
bool NativeCharacterCoverSearch::terminal(int32_t i)const{
    const auto&r=records_[i];if(r.node!=-1)return false;float x,z,d=planar(position(i),origin_.data(),x,z);if(d>1000.f)return false;
    //Original comiss admits unordered zero-distance direction; preserve it.
    float inv=1.f/d,dot=(direction_[2]*(z*inv)+direction_[1]*(inv*0.f))+direction_[0]*(x*inv);
    if(0.7070000171661377f>dot)return false;auto*p=points_.find(r.cover);return p&&p->occupant()==-1;
}
void NativeCharacterCoverSearch::step(){
    if(job_.complete)return;status_=2;if(open_.empty()){status_=3;job_.complete=true;return;}
    int32_t current=open_.front();open_.erase(open_.begin());records_[current].state=2;
    if(terminal(current)){result_=records_[current].cover;status_=4;job_.complete=true;return;}
    int32_t node=records_[current].node;if(node<0)return;
    auto push=[&](int32_t n,int32_t c){int32_t id=record(n,c,current);if(!records_[id].state&&admit(id)){records_[id].parent=current;enqueue(id);}};
    for(auto*p:points_.at(uint32_t(node)))push(-1,p->entity.id);
    const auto&n=points_.logic().nodes[node];for(uint32_t i=0;i<n.first.count;i++){if(n.first.data[i]==UINT32_MAX)return;push(int32_t(n.first.data[i]),-1);}
    for(uint32_t i=0;i<n.second.count;i++){if(n.second.data[i]==UINT32_MAX)return;push(int32_t(n.second.data[i]),-1);}
}
