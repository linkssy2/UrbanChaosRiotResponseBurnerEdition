#include "npc_thrown_weapon.h"
#include "audio.h"
extern "C" {
#include "../npc_thrown_sequence.h"
}
#include <DirectXMath.h>
#include <cstring>
#include <cmath>
#include <stdexcept>
namespace {
void require(bool v,const char*text){if(!v)throw std::runtime_error(text);}
void multiply(const float*a,const float*b,float*out){for(unsigned r=0;r<4;r++)for(unsigned c=0;c<4;c++)
    out[r*4+c]=(a[r*4]*b[c]+a[r*4+1]*b[c+4])+(a[r*4+2]*b[c+8]+a[r*4+3]*b[c+12]);}
const float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
}
struct NativeNpcThrownWeapon::Impl {
    NativeNpcThrownWeapon*owner;NativeMissionWorld*world=nullptr;NativeMissionCharacter*parent=nullptr;
    int32_t parent_id=-1;
    NativeRenderer*renderer=nullptr;const UCArchive*archive=nullptr;const uint8_t*hash=nullptr;
    std::filesystem::path catalog;UCArchiveLookup*lookup=nullptr;
    NativeNpcThrownDescriptor descriptor{};NativeNpcThrownServices services{};
    NativeAudioBank bank;std::unique_ptr<NativeAnimatedModel> model;NativeCoroutine coroutine;
    UCEntitySubscriptions*subscriptions=nullptr;float previous[16]{},tint[4]{1,1,1,1};
    bool constructed=false,removed=false,visible=true,pickup=false,pickup_settled=false;float pickup_speed=0;int failure=0;
    explicit Impl(NativeNpcThrownWeapon*p):owner(p){}
    ~Impl(){uc_entity_subscriptions_close(subscriptions);}
    void check(int result,const char*name){if(result){failure=2;throw std::runtime_error(name);}}
    float delta()const{require(world&&world->script_services().delta,"ThrownWeapon world clock");return *world->script_services().delta;}
    UCNpcThrownServices policy(){UCNpcThrownServices h{};h.owner=this;
        h.base=[](void*p){auto&s=*(Impl*)p;auto c=s.world->context();const void*definition="ThrownWeapon";
            c.definition=&definition;*c.registration->creation={0,1,s.parent_id};
            return uc_bullet_construct_matrix(&c,&s.owner->entity.view,&s.owner->base,s.descriptor.world);};
        h.listener_reset=[](void*p){auto&s=*(Impl*)p;return uc_entity_subscriptions_create(&s,&s.subscriptions);};
        h.coroutine_initialize=[](void*p){return ((Impl*)p)->coroutine.live()?1:0;};
        h.property=[](void*p,unsigned type,const char*name,UCNpcThrownProperty*out,unsigned*found){auto&d=((Impl*)p)->descriptor;*found=1;
            if(type==1&&!strcmp(name,"WeaponType"))out->integer=d.weapon;
            else if(type==2&&!strcmp(name,"WeaponDamage"))out->number=d.damage;
            else if(type==1&&!strcmp(name,"DamageInfoTeam"))out->integer=d.team;
            else if(type==6&&!strcmp(name,"SoundBank"))out->string=d.bank;
            else if(type==3&&!strcmp(name,"Secondary Weapon"))out->integer=d.secondary;
            else if(type==1&&!strcmp(name,"Owner"))out->integer=d.owner;
            else if(type==1&&!strcmp(name,"Texture variation"))out->integer=d.variation;else return 1;return 0;};
        h.bank=[](void*p,const char*name,void**out){auto&s=*(Impl*)p;if(!name)return 1;BytesOwner bytes;size_t at=find_resource(s.archive,name);
            if(at==SIZE_MAX||uc_archive_read(s.archive,at,UC_AR_LOCAL_PAYLOAD,&bytes.value)!=UC_AR_OK)return 1;
            s.bank.load(bytes.value.data,bytes.value.size);*out=&s.bank;return 0;};
        h.physics_construct=[](void*p){auto&s=*(Impl*)p;return s.services.physics_construct?s.services.physics_construct(s.services.owner,*s.owner):1;};
        h.coroutine_schedule=[](void*p){auto&s=*(Impl*)p;return s.coroutine.create([](void*q,NativeCoroutine&c){((Impl*)q)->sequence(c);},&s);};
        h.damage=[](void*p,float amount){((Impl*)p)->owner->entity.actor.valueA4=amount;return 0;};
        h.listener_register=[](void*p,int32_t id)->int{auto&s=*(Impl*)p;int inserted=0;return uc_entity_listener_register(&s.world->registry(),id,id,
            uc_entity_subscriptions_listener(s.subscriptions),s.world->listeners(),&inserted)||!inserted;};
        h.listener_bind=[](void*p,unsigned index){auto&s=*(Impl*)p;UCEntityEventSubscription row{};
            if(index==0){row.event=217;row.receive=[](void*q,const void*v,uint32_t*){if(!v)return 1;return ((Impl*)q)->owner->ignite(*(const float*)v);};}
            else if(index==1){row.event=221;row.receive=[](void*q,const void*v,uint32_t*){if(!v)return 1;auto*f=(const float*)v;return ((Impl*)q)->owner->launch(f,f[3],.1f);};}
            else if(index==2){row.event=218;row.receive=[](void*q,const void*,uint32_t*){return ((Impl*)q)->set_flags(1);};}else return 1;
            return uc_entity_subscriptions_append(s.subscriptions,&row);};
        h.find_template=[](void*p,const char*name,void**out){auto&s=*(Impl*)p;const int32_t*ids;uint32_t count;
            if(uc_entity_names_view(s.world->names(),name,&ids,&count))return 1;*out=nullptr;
            if(count&&ids[0]!=-1){auto*t=uc_entity_event_lookup(&s.world->registry(),ids[0]);if(t){auto&c=s.world->context();*out=(void*)c.resolve(c.owner,t);if(!*out)return 1;}}return 0;};
        h.name_add=[](void*p,const char*name,int32_t id){return uc_entity_names_bind(((Impl*)p)->world->names(),name,id);};
        h.physics_ignite=[](void*p,void*body,float seconds){auto&s=*(Impl*)p;return s.services.physics_ignite?s.services.physics_ignite(s.services.owner,body,seconds):1;};
        h.physics_source_owned=[](void*p,void*body,unsigned*out){auto&s=*(Impl*)p;return s.services.physics_source_owned?s.services.physics_source_owned(s.services.owner,body,out):1;};
        h.physics_launch=[](void*p,void*body,const float*v,float d){auto&s=*(Impl*)p;return s.services.physics_launch?s.services.physics_launch(s.services.owner,body,v,d):1;};
        h.dropped_template=[](void*p){auto&s=*(Impl*)p;
            if(s.services.dropped_template)return s.services.dropped_template(s.services.owner,*s.owner);
            if(s.owner->state.weapon1EC!=11||!s.services.pickup_floor||!s.services.pickup)return 1;
            //ponytail: retain the actual bottle/identity instead of porting
            //DroppedWeapon's Havok body. Only unarmed Molotovs are admitted.
            //Finish real model creation while still held, then own its fall.
            if(!s.model&&s.coroutine.resume()<0)return 1;
            if(!s.model||s.coroutine.close())return 1;
            s.active_coroutine=nullptr;s.pickup=true;s.pickup_speed=0;
            return uc_entity_names_bind(s.world->names(),"WEAPON_PICKUP",s.owner->entity.id);};
        h.set_flags=[](void*p,uint32_t flags){return ((Impl*)p)->set_flags(flags);};return h;
    }
    int set_flags(uint32_t flags){auto&e=owner->entity;e.flags|=flags;if((flags&1)&&(e.flags&8)){
        auto&r=*world->context().registration;*r.creation={0,1,e.parent};if(r.insert_record(r.owner,&e.view.target,e.id,1))return failure=2;e.flags&=~8u;}return 0;}
    void sequence(NativeCoroutine&c){
        UCNpcThrownSequenceServices h{};h.owner=this;active_coroutine=&c;
        h.template_asset=[](void*p,void*source,const char*name,const char**asset,unsigned*found){auto&s=*(Impl*)p;
            return s.services.template_property?s.services.template_property(s.services.owner,(const UCEntityFreshView*)source,name,asset,found):1;};
        h.model_load=[](void*p,const char*asset){auto&s=*(Impl*)p;require(asset,"ThrownWeapon model asset");s.model=std::make_unique<NativeAnimatedModel>();
            s.model->load(*s.renderer,s.archive,s.hash,s.catalog,asset,nullptr,nullptr,s.lookup);s.model->select_skin(s.owner->state.variation240);
            for(const auto&m:s.model->materials)require(m.supported(),"ThrownWeapon material");s.model->bind_pose(*s.renderer,identity);s.owner->entity.model=s.model.get();return 0;};
        h.visible=[](void*p,unsigned visible){((Impl*)p)->visible=visible!=0;return 0;};
        h.yield=[](void*p){return ((Impl*)p)->active_coroutine->yield();};
        h.ignition=[](void*p){auto&s=*(Impl*)p;s.check(s.services.ignition?s.services.ignition(s.services.owner,*s.owner):1,"ThrownWeapon ignition");return 0;};
        h.flight=[](void*p){auto&s=*(Impl*)p;s.check(s.services.flight?s.services.flight(s.services.owner,*s.owner):1,"ThrownWeapon flight");return 0;};
        h.explosion=[](void*p){auto&s=*(Impl*)p;s.check(s.services.explosion?s.services.explosion(s.services.owner,*s.owner):1,"ThrownWeapon explosion");return 0;};
        h.delta=[](void*p,double*out){*out=((Impl*)p)->delta();return 0;};h.flags=[](void*p,uint32_t flags){return ((Impl*)p)->set_flags(flags);};
        h.pickup=[](void*p){auto&s=*(Impl*)p;if(uc_entity_names_bind(s.world->names(),"WEAPON_PICKUP",s.owner->entity.id))return 1;
            std::memcpy(s.owner->entity.transform,identity,48);s.owner->entity.transform[13]+=9;s.owner->entity.transform[15]=1;return 0;};
        h.land_sound=[](void*p){auto&s=*(Impl*)p;return s.services.land_sound?s.services.land_sound(s.services.owner,*s.owner):1;};
        h.sound_active=[](void*p,unsigned*out){auto&s=*(Impl*)p;s.check(s.services.sound_active?s.services.sound_active(s.services.owner,*s.owner,out):1,"ThrownWeapon sound lifetime");return 0;};
        h.remove=[](void*p){return ((Impl*)p)->owner->remove();};
        check(uc_npc_thrown_sequence(&owner->state,&owner->base,&h),"ThrownWeapon source coroutine");active_coroutine=nullptr;
    }
    NativeCoroutine*active_coroutine=nullptr;

};
NativeNpcThrownWeapon::NativeNpcThrownWeapon():p_(std::make_unique<Impl>(this)){}
NativeNpcThrownWeapon::~NativeNpcThrownWeapon()=default;
int NativeNpcThrownWeapon::construct(NativeRenderer&r,NativeMissionWorld&w,NativeMissionCharacter&parent,const UCArchive*a,
 const uint8_t*hash,const std::filesystem::path&catalog,UCArchiveLookup&lookup,const NativeNpcThrownDescriptor&d,const NativeNpcThrownServices&services){
    if(uc_entity_event_lookup(&w.registry(),parent.entity.id)!=&parent.entity.identity)return 1;
    int result=construct(r,w,parent.entity.id,a,hash,catalog,lookup,d,services);if(!result)p_->parent=&parent;return result;
}
int NativeNpcThrownWeapon::construct(NativeRenderer&r,NativeMissionWorld&w,int32_t parent,const UCArchive*a,
 const uint8_t*hash,const std::filesystem::path&catalog,UCArchiveLookup&lookup,const NativeNpcThrownDescriptor&d,const NativeNpcThrownServices&services){
    auto&s=*p_;if(s.world||!a||!hash||lookup.archive!=a||!services.physics_construct||
        !uc_entity_event_lookup(&w.registry(),parent))return 1;
    for(float value:d.world)if(!std::isfinite(value))return 1;
    s.world=&w;s.parent_id=parent;s.renderer=&r;s.archive=a;s.hash=hash;s.catalog=catalog;s.lookup=&lookup;s.descriptor=d;s.services=services;
    if(w.bind_dynamic({&entity.view,this,[](void*p){return ((NativeNpcThrownWeapon*)p)->update();},[](void*p){return ((NativeNpcThrownWeapon*)p)->late();},
        [](void*p){return ((NativeNpcThrownWeapon*)p)->close_detached();},nullptr,[](void*p){return ((NativeNpcThrownWeapon*)p)->remove();},
        [](void*p){return ((NativeNpcThrownWeapon*)p)->close_detached();}}))return s.failure=2;
    auto h=s.policy();if(uc_npc_thrown_construct(&state,&base,&entity.view,&h))return s.failure=2;
    std::memcpy(s.previous,entity.transform,64);s.constructed=true;return 0;
}
int NativeNpcThrownWeapon::update(){auto&s=*p_;if(!ready())return s.failure?s.failure:1;if(s.pickup)return 0;if(s.coroutine.resume()<0)return s.failure=2;
    unsigned held=1;if(state.physics250){auto h=s.policy();if(h.physics_source_owned(&s,state.physics250,&held))return s.failure=2;}
    if(held){if(uc_bullet_motion_begin_frame(&base.motion,entity.transform)||entity.helper||entity.actor.handleBC||entity.actor.audioC0||entity.actor.scriptC8||
        entity.actor.flagCC||entity.actor.flagCD||entity.actor.flagCE||!entity.aux||uc_entity_fresh_aux_clear(entity.aux))return s.failure=2;}return 0;
}
int NativeNpcThrownWeapon::late(){auto&s=*p_;s.check(!ready(),"ThrownWeapon late lifetime");std::memcpy(s.previous,entity.transform,64);
    if(s.pickup){if(!s.pickup_settled){float from[3],to[3],point[3];std::memcpy(from,entity.transform+12,12);std::memcpy(to,from,12);
            s.pickup_speed-=980.f*s.delta();to[1]+=s.pickup_speed*s.delta();uint32_t material=0;
            int hit=s.services.pickup_floor(s.services.owner,from,to,point,&material);s.check(hit<0,"Dropped Molotov floor query");
            if(hit){std::memcpy(entity.transform+12,point,12);entity.transform[13]+=9;state.material214=material;
                s.pickup_settled=true;state.settled202=1;std::memcpy(entity.transform,identity,48);
                s.check(!s.services.land_sound||s.services.land_sound(s.services.owner,*this),"Dropped Molotov landing sound");}
            else std::memcpy(entity.transform+12,to,12);}
        if(s.pickup_settled){unsigned taken=0;s.check(s.services.pickup(s.services.owner,*this,&taken)||taken>1,"Dropped Molotov inventory admission");if(taken)return remove();}
        return 0;}
    if(!state.exploded200&&state.armed1F0){double remaining=double(state.fuse1F4)-s.delta();state.fuse1F4=float(remaining);
        if(remaining<=0&&state.weapon1EC!=8&&state.weapon1EC!=32&&state.weapon1EC!=33){state.exploded200=1;s.set_flags(0x100);
            if(!state.launched1F8){if(state.weapon1EC==11){if(!s.services.owner_fuse_expired||s.services.owner_fuse_expired(s.services.owner,*this)){
                s.failure=2;throw std::runtime_error("ThrownWeapon held fuse-expiry burn54 owner="+std::to_string(base.actor)+
                    " parent="+std::to_string(entity.parent)+" status="+std::to_string(s.parent?s.parent->status():UINT32_MAX)+
                    " health="+std::to_string(s.parent?s.parent->entity.actor.health9C:0));}}
                state.launched1F8=1;}else state.armed1F0=0;}}
    if(!state.launched1F8&&!state.exploded200){unsigned held=1;if(state.physics250){auto h=s.policy();s.check(h.physics_source_owned(&s,state.physics250,&held),"ThrownWeapon held physics ownership");}
        if(held){auto*t=uc_entity_event_lookup(&s.world->registry(),entity.parent);if(t){s.check(!s.parent||t!=&s.parent->entity.identity,"ThrownWeapon held parent identity");
            float bone[16],local[16];s.check(!s.parent->posed_bone(state.secondary218?"Bip01 L Hand":"gundummy",bone),"ThrownWeapon held posed bone");
            multiply(identity,bone,local);multiply(local,s.parent->entity.transform,entity.transform);}}}
    if(state.launched1F8||state.settled202||state.dropped210||state.effect248)s.check(s.services.flight_late?s.services.flight_late(s.services.owner,*this):1,"ThrownWeapon late flight");
    if(state.physics250)s.check(!s.services.physics_sync||s.services.physics_sync(s.services.owner,*this),"ThrownWeapon physics sync DBFC0");return 0;
}
int NativeNpcThrownWeapon::ignite(float t){auto h=p_->policy();return uc_npc_thrown_ignite(&state,t,&h);}
void NativeNpcThrownWeapon::set_fuse(float t){uc_npc_thrown_set_fuse(&state,t);}
int NativeNpcThrownWeapon::launch(const float*v,float d,float spin){auto h=p_->policy();return uc_npc_thrown_launch(&state,v,d,spin,&h);}
int NativeNpcThrownWeapon::drop(){auto h=p_->policy();return uc_npc_thrown_drop(&state,&h);}
int NativeNpcThrownWeapon::set_flags(uint32_t flags){return p_->set_flags(flags);}
NativeNpcThrownView NativeNpcThrownWeapon::view()const{NativeNpcThrownView v;std::memcpy(v.position,entity.transform+12,12);v.fuse=uc_npc_thrown_fuse(&state);v.bounced=uc_npc_thrown_settled(&state)!=0;v.exploded=uc_npc_thrown_exploded(&state)!=0;return v;}
bool NativeNpcThrownWeapon::ready()const{return p_->constructed&&!p_->removed&&!p_->failure;}
bool NativeNpcThrownWeapon::model_tint(float*out)const{if(!out||!p_->model)return false;std::memcpy(out,p_->tint,16);return true;}
int NativeNpcThrownWeapon::set_model_tint(const float*value){if(!value||!p_->model)return 1;std::memcpy(p_->tint,value,16);return 0;}
int32_t NativeNpcThrownWeapon::bone_index(const char*name)const{if(!p_->model||!name)return -1;auto&m=*p_->model;
    for(uint32_t i=0;i<m.source.bone_count;i++)if(!_strnicmp((const char*)m.source.bones[i].source_metadata+4,name,16))return (int32_t)i;return -1;}
