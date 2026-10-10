#include "mission_world.h"
#include "mission_trigger.h"
extern "C" {
#include "../entity_update.h"
#include "../entity_retirement.h"
#include "../entity_pending.h"
#include "../entity_record_priority.h"
}
#include <cmath>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <new>
#include <vector>
struct NativeMissionWorld::Impl {
    NativeMissionWorld *world;const UCLevel *level=nullptr;UCLevelLogic *logic=nullptr;
    NativeMissionWorldServices external{};NativeMissionServices services{};
    UCEntityEventRegistry registry{};UCEntityNames *names=nullptr;UCEntityListenerList *listeners=nullptr;
    UCEntityList *live=nullptr,*pending=nullptr;UCEntityRecords *records[2]{};
    //209ED0/209F1E..23: both deltas start at44C53C (0x3D088889).
    uint32_t next=0,counter=0,frame=0;bool startup_complete=false;float delta=1.f/30,other=1.f/30,elapsed=0;int32_t count=0;
    UCEntityCreation creation{0,1,-1};UCEntityRegistration registration{};
    UCEntityRetirement retirement{};
    const void *definition=nullptr;const UCEntityAuthored *active=nullptr;
    UCEntityFreshContext context{};UCLevelEntitySlots lookup{};
    std::vector<int32_t> ids;std::vector<uint32_t> flags;
    std::vector<UCLevelEntitySlot> slots;std::vector<UCLevelEntitySlot*> pointers;
    std::vector<UCEntityAuthored> metadata;std::vector<const uint8_t*> cursors;
    std::vector<std::vector<UCMissionCondition>> conditions;
    std::vector<NativeMissionBinding> bindings;
    struct ScriptOwner {Impl *world=nullptr;NativeMissionEntity entity;};
    std::vector<std::unique_ptr<ScriptOwner>> owned;
    struct Trigger {NativeMissionEntity entity;NativeMissionTrigger controller;};
    std::vector<std::unique_ptr<Trigger>> triggers;
    std::vector<uint32_t> trigger_ordinals;
    std::vector<UCEntityRecord*> orphan_records;
    std::vector<int32_t> step_parents;
    struct Action {uint32_t source,mode;float age;};
    std::vector<Action> actions;
    struct Operate {uint32_t source,target;float age;};
    std::vector<Operate> operations;
    uint32_t allocating=0,failed_source=UINT32_MAX,failed_opcode=UINT32_MAX;
    const char *phase="initialization",*failed_phase="none";
    int error=0;bool ticking=false,in_step=false,pre_world=false;
    explicit Impl(NativeMissionWorld*w):world(w){}
    int fail(uint32_t i,uint32_t op=UINT32_MAX){if(!error){failed_source=i;failed_opcode=op;failed_phase=phase;}error=2;return 2;}
    uint32_t index(const UCEntityEventTarget*t)const{for(uint32_t i=0;i<bindings.size();i++)if(bindings[i].view&&bindings[i].view->target.identity==t)return i;return UINT32_MAX;}
    static uint32_t read_flags(void*p,const UCEntityEventTarget*t){auto&w=*(Impl*)p;uint32_t i=w.index(t);return i<w.bindings.size()?*w.bindings[i].view->target.flags:0;}
    static const UCEntityFreshView *resolve(void*p,UCEntityEventTarget*t){auto&w=*(Impl*)p;uint32_t i=w.index(t);return i<w.bindings.size()?w.bindings[i].view:nullptr;}
    static void *allocate(void*,size_t n){return std::malloc(n);}
    static void release(void*,void*p){std::free(p);}
    static UCEntityRecord *record_new(void*){return (UCEntityRecord*)std::malloc(sizeof(UCEntityRecord));}
    static void record_delete(void*,UCEntityRecord*r){std::free(r);}
    static int remove_update(void*p,int32_t id){auto&w=*(Impl*)p;UCEntityRecordMemory m{p,record_new,record_delete};uint8_t removed;return uc_entity_record_remove_update(&m,w.records[1],id,&removed);}
    static int remove_render(void*p,int32_t id){auto&w=*(Impl*)p;UCEntityRecordMemory m{p,record_new,record_delete};uint8_t removed;return uc_entity_record_remove_render(&m,w.records[0],id,&removed);}
    static int insert(void*p,const UCEntityRegistrationTarget*t,int32_t id,int update){auto&w=*(Impl*)p;
        if(w.ticking){
            if(!w.in_step||w.creation.priority!=1)return 1;
            if(w.creation.relative){bool admitted=false;for(auto parent:w.step_parents)if(parent==*t->parent_id)admitted=true;if(!admitted)return 1;}
        }
        UCEntityRecordMemory m{p,record_new,record_delete};UCEntityRecordInsertResult result{};
        int r=uc_entity_record_insert(&m,w.records[update],&w.creation,t,id,&result);
        if(r)return r;
        if(!result.inserted){w.orphan_records.push_back(result.record);return 1;}
        if(w.ticking)w.step_parents.push_back(id);return 0;
    }
    static int script_step(void*p){return ((NativeMissionEntity*)p)->script.update();}
    static int script_late(void*p){auto&e=*(NativeMissionEntity*)p;
        //Default218C30/218400/46980: zero velocity still publishes W and +zero.
        if(!(e.actor.word90&1)){for(unsigned i=12;i<15;i++){if(!std::isfinite(e.transform[i]))return 1;e.transform[i]+=0.0f;}e.transform[15]=1.0f;}return 0;
    }
    static int script_close(void*p){auto&e=*(NativeMissionEntity*)p;if(e.script.close_detached())return 1;if(uc_entity_fresh_aux_destroy(e.aux))return 1;e.aux=nullptr;return 0;}
    static int trigger_step(void*p){return ((Trigger*)p)->controller.update();}
    static int trigger_late(void*p){return script_late(&((Trigger*)p)->entity);}
    static int trigger_close(void*p){auto&t=*(Trigger*)p;if(t.controller.close_detached())return 1;return script_close(&t.entity);}
    static int owned_remove(void*p){auto&o=*(ScriptOwner*)p;auto&w=*o.world;auto&e=o.entity;
        //218B30/2183C0/20A620, concrete model/helper/listener-free controllers.
        //Do not admit an unimplemented model, source helper or wait-handle suffix.
        if(e.model||e.helper||e.actor.handleBC||!e.aux)return 1;
        for(auto*l:e.identity.listeners)if(l)return 1;
        if((e.flags&0x1000)&&(e.source<0||uint32_t(e.source)>=w.slots.size()))return 1;
        uc_mission_command_destroy(nullptr,e.actor.audioC0);e.actor.audioC0=nullptr;
        if(uc_entity_fresh_aux_clear(e.aux))return 2;e.flags|=0x8100;
        uint8_t queued=0;if(uc_entity_retire(&w.retirement,e.id,&queued)||!queued)return 2;
        if(e.flags&0x1000){auto*f=w.slots[e.source].flags;*f=(*f&~15u)|16u;}
        e.flags|=0x80000;return 0;
    }
    static int owned_destroy(void*p){auto&o=*(ScriptOwner*)p;auto&w=*o.world;uint32_t i=w.index(&o.entity.identity);
        if(i>=w.owned.size()||w.owned[i].get()!=&o||script_close(&o.entity))return 1;
        w.owned[i].reset();return 0;
    }
    static NativeMissionEntity *allocate_script(void*p){auto&w=*(Impl*)p;
        auto e=std::unique_ptr<ScriptOwner>(new(std::nothrow)ScriptOwner);
        if(!e)return nullptr;e->world=&w;auto*q=e.get();w.owned[w.allocating]=std::move(e);
        w.bindings[w.allocating]={&q->entity.view,q,
            [](void*p){return script_step(&((ScriptOwner*)p)->entity);},
            [](void*p){return script_late(&((ScriptOwner*)p)->entity);},
            [](void*p){return script_close(&((ScriptOwner*)p)->entity);},&q->entity.script,owned_remove,owned_destroy};return &q->entity;
    }
    static int create(void*p,UCLevelEntitySlot*s,uint32_t i){auto&w=*(Impl*)p;if(w.error||i>=w.metadata.size())return 1;
        if((*s->flags&0x18)||(i<w.level->entity_count?!(*s->flags&2):!(*s->flags&0x40)))return 0;
        const auto&m=w.metadata[i];w.allocating=i;
        if(i>=w.level->entity_count){
            if(!w.external.trigger_body)return w.fail(i);
            auto t=std::unique_ptr<Trigger>(new(std::nothrow)Trigger);if(!t)return 0;
            auto *q=t.get();w.triggers[i]=std::move(t);
            if(w.world->bind(i,{&q->entity.view,q,trigger_step,trigger_late,trigger_close,nullptr}))return w.fail(i);
            UCMissionTriggerWorld query{w.names,&w.registry,&w.lookup,w.external.owner,w.external.trigger_body};
            auto&definition=w.logic->triggers[w.trigger_ordinals[i]];
            int r=q->controller.construct(w.context,&w.active,q->entity.view,definition,(int32_t)i,*s,
                w.conditions[i].data(),(uint32_t)w.conditions[i].size(),query,w.services);
            return r?w.fail(i):0;
        }
        if(i<w.level->entity_count&&(!std::strcmp(m.definition->class_name,"ScriptController")||
           !std::strcmp(m.definition->class_name,"WaveController")||!std::strcmp(m.definition->class_name,"FriendlyWaveController"))){
            NativeMissionEntity*out=nullptr;int r=native_mission_create(w.context,&w.active,m,&w.cursors[i],*s,w.services,&w,allocate_script,&out);return r?w.fail(i):0;
        }
        if(!w.external.construct)return w.fail(i);
        w.creation={0,1,-1};w.active=&m;
        w.phase="construction";int r=w.external.construct(w.external.owner,*w.world,i);w.active=nullptr;
        if(r)return w.fail(i);
        w.phase="registration after construction";const auto*b=w.bindings[i].view;
        if(b){if(*b->id<0||uc_entity_event_lookup(&w.registry,*b->id)!=b->target.identity)return w.fail(i);
            if(i>=w.level->entity_count){if(!(*s->flags&8))return w.fail(i);return 0;}
            if(!(*s->flags&0x20))*b->target.flags|=0x10000000;
            *s->id=*b->id;*s->flags=(*s->flags&0xffffffe8u)|8;
        }
        w.cursors[i]=m.definition->properties.data;return 0;
    }
    static int ensure(void*p,uint32_t i){return ((Impl*)p)->world->ensure(i);}
    int apply(int32_t id,uint32_t mode){
        auto*t=uc_entity_event_lookup(&registry,id);uint32_t i=index(t);
        if(!t||i>=bindings.size())return fail(UINT32_MAX);
        auto*f=bindings[i].view->target.flags;
        if(mode==0){*f|=1;
            //20A140(1) tail-calls207F00: dormant entities must rejoin updates
            //so their actual remove/destructor path can run.
            if(*f&8){creation={0,1,*bindings[i].view->target.parent_id};
                if(insert(this,&bindings[i].view->target,id,1))return fail(i);
                *f&=~8u;
            }
        }else if(mode==1)*f|=0x10000000;else *f&=~0x10000000u;
        return 0;
    }
    static int action(void*p,uint32_t source,uint32_t mode){auto&w=*(Impl*)p;int32_t id;
        if(mode>2||uc_level_entity_lookup(&w.lookup,source,&id))return w.fail(source);
        if(id==-2)return 0;
        if(id!=-1)return w.apply(id,mode);
        try{w.actions.push_back({source,mode,0});}catch(const std::bad_alloc&){return w.fail(source);}
        return 0;
    }
    static int operate(void*p,uint32_t source,int32_t sender,const float*position,uint32_t target){auto&w=*(Impl*)p;int32_t id;
        if(!position||uc_level_entity_lookup(&w.lookup,target,&id))return w.fail(target);
        if(id==-2)return 0;
        if(id==-1){try{w.operations.push_back({source,target,0});}catch(const std::bad_alloc&){return w.fail(target);}return 0;}
        uint8_t handled=0;uint32_t from=(uint32_t)sender;
        //Retail4F9FC8 = BF. BE2A0 copies sender54, forwards position40.
        return uc_entity_event_send(&w.registry,id,0xBF,position,12,&from,&handled)?w.fail(target):0;
    }
    int queued_operations(){
        for(size_t i=0;i<operations.size();){auto a=operations[i];int32_t sender,target;
            if(uc_level_entity_lookup(&lookup,a.source,&sender))return fail(a.source);
            //208FD0 drops the request immediately if its sender is gone.
            if(sender!=-1&&sender!=-2){
                if(uc_level_entity_lookup(&lookup,a.target,&target))return fail(a.target);
                if(target==-1||target==-2){if((operations[i].age+=1.f)<=30.f){i++;continue;}}
                else{const float zero[3]={};uint32_t from=(uint32_t)sender;uint8_t handled=0;
                    if(uc_entity_event_send(&registry,target,0xBF,zero,12,&from,&handled))return fail(a.target);}
            }
            operations.erase(operations.begin()+i);
        }return 0;
    }
    //208F80 queue2064 precedes entity updates.208300 appends in request order.
    //Absent or retired targets expire after31 updates, independent of delta.
    //ponytail: vector erase is quadratic for mass expiry; compact in one pass
    //if authored queue volume makes this measurable.
    int queued_actions(){
        for(size_t i=0;i<actions.size();){auto&a=actions[i];int32_t id;
            if(uc_level_entity_lookup(&lookup,a.source,&id))return fail(a.source);
            if(id!=-1&&id!=-2){if(apply(id,a.mode))return 2;}
            else if((a.age+=1.f)<=30.f){i++;continue;}
            actions.erase(actions.begin()+i);
        }return 0;
    }
    static int extra(void*p,UCMissionEvent*e,const UCLevelEntity*d,int32_t*r){auto&w=*(Impl*)p;return w.external.decode_extra?w.external.decode_extra(w.external.owner,e,d,r):1;}
    static int character_query(void*p,int32_t id){auto&w=*(Impl*)p;return w.external.character_query?w.external.character_query(w.external.owner,id):1;}
    static int character_status(void*p,int32_t id,uint32_t*f){auto&w=*(Impl*)p;return w.external.character_status?w.external.character_status(w.external.owner,id,f):1;}
    static UCEntityUpdateFields fields(void*p,UCEntityEventTarget*t){auto&w=*(Impl*)p;auto*v=w.bindings[w.index(t)].view;return {v->flags60,v->helper78};}
    static void step(void*p,UCEntityEventTarget*t){auto&w=*(Impl*)p;uint32_t i=w.index(t);if(w.error)return;auto b=w.bindings[i];
        w.in_step=true;w.step_parents.clear();w.step_parents.push_back(*b.view->id);
        auto start=w.external.profile_step?std::chrono::steady_clock::now():std::chrono::steady_clock::time_point{};
        w.phase="update";int result=b.step?b.step(b.owner):1;
        if(w.external.profile_step)w.external.profile_step(w.external.owner,i,std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count());w.in_step=false;w.step_parents.clear();
        if(result){uint32_t op=UINT32_MAX;if(b.view->actor&&b.view->actor->wordC4)std::memcpy(&op,b.view->actor->wordC4,4);w.fail(i,op);}}
    static void late(void*p,UCEntityEventTarget*t){auto&w=*(Impl*)p;if(w.error)return;uint32_t i=w.index(t);w.phase="late update";if(i>=w.bindings.size()){w.fail(i);return;}auto b=w.bindings[i];auto start=w.external.profile_step?std::chrono::steady_clock::now():std::chrono::steady_clock::time_point{};if(!b.late||b.late(b.owner))w.fail(i);if(w.external.profile_step)w.external.profile_step(w.external.owner,i|0x80000000u,std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count());}
    static void helper(void*p,void*h,float dt){auto&w=*(Impl*)p;if(!w.error&&(!w.external.helper_tick||w.external.helper_tick(w.external.owner,h,dt)))w.fail(UINT32_MAX);}
    static void remove(void*p,UCEntityEventTarget*t){auto&w=*(Impl*)p;if(w.error)return;uint32_t i=w.index(t);if(i>=w.bindings.size()){w.fail(i);return;}auto b=w.bindings[i];
        //208F80 invokes virtual24 inside the same live update traversal as
        //virtual28. E3930 exits the controller's dormant children via20A140,
        //which reinserts their actual records before this actor is destroyed.
        w.in_step=true;w.step_parents.clear();w.step_parents.push_back(*b.view->id);
        w.phase="removal";int result=b.remove?b.remove(b.owner):1;w.in_step=false;w.step_parents.clear();if(result)w.fail(i);}
    static void destroy(void*p,UCEntityEventTarget*t){auto&w=*(Impl*)p;if(w.error)return;uint32_t i=w.index(t);if(i>=w.bindings.size()){w.fail(i);return;}auto b=w.bindings[i];
        w.phase="destruction";if(!b.destroy||b.destroy(b.owner)){w.fail(i);return;}w.bindings[i]={};}
    static void pending_destroy(void*p,void*t){destroy(p,(UCEntityEventTarget*)t);}
};
NativeMissionWorld::NativeMissionWorld():p_(std::make_unique<Impl>(this)){}
NativeMissionWorld::~NativeMissionWorld(){close();}
int NativeMissionWorld::load(const UCLevel&l,UCLevelLogic&g,uint8_t*f,size_t n,uint32_t*seed,const NativeMissionWorldServices&s){
 auto&w=*p_;if(w.level||g.level!=&l||!l.entities||!f||n<l.field||!seed)return 1;
 uint32_t count=l.entity_count;for(uint32_t i=0;i<g.trigger_count;i++)count+=(g.triggers[i].flags&32)!=0;
 if(!count||count>100000)return 1;
 w.level=&l;w.logic=&g;w.external=s;w.count=(int32_t)count;w.elapsed=0;
 w.ids.assign(count,-1);w.flags.resize(count);w.slots.resize(count);w.pointers.resize(count);w.metadata.resize(count);w.cursors.resize(count);w.conditions.resize(count);w.bindings.resize(count);w.owned.resize(count);
 w.triggers.resize(count);w.trigger_ordinals.assign(count,UINT32_MAX);
 if(uc_entity_slots_initialize(&w.registry,&w.next)||uc_entity_names_create(&w.names)||uc_entity_listener_list_create(&w.listeners))return w.fail(UINT32_MAX);
 w.live=uc_entity_list_create(1024);w.pending=uc_entity_list_create(1024);w.records[0]=uc_entity_records_create(1024,4);w.records[1]=uc_entity_records_create(1024,4);if(!w.live||!w.pending||!w.records[0]||!w.records[1])return w.fail(UINT32_MAX);
 uint32_t ordinal=0;
 for(uint32_t i=0;i<count;i++){
  const UCLevelEntity*d=nullptr;uint8_t present=0;
  if(i<l.entity_count){d=l.entities+i;w.flags[i]=d->active?4u:2u;if(uc_level_entity_asset_present(&l,i,&present))return w.fail(i);}
  else{while(!(g.triggers[ordinal].flags&32))ordinal++;w.trigger_ordinals[i]=ordinal;d=&g.triggers[ordinal++].script;w.flags[i]=0x41;}
  w.slots[i]={&w.ids[i],&w.flags[i]};w.pointers[i]=&w.slots[i];w.cursors[i]=d->properties.data;
  uint32_t capacity=0;if(d->script_records.size>=4)std::memcpy(&capacity,d->script_records.data,4);
  if(capacity>4096)return w.fail(i);w.conditions[i].resize(capacity);uint32_t used=0;if(uc_mission_conditions_load(d,w.conditions[i].data(),capacity,&used))return w.fail(i);w.conditions[i].resize(used);
  w.metadata[i]={d,(int32_t)i,present,w.conditions[i].data(),used};
 }
 w.registration={&w.registry,&w.next,w.live,&w.creation,&w,Impl::read_flags,Impl::insert};w.context={&w.registration,&w.counter,&w.definition,&w,Impl::resolve,{&w,Impl::allocate,Impl::release}};
 w.retirement={&w.registry,w.live,w.pending,&w,Impl::remove_update,Impl::remove_render};
 w.lookup={w.pointers.data(),&w.count,count,&w,Impl::create};w.services.flags=f;w.services.flag_count=n;w.services.flag_limit=l.field;w.services.crt_seed=seed;w.services.delta=&w.delta;w.services.owner=&w;w.services.ensure=Impl::ensure;w.services.decode_extra=Impl::extra;
 w.services.slots=&w.lookup;w.services.registry=&w.registry;w.services.logic=&g;w.services.entity_action=Impl::action;w.services.operate=Impl::operate;w.services.character_query=Impl::character_query;w.services.character_status=Impl::character_status;return 0;
}
int NativeMissionWorld::start_automatic(){auto&w=*p_;if(!w.level||w.error)return 1;for(uint32_t i=0;i<w.slots.size();i++)if((w.flags[i]&2)||(w.flags[i]&0x40)){if(Impl::create(&w,&w.slots[i],i))return 2;}return 0;}
int NativeMissionWorld::complete_startup(){auto&w=*p_;if(!w.level||w.error||w.ticking||w.startup_complete||w.frame)return 1;++w.frame;w.startup_complete=true;return 0;}
uint32_t NativeMissionWorld::frame()const{return p_->frame;}
const char *NativeMissionWorld::failure_phase()const{return p_->failed_phase;}
int NativeMissionWorld::ensure(uint32_t i,int32_t*out){auto&w=*p_;if(!w.level||w.error)return 1;int32_t id=-1;int r=uc_level_entity_ensure(&w.lookup,i,&id);if(r)return w.fail(i);if(out)*out=id;return 0;}
int NativeMissionWorld::ensure_trigger(uint32_t ordinal,int32_t*out){auto&w=*p_;if(!w.level||w.error||ordinal>=w.logic->trigger_count)return 1;
 for(uint32_t i=w.level->entity_count;i<w.trigger_ordinals.size();i++)if(w.trigger_ordinals[i]==ordinal)return ensure(i,out);return 1;}
