#include "character_navigation.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {void require(bool ok,const char *s){if(!ok)throw std::runtime_error(s);}}
void NativeCharacterSearchManager::insert(std::list<Job *>&list,Job&job){
    auto at=list.begin();while(at!=list.end()&&job.priority<(*at)->priority)++at;list.insert(at,&job);
}
void NativeCharacterSearchManager::add(Job&job){
    require(!job.scheduled&&std::isfinite(job.priority)&&job.initialize&&job.step,"Search job registration");
    insert(pending_,job);job.scheduled=true;
}
void NativeCharacterSearchManager::remove(Job&job){
    auto active=std::find(active_.begin(),active_.end(),&job);
    if(active!=active_.end()){active_.erase(active);if(job.cancel)job.cancel(job);}
    pending_.remove(&job);completed_.remove(&job);job.scheduled=false;
}
void NativeCharacterSearchManager::update(){
    while(active_.size()<20&&!pending_.empty()){
        Job&job=*pending_.front();pending_.pop_front();job.initialize(job);
        if(job.complete)completed_.push_front(&job);else insert(active_,job);
    }
    auto at=active_.begin();
    for(int budget=10;budget>0&&!active_.empty();--budget){
        Job&job=**at;job.step(job);
        if(job.complete){at=active_.erase(at);completed_.push_front(&job);}else ++at;
        if(at==active_.end())at=active_.begin();
    }
}
uint8_t NativeCharacterDoors::check(int32_t target,int32_t sender,const float actor_position[3])const{
    require(target>0&&sender>0&&actor_position&&property&&position&&open,"Navigation door services");
    if(!property(owner,target,"is a door",0)||!property(owner,target,"npc can open",0)||
        !property(owner,target,"is closed",0)||property(owner,target,"is locked",1))return 0;
    const float*p=position(owner,target);require(p!=nullptr,"Navigation door position");
    float d[3];for(unsigned j=0;j<3;j++){require(std::isfinite(p[j])&&std::isfinite(actor_position[j]),"Navigation door coordinates");d[j]=actor_position[j]-p[j];}
    float square=(d[2]*d[2]+d[1]*d[1])+d[0]*d[0];if(square<40000)open(owner,target,sender);return 1;
}
void NativeCharacterDoors::ahead(int32_t sender,const float from[3],const float waypoint[3])const{
    require(sender>0&&from&&waypoint&&simple_door&&cast_template,"Navigation forward door services");
    int32_t type=simple_door(owner);if(!type)return;
    float direction[3],origin[3];for(unsigned j=0;j<3;j++){require(std::isfinite(from[j])&&std::isfinite(waypoint[j]),"Navigation door ray coordinates");direction[j]=waypoint[j]-from[j];origin[j]=from[j];}
    origin[1]+=50;int32_t target=cast_template(owner,origin,direction,type);if(target)check(target,sender,from);
}
namespace {
struct CharacterWorldHit {
    UCCollisionRay ray{};UCCollisionClosest closest{};UCCollisionResultOwner owner{};
    uint32_t kind=0;int32_t entity=0;const void *actor=nullptr;
};
int character_world_cast(const NativeCharacterWorld&w,const float*from,const float*to,
    uint32_t region,uint32_t shape_mask,uint32_t type_mask,CharacterWorldHit&out){
    if(!w.scenery)return -1;
    int ready=uc_collision_ray_prepare(from,to,shape_mask,region,type_mask,&out.ray);if(ready<0)return -1;
    auto&ray=out.ray;int found=0;float limit=1;
    if(ready){
        UCCollisionBodyFilter scenery{24,w.scenery->shape_user_data,0x02F00000,1,0,0,0};
        if(uc_collision_body_filter(ray.filter,&scenery)==1){
            UCCollisionRayHit contact;int status=uc_collision_raycast(w.scenery,ray.from,ray.to,&contact);if(status<0)return -1;
            if(status){
                if(contact.triangle>=w.scenery->triangle_count)return -1;
                found=1;limit=contact.fraction;out.closest.hit.fraction=limit;
                std::copy_n(contact.normal,3,out.closest.hit.normal);out.closest.hit.normal[3]=0;
                out.closest.material=w.scenery->triangles[contact.triangle].material;
                out.closest.filter_info=scenery.filter_info;out.owner=w.scenery_owner;out.kind=1;
            }
        }
        for(const auto&b:w.boxes){
            if(!b.geometry||b.entity<=0)return -1;
            int admitted=uc_collision_body_filter(ray.filter,&b.filter);if(admitted<0)return -1;if(!admitted)continue;
            UCCollisionTriangleHit contact;int status=uc_solid_block_cast(b.geometry,ray.from,ray.to,limit,&contact);if(status<0)return -1;
            if(status&&(!found||contact.fraction<limit)){
                found=1;limit=contact.fraction;out.entity=b.entity;out.actor=b.actor;out.closest.hit=contact;
                //2A8D00's nontriangle branch uses FINAL ROOT userdata, also
                //for an aggregate list root. It does not select child userdata.
                out.closest.material=b.filter.shape_user_data;out.closest.filter_info=b.filter.filter_info;
                out.owner=b.result_owner;out.kind=b.filter.collidable_type;
            }
        }
        for(const auto&b:w.capsules){
            if(!b.geometry||b.entity<=0)return -1;
            int admitted=uc_collision_body_filter(ray.filter,&b.filter);if(admitted<0)return -1;if(!admitted)continue;
            UCCollisionTriangleHit contact;int status=uc_character_capsule_cast(b.geometry,ray.from,ray.to,limit,&contact);if(status<0)return -1;
            if(status&&(!found||contact.fraction<limit)){
                found=1;limit=contact.fraction;out.entity=b.entity;out.actor=b.actor;out.closest.hit=contact;
                out.closest.material=b.filter.shape_user_data;out.closest.filter_info=b.filter.filter_info;
                out.owner=b.result_owner;out.kind=b.filter.collidable_type;
            }
        }
    }
    out.closest.limit=limit;out.closest.root_body=out.owner.identity;return found;
}
}
int NativeCharacterWorld::ray(void *owner,const float from[3],const float to[3],uint32_t region,
    uint32_t shape_mask,uint32_t type_mask,uint8_t *hit,int32_t *entity){
    if(!owner||!hit||!entity)return 1;CharacterWorldHit result;
    int status=character_world_cast(*static_cast<NativeCharacterWorld*>(owner),from,to,region,shape_mask,type_mask,result);
    if(status<0)return 1;*hit=(uint8_t)status;*entity=result.entity;return 0;
}
int NativeCharacterWorld::query(const float from[3],const float to[3],uint32_t type,uint32_t region,
    uint32_t mask,float impulse,UCCollisionResult&result,int32_t&entity,const void**actor)const{
    if(!std::isfinite(impulse))return -1;
    CharacterWorldHit selected;int status=character_world_cast(*this,from,to,region,type,mask,selected);if(status<=0)return status;
    if(!selected.owner.identity||selected.owner.kind!=selected.kind||selected.owner.property45)return -1;
    const bool dynamic=selected.kind==1&&impulse!=0&&!selected.owner.impulse_blocked;
    if(selected.kind==1&&impulse!=0&&selected.owner.property20)return -1;
    if(dynamic&&!apply_impulse)return -1;
    UCCollisionResult converted;status=uc_collision_result_convert(&selected.ray,&selected.closest,&selected.owner,dynamic?0:impulse,&converted);
    if(status!=1)return -1;
    if(dynamic){
        float momentum[3],square=0;
        for(unsigned j=0;j<3;j++){momentum[j]=selected.ray.to[j]-selected.ray.from[j];square+=momentum[j]*momentum[j];}
        if(!(square>0)||!std::isfinite(square))return -1;
        const float scale=impulse/std::sqrt(square);
        for(float&v:momentum){v*=scale;if(!std::isfinite(v))return -1;}
        if(apply_impulse(impulse_owner,selected.entity,momentum))return -1;
    }
    result=converted;entity=selected.entity;if(actor)*actor=selected.actor;return 1;
}
int32_t NativeCharacterRoute::edge(int32_t from,int32_t to,int32_t parent){
    require(to>=0&&(uint32_t)to<logic_.node_count,"Navigation neighbor outside authored graph");
    uint64_t key=(uint64_t)(uint32_t)from<<32|(uint32_t)to;
    auto old=edge_ids_.find(key);if(old!=edge_ids_.end())return old->second;
    require(count_<records_.size(),"Navigation directed-edge capacity");
    int32_t id=(int32_t)count_++;edges_.push_back({from,to});records_[id].parent=parent;edge_ids_.emplace(key,id);return id;
}
double NativeCharacterRoute::heuristic(int32_t id)const{
    const auto *a=logic_.nodes[goal_].position,*b=logic_.nodes[edges_[id].to].position;
    double x=(double)a[0]-b[0],y=(double)a[1]-b[1],z=(double)a[2]-b[2];return sqrt((z*z+y*y)+x*x);
}
void NativeCharacterRoute::enqueue(int32_t id){
    // ponytail: preserve19290's mutable-score list and equal-score duplicates;
    // use a more complex queue only after a profile and parity proof justify it.
    require(open_.size()<1000000,"Navigation open-list limit");uint32_t count=(uint32_t)open_.size();open_.resize(count+1);
    require(!uc_character_search_enqueue(records_.data(),count_,id,open_.data(),&count,(uint32_t)open_.size()),"Navigation queue insertion");open_.resize(count);
}
void NativeCharacterRoute::finish(int32_t id){
    if(id<0){status_=3;return;}
    path.push_back({to_,-1});
    for(uint32_t seen=0;id!=-1;id=records_[id].parent){
        require(id>=0&&(uint32_t)id<count_&&seen++<count_,"Navigation parent cycle");
        int32_t node=edges_[id].to;const float*p=logic_.nodes[node].position;path.push_back({{p[0],p[1],p[2]},node});
    }
    path.push_back({from_,-1});std::reverse(path.begin(),path.end());status_=4;
}
NativeCharacterRoute::NativeCharacterRoute(const UCLevelLogic &logic,
    int32_t start,int32_t goal,const float from[3],const float to[3],
    const UCCharacterNavigationInfluence *influences,uint32_t influence_count)
    :logic_(logic),goal_(goal){
    require(logic.nodes&&(!logic.trigger_count||logic.triggers)&&from&&to&&start>=0&&goal>=0&&
        (uint32_t)start<logic.node_count&&(uint32_t)goal<logic.node_count&&influence_count<=4096&&(!influence_count||influences),"Navigation route inputs");
    for(unsigned j=0;j<3;j++){require(std::isfinite(from[j])&&std::isfinite(to[j]),"Navigation endpoints");from_[j]=from[j];to_[j]=to[j];}
    if(influence_count)influences_.assign(influences,influences+influence_count);
    size_t capacity=1;
    for(uint32_t i=0;i<logic.node_count;i++){
        const auto&n=logic.nodes[i];for(float x:n.position)require(std::isfinite(x),"Navigation node coordinates");
        require((!n.first.count||n.first.data)&&(!n.second.count||n.second.data),"Navigation adjacency storage");
        capacity+=(size_t)n.first.count+n.second.count;require(capacity<=1000000,"Navigation graph limit");
    }
    double checked;require(!uc_character_navigation_penalty(from,influences_.data(),influence_count,&checked),"Navigation influence domain");
    records_.resize(capacity);edges_.reserve(capacity);
    int32_t initial=edge(-1,start,-1);
    if(start==goal){finish(initial);return;}
    records_[initial].heuristic=(float)heuristic(initial);records_[initial].total=records_[initial].heuristic;enqueue(initial);
}
void NativeCharacterRoute::step(){
    if(status_>=3)return;status_=2;
    if(open_.empty()){finish(-1);return;}
    int32_t current=open_.front();open_.erase(open_.begin());records_[current].state=2;
    if(edges_[current].to==goal_){finish(current);return;}
    UCCharacterSearchServices s{};s.owner=this;
    s.neighbor=[](void*p,int32_t id,int32_t ordinal){auto&r=*static_cast<NativeCharacterRoute*>(p);int32_t from=r.edges_[id].to;const auto&n=r.logic_.nodes[from];
        uint32_t at=(uint32_t)ordinal;int32_t to=at<n.first.count?(int32_t)n.first.data[at]:at-n.first.count<n.second.count?(int32_t)n.second.data[at-n.first.count]:-1;
        return to==-1?-1:r.edge(from,to,id);};
    s.admit=[](void*p,int32_t id)->uint8_t{auto&r=*static_cast<NativeCharacterRoute*>(p);uint32_t key=r.logic_.nodes[r.edges_[id].to].key&0xFFF0FFFFu;
        return key<r.logic_.trigger_count&&(r.logic_.triggers[key].flags&0x10)!=0;};
    s.cost=[](void*p,int32_t id){auto&r=*static_cast<NativeCharacterRoute*>(p);const auto&e=r.edges_[id];double cost;
        require(!uc_character_navigation_cost(e.from<0?nullptr:r.logic_.nodes[e.from].position,r.logic_.nodes[e.to].position,r.influences_.data(),(uint32_t)r.influences_.size(),&cost),"Navigation edge cost");return cost;};
    s.heuristic=[](void*p,int32_t id){return static_cast<NativeCharacterRoute*>(p)->heuristic(id);};
    s.enqueue=[](void*p,int32_t id){static_cast<NativeCharacterRoute*>(p)->enqueue(id);};
    uint8_t more;for(int32_t ordinal=0;;++ordinal){
        require(!uc_character_search_expand(records_.data(),&count_,(uint32_t)records_.size(),current,ordinal,1,&s,&more),"Navigation search expansion");if(!more)break;
    }
}
NativeCharacterNavigator::NativeCharacterNavigator(const UCLevelLogic&logic):logic_(logic){
    require(logic.nodes&&(!logic.trigger_count||logic.triggers),"Navigator level lifetime");
}
NativeCharacterNavigator::~NativeCharacterNavigator(){if(manager_)manager_->detach(manager_->client(*this).binding.entity);}
void NativeCharacterNavigator::validate(const UCCharacterCandidateQuery&q){
    require(q.position&&q.ray&&q.scripted&&(!q.agent_count||q.agents)&&(!q.obstacle_count||q.obstacles)&&q.agent_count<=4096&&q.obstacle_count<=4096&&q.radius==40&&q.height==200&&q.horizon==1.5f,
        "Navigator requires original344C0 dimensions and world queries");
    for(unsigned j=0;j<3;j++)require(std::isfinite(q.position[j])&&fabsf(q.position[j])<=1e6f,"Navigator position");
}
void NativeCharacterNavigator::clear_paths(){if(manager_)manager_->cancel(*this);points_.clear();tags_.clear();nodes_.clear();detour_=NativeCharacterDetour{};blocker_=0;search_.reset();job_=0;progress_.cursor=0;}
bool NativeCharacterNavigator::near_goal()const{
    int result=uc_character_route_near(status(),101,goal_.data(),waypoint_.data());require(result>=0,"Navigator near-goal state");return result!=0;
}
void NativeCharacterNavigator::stop(){
    if(!status())return;progress_.status=0;restarts_=0;std::fill_n(timers_,4,0.f);clear_paths();
}
void NativeCharacterNavigator::restart(){
    if(!status())return;progress_.status=1;++restarts_;std::fill_n(timers_,4,0.f);clear_paths();
}
void NativeCharacterNavigator::start(const float goal[3],const UCCharacterCandidateQuery&q){
    validate(q);require(goal&&status()==0,"Navigator start requires the original controller reset");
    for(unsigned j=0;j<3;j++)require(std::isfinite(goal[j])&&fabsf(goal[j])<=1e6f,"Navigator goal");
    std::copy_n(goal,3,goal_.begin());int clear=uc_character_route_clear(&q,goal,101,1);require(clear>=0,"Navigator direct-goal query");
    if(clear){points_.push_back(goal_);tags_.push_back(0);nodes_.push_back(-1);progress_.status=2;waypoint_=goal_;}
    else progress_.status=1;
}
void NativeCharacterNavigator::retarget(const float goal[3],const UCCharacterCandidateQuery&q){
    validate(q);require(goal,"Navigator retarget goal");
    for(unsigned j=0;j<3;j++)require(std::isfinite(goal[j])&&fabsf(goal[j])<=1e6f,"Navigator retarget coordinates");
    std::array<float,3> next{goal[0],goal[1],goal[2]};goal_=next;
    if(status()==2){
        int32_t endpoint=-1;
        require(!uc_character_navigation_endpoint(&logic_,goal_.data(),q.region,q.ray,q.owner,&endpoint),"Navigator retarget endpoint");
        struct Query{NativeCharacterNavigator *nav;const UCCharacterCandidateQuery *q;const float *to;} context{this,&q,endpoint<0?nullptr:logic_.nodes[endpoint].position};
        auto clear=[](void*p,uint32_t index)->int{const auto&c=*static_cast<Query*>(p);const auto&q=*c.q;const float*from=c.nav->points_[index].data();uint8_t blocked;
            if(uc_character_world_blocked(from,c.to,q.radius,q.region,0,0,q.ray,q.scripted,q.owner,&blocked))return -1;
            if(blocked)return 0;
            int hit=uc_character_obstacles_cast(q.height,q.position,from,c.to,q.obstacles,q.obstacle_count,nullptr);
            return hit<0?-1:!hit;
        };
        UCCharacterRouteSplice plan{};int result=uc_character_route_splice(nodes_.data(),(uint32_t)nodes_.size(),progress_.cursor,endpoint,clear,&context,&plan);
        require(result>=0,"Navigator route splice");
        if(result){
            points_=std::vector<std::array<float,3>>(points_.begin()+plan.begin,points_.begin()+plan.end);
            tags_=std::vector<uint8_t>(tags_.begin()+plan.begin,tags_.begin()+plan.end);
            nodes_=std::vector<int32_t>(nodes_.begin()+plan.begin,nodes_.begin()+plan.end);
            if(plan.append_node){const auto&n=logic_.nodes[endpoint];points_.push_back({n.position[0],n.position[1],n.position[2]});nodes_.push_back(endpoint);tags_.push_back(n.field10==2);}
            points_.push_back(next);nodes_.push_back(-1);tags_.push_back(0);
            progress_.cursor=0;progress_.elapsed=0;detour_=NativeCharacterDetour{};blocker_=0;return;
        }
    }
    stop();start(next.data(),q);
}
void NativeCharacterNavigator::search_step(const UCCharacterCandidateQuery&q){
    validate(q);if(!job_||job_>=3)return;
    if(job_==1){
        int32_t from=-1,to=-1;
        require(!uc_character_navigation_endpoint(&logic_,search_from_.data(),q.region,q.ray,q.owner,&from)&&
            !uc_character_navigation_endpoint(&logic_,goal_.data(),q.region,q.ray,q.owner,&to),"Navigator search endpoints");
        if(from<0||to<0){job_=3;return;}
        search_=std::make_unique<NativeCharacterRoute>(logic_,from,to,search_from_.data(),goal_.data(),search_influences_.data(),(uint32_t)search_influences_.size());job_=search_->status();
        //1 denotes the search's ready state, not a second initialization.
        if(job_==1)job_=2;return;
    }
    search_->step();job_=search_->status();
}
void NativeCharacterNavigator::update(float delta,const UCCharacterCandidateQuery&input,
    const NativeCharacterDetour::Obstacle*obstacles,uint32_t count,
    void(*scripted_ray)(void*,const float*,const float*)){
    validate(input);require(std::isfinite(delta)&&delta>=0&&scripted_ray&&(!count||obstacles)&&count<=4096&&count==input.obstacle_count,"Navigator update services");
    auto q=input;
    if(status()==1){
        timers_[0]+=delta;
        if(!job_){
            std::copy_n(q.position,3,waypoint_.begin());float x=goal_[0]-q.position[0],z=goal_[2]-q.position[2];
            if(fabs((double)goal_[1]-q.position[1])<100&&sqrt((double)x*x+(double)z*z)<101){progress_.status=4;timers_[0]=0;return;}
            std::copy_n(q.position,3,search_from_.begin());search_influences_.clear();
            //1FAFE copies the entity-backed influence list into the search at
            //job creation; both lists prepend, so retain that exact order.
            for(const auto&i:influences_){UCCharacterNavigationInfluence v{{3.402823466e38f,3.402823466e38f,3.402823466e38f},i.radius,i.weight};
                for(uint32_t n=0;n<q.agent_count;n++)if(q.agents[n].entity==i.entity){std::copy_n(q.agents[n].body.position,3,v.position);break;}
                search_influences_.insert(search_influences_.begin(),v);}
            job_=1;if(manager_)manager_->enqueue(*this);
        }else if(job_==3){if(manager_)manager_->cancel(*this);search_.reset();job_=0;progress_.status=3;timers_[0]=0;}
        else if(job_==4){
            for(const auto&p:search_->path){points_.push_back(p.position);tags_.push_back(p.node>=0&&logic_.nodes[p.node].field10==2);nodes_.push_back(p.node);}
            if(manager_)manager_->cancel(*this);search_.reset();job_=0;
            require(!uc_character_route_adopt(&progress_,reinterpret_cast<const float(*)[3]>(points_.data()),tags_.data(),(uint32_t)points_.size(),&q),"Navigator search adoption");timers_[0]=0;
        }else{double x=(double)waypoint_[0]-q.position[0],z=(double)waypoint_[2]-q.position[2];if(sqrt(x*x+z*z)>350)restart();}
        return;
    }
    if(status()==2){
        uint8_t changed=timers_[1]==0;timers_[1]+=delta;
        require(!uc_character_route_update(&progress_,reinterpret_cast<const float(*)[3]>(points_.data()),tags_.data(),(uint32_t)points_.size(),101,delta,&q,&changed,waypoint_.data()),"Navigator route update");
        if(status()!=2){if(status()==1)restart();else{clear_paths();timers_[1]=0;}return;}
        if(changed){detour_=NativeCharacterDetour{};blocker_=0;}
        q.destination=points_[progress_.cursor].data();
        UCCharacterAgentSteering steering{};std::copy_n(waypoint_.data(),3,steering.waypoint);steering.status=status();steering.changed=changed;steering.blocker=blocker_;
        int more=detour_.step(steering,q,goal_.data(),101,obstacles,count);
        if(more){more=uc_character_agent_steer(&steering,q.radius,q.height,q.horizon,q.velocity,q.self,q.position,goal_.data(),q.agents,q.agent_count,uc_character_candidate_clear,&q);require(more>=0,"Navigator agent steering");}
        std::copy_n(steering.waypoint,3,waypoint_.begin());blocker_=steering.blocker;progress_.status=steering.status;
        if(!more)timers_[1]=0;else scripted_ray(q.owner,q.position,waypoint_.data());return;
    }
    uint8_t again=0;
    if(status()==3){
        const float*velocity=nullptr;
        if(blocker_){for(uint32_t i=0;i<q.agent_count;i++)if(q.agents[i].entity==blocker_){velocity=q.agents[i].body.velocity;break;}require(velocity!=nullptr,"Navigator blocker retired without agent detach");}
        require(!uc_character_navigation_blocked(timers_+2,delta,101,velocity,&again),"Navigator blocked recovery");
    }else if(status()==4)require(!uc_character_navigation_arrived(timers_+3,delta,101,q.position,goal_.data(),&again),"Navigator arrived recovery");
    if(again)restart();
}
NativeCharacterNavigationManager::Client &NativeCharacterNavigationManager::client(const NativeCharacterNavigator&nav){
    for(auto&c:clients_)if(c->binding.navigator==&nav)return *c;throw std::runtime_error("Unregistered navigator");
}
void NativeCharacterNavigationManager::enqueue(NativeCharacterNavigator&nav){auto&job=client(nav).search;job.complete=false;searches_.add(job);}
void NativeCharacterNavigationManager::cancel(NativeCharacterNavigator&nav){searches_.remove(client(nav).search);}
NativeCharacterNavigationManager::~NativeCharacterNavigationManager(){while(!clients_.empty())detach(clients_.back()->binding.entity);}
void NativeCharacterNavigationManager::attach(const Binding&binding){
    require(binding.entity>0&&binding.position&&std::isfinite(binding.radius)&&binding.radius>0&&std::isfinite(binding.height)&&binding.height>0&&clients_.size()<4096,"Navigation agent attachment");
    require(!binding.navigator||(!binding.navigator->manager_&&binding.radius==40&&binding.height==200&&binding.ray&&binding.scripted&&binding.scripted_ray),"Navigator attachment");
    for(const auto&c:clients_)require(c->binding.entity!=binding.entity,"Duplicate navigation actor");
    for(unsigned j=0;j<3;j++)require(std::isfinite(binding.position[j])&&fabsf(binding.position[j])<=1e6f,"Navigation actor position");
    auto c=std::make_unique<Client>();c->binding=binding;c->manager=this;c->search.owner=c.get();
    c->search.initialize=c->search.step=[](NativeCharacterSearchManager::Job&job){
        auto&c=*static_cast<Client*>(job.owner);auto&nav=*c.binding.navigator;
        nav.search_step(c.manager->query(c.binding.entity));job.complete=nav.job_>=3;
    };
    UCCharacterAgentEntry agent{};agent.entity=binding.entity;agent.body.radius=binding.radius;agent.body.height=binding.height;std::copy_n(binding.position,3,agent.body.position);
    clients_.push_back(std::move(c));try{agents_.push_back(agent);}catch(...){clients_.pop_back();throw;}
    if(binding.navigator){binding.navigator->stop();binding.navigator->manager_=this;}
}
void NativeCharacterNavigationManager::add_agent(int32_t entity,const float*position,float radius,float height){Binding b;b.entity=entity;b.position=position;b.radius=radius;b.height=height;attach(b);}
void NativeCharacterNavigationManager::detach(int32_t entity){
    auto at=std::find_if(clients_.begin(),clients_.end(),[entity](const auto&c){return c->binding.entity==entity;});
    if(at==clients_.end())return;
    searches_.remove((*at)->search);if(auto*nav=(*at)->binding.navigator){nav->manager_=nullptr;nav->stop();}
    size_t index=(size_t)(at-clients_.begin());clients_.erase(at);agents_.erase(agents_.begin()+index);
    //227F0 must invalidate every borrowed blocker before the actor disappears.
    for(auto&c:clients_)if(auto*nav=c->binding.navigator){if(nav->blocker_==entity){nav->blocker_=0;nav->restart();}}
}
void NativeCharacterNavigationManager::snapshot_obstacles(){
    obstacles_.clear();collision_.clear();
    for(const auto*p:obstacle_owners_){obstacles_.push_back(*p);collision_.push_back(p->collision);}
}
void NativeCharacterNavigationManager::add_obstacle(const NativeCharacterDetour::Obstacle&obstacle){
    require(obstacle.identity&&obstacle_owners_.size()<4096,"Navigation obstacle registration");
    for(const auto*p:obstacle_owners_)require(p->identity!=obstacle.identity,"Duplicate navigation obstacle");
    obstacle_owners_.push_back(&obstacle);snapshot_obstacles();
}
void NativeCharacterNavigationManager::remove_obstacle(const void*identity){
    require(identity!=nullptr,"Navigation obstacle identity");
    for(auto&c:clients_)if(auto*nav=c->binding.navigator){if(nav->detour_.identity()==identity)nav->restart();}
    obstacle_owners_.erase(std::remove_if(obstacle_owners_.begin(),obstacle_owners_.end(),[identity](const auto*p){return p->identity==identity;}),obstacle_owners_.end());snapshot_obstacles();
}
UCCharacterCandidateQuery NativeCharacterNavigationManager::query(int32_t entity)const{
    for(size_t i=0;i<clients_.size();i++)if(clients_[i]->binding.entity==entity){
        const auto&b=clients_[i]->binding;require(b.navigator!=nullptr,"Passive agent has no navigator query");UCCharacterCandidateQuery q{};
        q.radius=40;q.height=200;q.horizon=1.5f;q.position=b.position;q.velocity=agents_[i].body.velocity;q.self=entity;q.region=b.region;
        q.obstacles=collision_.data();q.obstacle_count=(uint32_t)collision_.size();q.agents=agents_.data();q.agent_count=(uint32_t)agents_.size();
        q.ray=b.ray;q.scripted=b.scripted;q.owner=b.owner;return q;
    }
    throw std::runtime_error("Unregistered navigation actor");
}
void NativeCharacterNavigationManager::update(float delta,void(*refresh_obstacles)(void*),void*owner){
    require(std::isfinite(delta)&&delta>0,"Navigation manager positive simulation delta");
    require(std::isfinite(1.f/delta),"Navigation manager delta reciprocal");
    searches_.update(); //55610: searches precede obstacle/agent refresh and21F00.
    if(refresh_obstacles)refresh_obstacles(owner);snapshot_obstacles();
    const float inverse=1.f/delta;
    for(size_t i=0;i<clients_.size();i++){
        auto&a=agents_[i].body;const auto*p=clients_[i]->binding.position;
        for(unsigned j=0;j<3;j++){require(std::isfinite(p[j])&&fabsf(p[j])<=1e6f,"Navigation actor position");a.velocity[j]=(p[j]-a.position[j])*inverse;a.position[j]=p[j];}
        a.velocity[1]=0;
    }
    for(size_t i=0;i<clients_.size();i++){
        auto*nav=clients_[i]->binding.navigator;if(!nav||nav->status()!=2)continue;
        auto&a=agents_[i].body;float speed=(float)sqrt(((double)a.velocity[0]*a.velocity[0]+(double)a.velocity[1]*a.velocity[1])+(double)a.velocity[2]*a.velocity[2]);
        for(unsigned j=0;j<3;j++)a.velocity[j]=nav->waypoint_[j]-a.position[j];
        float square=(a.velocity[0]*a.velocity[0]+a.velocity[1]*a.velocity[1])+a.velocity[2]*a.velocity[2];
        if(square>0.0001f){double scale=1./sqrt(((double)a.velocity[0]*a.velocity[0]+(double)a.velocity[1]*a.velocity[1])+(double)a.velocity[2]*a.velocity[2]);for(float&v:a.velocity)v=(float)((double)v*scale);}
        for(float&v:a.velocity)v*=speed;
    }
    for(auto&c:clients_){const auto&b=c->binding;if(b.navigator)b.navigator->update(delta,query(b.entity),obstacles_.data(),(uint32_t)obstacles_.size(),b.scripted_ray);}
}