bool NativeNpcThrownWeapon::posed_bone(int32_t index,float*out)const{if(!p_->model||!out||index<0)return false;auto&m=*p_->model;
    if(uint32_t(index)>=m.source.bone_count||!m.prepared.ready||!m.prepared.world_valid[index])return false;std::memcpy(out,m.prepared.world[index],64);return true;}
int NativeNpcThrownWeapon::remove(){auto&s=*p_;if(!s.world||s.removed||entity.helper||!entity.aux||entity.source!=-1)return 1;
    if(uc_entity_names_remove(s.world->names(),"WEAPON_PICKUP",entity.id))return s.failure=2;
    if((state.weapon1EC==8||state.weapon1EC==11)&&uc_entity_names_remove(s.world->names(),"EXTREMELY_INTERESTING_LOOK",entity.id))return s.failure=2;
    if(uc_entity_fresh_aux_clear(entity.aux))return s.failure=2;entity.flags|=0x8100;uint8_t queued;
    if(uc_entity_retire(&s.world->retirement(),entity.id,&queued)||!queued)return s.failure=2;
    if(entity.identity.listeners[0]){if(uc_entity_listener_list_remove_first(s.world->listeners(),entity.id))return s.failure=2;for(auto&l:entity.identity.listeners)l=nullptr;}
    entity.flags|=0x80000;s.removed=true;return 0;
}
int NativeNpcThrownWeapon::close_detached(){auto&s=*p_;if(!s.world)return 0;if(uc_entity_event_lookup(&s.world->registry(),entity.id))return 1;
    if(state.effect248){if(!s.services.release_light||s.services.release_light(s.services.owner,state.effect248))return 1;state.effect248=nullptr;}
    if(state.physics250&&(!s.services.physics_close||s.services.physics_close(s.services.owner,*this)))return 1;if(s.coroutine.close())return 1;
    if(entity.identity.listeners[0]){if(uc_entity_listener_list_remove_first(s.world->listeners(),entity.id))return 1;for(auto&l:entity.identity.listeners)l=nullptr;}
    uc_entity_subscriptions_close(s.subscriptions);s.subscriptions=nullptr;s.model.reset();entity.model=nullptr;
    if(entity.aux&&uc_entity_fresh_aux_destroy(entity.aux))return 1;entity.aux=nullptr;s.constructed=false;s.world=nullptr;return 0;
}
void NativeNpcThrownWeapon::draw(NativeRenderer&r,const float*vp,float exposure,float fraction,NativeModelLighting *lighting,const NativeLightingFrame *frame){auto&s=*p_;if(!ready()||!s.model||!s.visible)return;
    float transform[16];native_interpolate_transform(s.previous,entity.transform,fraction,transform);DirectX::XMFLOAT4X4 a,b,result;
    std::memcpy(&a,transform,64);std::memcpy(&b,vp,64);DirectX::XMStoreFloat4x4(&result,DirectX::XMLoadFloat4x4(&a)*DirectX::XMLoadFloat4x4(&b));
    NativeTextureContext texture{r.material_phase};if(lighting&&frame)s.model->draw_lit(r,*lighting,*frame,transform,&result._11,s.tint,&texture,exposure);else s.model->draw(r,&result._11,s.tint,&texture,exposure);
}