int NativeMissionWorld::event(uint32_t i,uint32_t group,uint8_t*a){auto&w=*p_;if(w.error||i>=w.bindings.size()||!w.bindings[i].script)return 1;int r=w.bindings[i].script->event(group,1,a);return r?w.fail(i):0;}
int NativeMissionWorld::tick(float dt){auto&w=*p_;if(!w.level||w.error||w.ticking||w.pre_world||!std::isfinite(dt)||dt<0)return 1;
 //208F91..208F9F publishes this before operations, actions and both passes.
 const float next_time=dt+w.elapsed;if(!std::isfinite(next_time))return 1;++w.frame;
 w.phase="pre-world managers";
 if(w.external.pre_world){struct Scope{bool&b;explicit Scope(bool&v):b(v){b=true;}~Scope(){b=false;}}scope(w.pre_world);
  auto start=w.external.profile_step?std::chrono::steady_clock::now():std::chrono::steady_clock::time_point{};
  if(w.external.pre_world(w.external.owner,w.delta))return w.fail(UINT32_MAX);
  if(w.external.profile_step)w.external.profile_step(w.external.owner,UINT32_MAX,std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count());}
 w.elapsed=next_time;
 if(w.queued_operations()||w.queued_actions())return 2;
 w.phase="removal readiness";
 for(auto&b:w.bindings)if(b.view){uint32_t f=*b.view->target.flags;if(((f&1)&&(!b.remove||!b.destroy))||((f&0x8000)&&!b.destroy))return w.fail(w.index(b.view->target.identity));}
 // A callback exception must still permit detached cleanup during unwinding.
 struct UpdateScope{Impl&w;explicit UpdateScope(Impl&v):w(v){w.ticking=true;}~UpdateScope(){w.ticking=w.in_step=false;w.step_parents.clear();}}scope(w);
 UCEntityUpdateServices h{&w,Impl::fields,Impl::remove,Impl::step,Impl::helper,Impl::late,Impl::destroy};int r=uc_entity_update_records(w.records[1],w.pending,dt,dt,&w.delta,&w.other,&h);
 if(!r&&!w.error)r=uc_entity_pending_drain(w.pending,Impl::pending_destroy,&w);
 return (r||w.error)?w.fail(w.failed_source,w.failed_opcode):0;
}
int NativeMissionWorld::bind(uint32_t i,const NativeMissionBinding&b){auto&w=*p_;if(i>=w.bindings.size()||w.bindings[i].view||!b.view||!b.view->target.identity||!b.view->target.flags||!b.view->id||!b.view->flags60||!b.view->helper78||(!b.view->actor&&b.script)||!b.step||!b.late||!b.close_detached)return 1;w.bindings[i]=b;return 0;}
int NativeMissionWorld::bind_dynamic(const NativeMissionBinding&b,uint32_t*out){auto&w=*p_;if(!w.level||w.error||w.bindings.size()-w.metadata.size()>=1024||!b.view||w.index(b.view->target.identity)!=UINT32_MAX)return 1;
 uint32_t i=(uint32_t)w.bindings.size();try{w.bindings.push_back({});}catch(const std::bad_alloc&){return 1;}
 if(bind(i,b)){w.bindings.pop_back();return 1;}if(out)*out=i;return 0;
}
int NativeMissionWorld::set_priority(int32_t id,int32_t priority){auto&w=*p_;
 if(!w.level||w.error||!uc_entity_event_lookup(&w.registry,id))return 1;
 if(w.ticking){if(!w.in_step)return 1;bool child=false;for(size_t i=1;i<w.step_parents.size();i++)child|=w.step_parents[i]==id;if(!child)return 1;}
 UCEntityRecordMemory memory{&w,Impl::record_new,Impl::record_delete};UCEntityPriorityLists lists{w.records[0],w.records[1]};const auto*active=&lists;
 return uc_entity_record_priority_both(&memory,&active,id,priority);
}
int NativeMissionWorld::close(){auto&w=*p_;if(!w.level)return 0;if(w.ticking||w.pre_world)return 1;int failed=0;UCEntityRecordMemory memory{&w,Impl::record_new,Impl::record_delete};
 for(auto&b:w.bindings)if(b.view){const auto*v=b.view;uint8_t removed=0;failed|=uc_entity_record_remove_render(&memory,w.records[0],*v->id,&removed);failed|=uc_entity_record_remove_update(&memory,w.records[1],*v->id,&removed);uc_entity_list_remove_first(w.live,v->target.identity);uc_entity_slot_release(&w.registry,*v->id);if(b.close_detached)failed|=b.close_detached(b.owner);b={};}
 w.owned.clear();w.triggers.clear();for(auto*r:w.orphan_records)std::free(r);w.orphan_records.clear();uc_entity_records_destroy(w.records[0]);uc_entity_records_destroy(w.records[1]);uc_entity_list_destroy(w.live);uc_entity_list_destroy(w.pending);uc_entity_names_close(w.names);uc_entity_listener_list_close(w.listeners);w.level=nullptr;return failed;
}
const NativeMissionBinding*NativeMissionWorld::binding(uint32_t i)const{return i<p_->bindings.size()&&p_->bindings[i].view?&p_->bindings[i]:nullptr;}
const UCEntityFreshContext&NativeMissionWorld::context()const{return p_->context;}
const UCEntityAuthored**NativeMissionWorld::active_descriptor(){return &p_->active;}
const UCEntityAuthored&NativeMissionWorld::metadata(uint32_t i)const{return p_->metadata.at(i);}
const uint8_t**NativeMissionWorld::property_cursor(uint32_t i){return &p_->cursors.at(i);}
UCLevelEntitySlot&NativeMissionWorld::slot(uint32_t i){return p_->slots.at(i);}
const UCLevelEntitySlots&NativeMissionWorld::slots()const{return p_->lookup;}
UCEntityEventRegistry&NativeMissionWorld::registry(){return p_->registry;}
UCEntityNames*NativeMissionWorld::names(){return p_->names;}
UCEntityListenerList*NativeMissionWorld::listeners(){return p_->listeners;}
const UCEntityRetirement&NativeMissionWorld::retirement()const{return p_->retirement;}
const NativeMissionServices&NativeMissionWorld::script_services()const{return p_->services;}
const float&NativeMissionWorld::time()const{return p_->elapsed;}
const float&NativeMissionWorld::unscaled_delta()const{return p_->other;}
uint32_t NativeMissionWorld::source_count()const{return (uint32_t)p_->metadata.size();}
uint32_t NativeMissionWorld::failure_source()const{return p_->failed_source;}
uint32_t NativeMissionWorld::failure_opcode()const{return p_->failed_opcode;}
const char*NativeMissionWorld::failure_class()const{
 if(p_->failed_source<p_->metadata.size())return p_->metadata[p_->failed_source].definition->class_name;
 // Dynamic PC owners publish their retained class name in definition70.
 auto*b=binding(p_->failed_source);return b&&b->view->definition70&&*b->view->definition70?static_cast<const char*>(*b->view->definition70):"";
}
int NativeMissionWorld::failure()const{return p_->error;}
