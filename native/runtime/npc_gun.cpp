#include "npc_gun.h"
#include "audio.h"
#include <DirectXMath.h>
#include <cmath>
#include <cstring>
#include <stdexcept>

struct NativeNpcGun::Impl {
    NativeNpcGun *owner;NativeMissionWorld *world=nullptr;NativeMissionCharacter *parent=nullptr;
    NativeRenderer *renderer=nullptr;const UCArchive *archive=nullptr;const uint8_t *hash=nullptr;
    std::filesystem::path catalog;UCArchiveLookup *lookup=nullptr;UCGameRandom *random=nullptr;
    UCParticleRegistry *registry=nullptr;
    const UCGunFlareServices *particle_service=nullptr;
    NativeNpcGunShotServices shot_services{};
    UCNpcGun gun{};UCNpcGunDescriptor descriptor{};NativeAudioBank bank;
    std::unique_ptr<NativeAnimatedModel> model;std::unique_ptr<NativeNpcFlare> flare;
    UCEntitySubscriptions *subscriptions=nullptr;float heat=0,previous[16]{};
    bool ready=false,removed=false;int failure=0;
    explicit Impl(NativeNpcGun *p):owner(p){}
    ~Impl(){uc_entity_subscriptions_close(subscriptions);}
    NativeMissionEntity &entity(){return owner->entity;}
    static int resolve(void*p,int32_t id,const UCEntityFreshView**out){auto&s=*(Impl*)p;
        auto*t=uc_entity_event_lookup(&s.world->registry(),id);auto&c=s.world->context();*out=t?c.resolve(c.owner,t):nullptr;return t&&!*out?1:0;}
    static int request_remove(void*p,const void*,uint32_t*){auto&s=*(Impl*)p;auto&e=s.entity();e.flags|=1;
        if(e.flags&8){auto&r=*s.world->context().registration;*r.creation={0,1,e.parent};
            if(r.insert_record(r.owner,&e.view.target,e.id,1))return s.failure=2;e.flags&=~8u;}return 0;}
    static int drop(void*p,const void*,uint32_t*){return ((Impl*)p)->failure=2;}
    UCNpcGunShotServices effects(){UCNpcGunShotServices h{};h.owner=this;h.flash_position=shot_services.flash_position;
        h.bullet=[](void*p,const UCWeaponBulletRequest*r,int32_t parent){auto&s=*(Impl*)p;return s.shot_services.bullet?s.shot_services.bullet(s.shot_services.owner,*r,parent):1;};
        h.sound=[](void*p,void*bank,const char*event,unsigned*found){auto&s=*(Impl*)p;if(bank!=&s.bank)return 1;
            *found=s.bank.find(event)>=0;if(!*found)return 0;if(!s.shot_services.sound)return 1;return s.shot_services.sound(s.shot_services.owner,*s.owner,s.gun.bankD0,event,found);};
        h.flare=[](void*p,int32_t target,int32_t sender,int32_t weapon,uint8_t secondary){auto&s=*(Impl*)p;struct{int32_t weapon;uint8_t secondary,pad[3];}payload{weapon,secondary,{}};
            uint32_t from=(uint32_t)sender;uint8_t handled=0;return int(uc_entity_event_send(&s.world->registry(),target,0x52,&payload,sizeof payload,&from,&handled)||!handled);};
        h.lightflash=[](void*p,const UCShotLightFlashRequest*r){auto&s=*(Impl*)p;return s.shot_services.lightflash?s.shot_services.lightflash(s.shot_services.owner,*r):1;};return h;}
    UCNpcGunServices services(){UCNpcGunServices h{};h.owner=this;
        h.health_base=[](void*p){auto&s=*(Impl*)p;auto context=s.world->context();const void*definition="Gun";
            context.definition=&definition;*context.registration->creation={0,1,s.parent->entity.id};
            return uc_entity_health_construct_matrix(&context,&s.entity().view,s.descriptor.world);};
        h.listener_reset=[](void*p){auto&s=*(Impl*)p;return uc_entity_subscriptions_create(&s,&s.subscriptions);};
        h.property=[](void*p,unsigned type,const char*name,UCNpcGunProperty*out,unsigned*found){auto&d=((Impl*)p)->descriptor;*found=1;
            if(type==1&&!strcmp(name,"WeaponType"))out->integer=d.weapon;
            else if(type==2&&!strcmp(name,"DamageAmount"))out->number=d.damage;
            else if(type==1&&!strcmp(name,"DamageTeam"))out->integer=d.team;
            else if(type==3&&!strcmp(name,"Hand"))out->integer=d.hand;
            else if(type==1&&!strcmp(name,"Owner"))out->integer=d.owner;
            else if(type==6&&!strcmp(name,"SoundBank"))out->string=d.bank;else return 1;return 0;};
        h.bank=[](void*p,const char*name,void**out){auto&s=*(Impl*)p;if(!name)return 1;BytesOwner bytes;size_t at=find_resource(s.archive,name);
            if(at==SIZE_MAX||uc_archive_read(s.archive,at,UC_AR_LOCAL_PAYLOAD,&bytes.value)!=UC_AR_OK)return 1;
            s.bank.load(bytes.value.data,bytes.value.size);*out=&s.bank;return 0;};
        h.resolve=resolve;
        h.asset=[](void*p,const char**out){*out=((Impl*)p)->descriptor.asset;return 0;};
        h.model_load=[](void*p,const char*name){auto&s=*(Impl*)p;s.model=std::make_unique<NativeAnimatedModel>();
            s.model->load(*s.renderer,s.archive,s.hash,s.catalog,name,nullptr,nullptr,s.lookup);
            for(const auto&m:s.model->materials)if(!m.supported())return 1;
            const float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};s.model->bind_pose(*s.renderer,identity);
            s.entity().model=s.model.get();return 0;};
        h.flare=[](void*p,int32_t weapon,int32_t*out){auto&s=*(Impl*)p;if(weapon!=2||s.flare)return 1;
            *s.world->context().registration->creation={0,1,s.entity().id};s.flare=std::make_unique<NativeNpcFlare>();
            int r=s.flare->construct(*s.renderer,*s.world,s.entity().id,s.archive,s.hash,s.catalog,*s.lookup,*s.random,*s.registry,s.particle_service);
            if(!r)*out=s.flare->entity.id;return r;};
        h.tag=[](void*p,const UCEntityFreshView*v,const char*name,int32_t*out){auto&s=*(Impl*)p;
            if(v==&s.parent->entity.view){*out=s.parent->bone_index(name);return 0;}
            if(v!=&s.entity().view||!s.model)return 1;*out=-1;
            for(uint32_t i=0;i<s.model->source.bone_count;++i)if(!_strnicmp((const char*)s.model->source.bones[i].source_metadata+4,name,16)){*out=(int32_t)i;break;}return 0;};
        h.listener_register=[](void*p,int32_t id)->int{auto&s=*(Impl*)p;int inserted=0;
            return uc_entity_listener_register(&s.world->registry(),id,id,uc_entity_subscriptions_listener(s.subscriptions),s.world->listeners(),&inserted)||!inserted;};
        h.listener_bind=[](void*p,unsigned event){auto&s=*(Impl*)p;if(event>1)return 1;
            UCEntityEventSubscription subscription{event?218u:215u,event?request_remove:drop};return uc_entity_subscriptions_append(s.subscriptions,&subscription);};
        h.priority=[](void*p,int32_t id,unsigned priority){return ((Impl*)p)->world->set_priority(id,(int32_t)priority);};
        h.delta=[](void*p,float*out){auto&s=*(Impl*)p;if(!s.world->script_services().delta)return 1;*out=*s.world->script_services().delta;return 0;};
        //21AB30 consumes model+18 only in thermal mode. Normal rendering uses
        //the separate model+20 tint; keep heat without darkening normal guns.
        h.model_heat=[](void*p,void*model,float value){auto&s=*(Impl*)p;if(model!=s.model.get())return 1;s.heat=value<0?0:value>1?1:value;return 0;};
        h.health_update=[](void*p){auto&e=((Impl*)p)->entity();if(e.flags&0x800||e.helper||!e.aux)return 1;return uc_entity_fresh_aux_clear(e.aux);};
        h.pose_tag=[](void*p,const UCEntityFreshView*v,int32_t index,float*out,unsigned*found){auto&s=*(Impl*)p;
            if(v==&s.parent->entity.view){*found=s.parent->posed_bone_local(index,out);return 0;}
            if(v!=&s.entity().view||!s.model)return 1;*found=0;
            if(index>=0&&uint32_t(index)<s.model->source.bone_count&&s.model->prepared.ready&&s.model->prepared.world_valid[index]){
                std::memcpy(out,s.model->prepared.world[index],64);*found=1;}return 0;};
        h.set_world=[](void*p,const float*m){std::memcpy(((Impl*)p)->entity().transform,m,64);return 0;};
        h.time=[](void*p,float*out){*out=((Impl*)p)->world->time();return 0;};
        h.player_count=[](void*)->int32_t{return 1;}; //Current native session has one player slot, even while absent.
        h.player_id=[](void*p){return ((Impl*)p)->shot_services.player_id;};
        h.remove=[](void*p){return ((Impl*)p)->owner->remove();};return h;
    }
};
NativeNpcGun::NativeNpcGun():p_(std::make_unique<Impl>(this)){}
NativeNpcGun::~NativeNpcGun()=default;
int NativeNpcGun::construct(NativeRenderer&r,NativeMissionWorld&w,NativeMissionCharacter&parent,const UCArchive*a,
 const uint8_t hash[32],const std::filesystem::path&catalog,UCArchiveLookup&lookup,UCGameRandom&random,UCParticleRegistry&registry,
 const UCGunFlareServices*particle_service){
    auto&s=*p_;if(s.world||!a||!hash||lookup.archive!=a||uc_entity_event_lookup(&w.registry(),parent.entity.id)!=&parent.entity.identity)return 1;
    float dummy[16];if(!parent.posed_bone_local(parent.bone_index("gundummy"),dummy)||uc_npc_gun_descriptor(dummy,parent.entity.transform,parent.entity.id,&random,&s.descriptor))return 1;
    s.world=&w;s.parent=&parent;s.renderer=&r;s.archive=a;s.hash=hash;s.catalog=catalog;s.lookup=&lookup;s.random=&random;s.registry=&registry;
    s.particle_service=particle_service;
    if(w.bind_dynamic({&entity.view,this,[](void*p){return ((NativeNpcGun*)p)->update();},[](void*p){return ((NativeNpcGun*)p)->late();},
        [](void*p){return ((NativeNpcGun*)p)->close_detached();},nullptr,[](void*p){return ((NativeNpcGun*)p)->remove();},[](void*p){return ((NativeNpcGun*)p)->close_detached();}}))return s.failure=2;
    auto h=s.services();if(uc_npc_gun_construct(&s.gun,&entity.view,&h))return s.failure=2;
    std::memcpy(s.previous,entity.transform,64);s.ready=true;return 0;
}
int NativeNpcGun::update(){auto&s=*p_;if(!ready())return s.failure?s.failure:1;auto h=s.services();return uc_npc_gun_update(&s.gun,&entity.view,&h)?s.failure=2:0;}
int NativeNpcGun::late(){auto&s=*p_;if(!ready())return s.failure?s.failure:1;std::memcpy(s.previous,entity.transform,64);auto h=s.services();
    if(uc_npc_gun_attached(&s.gun,&entity.view,&h)){
        const UCEntityFreshView*parent=nullptr;float bone[16];unsigned found=0;h.resolve(&s,entity.parent,&parent);
        if(parent)h.pose_tag(&s,parent,s.gun.tags[s.gun.handE4?0:1],bone,&found);
        s.failure=2;throw std::runtime_error("Gun attachment: parent "+std::to_string(entity.parent)+
            " present "+std::to_string(uc_entity_event_lookup(&s.world->registry(),entity.parent)!=nullptr)+
            " bone "+std::to_string(found)+" remove "+std::to_string(s.gun.removeF0)+" dropped "+std::to_string(s.gun.droppedF4));}return 0;}
int NativeNpcGun::remove(){auto&s=*p_;if(!s.world||s.removed||entity.helper||!entity.aux||entity.source!=-1)return 1;
    if(uc_entity_fresh_aux_clear(entity.aux))return s.failure=2;entity.flags|=0x8100;uint8_t queued=0;
    if(uc_entity_retire(&s.world->retirement(),entity.id,&queued)||!queued)return s.failure=2;
    if(entity.identity.listeners[0]){if(uc_entity_listener_list_remove_first(s.world->listeners(),entity.id))return s.failure=2;
        for(auto&listener:entity.identity.listeners)listener=nullptr;}entity.flags|=0x80000;s.removed=true;return 0;
}
int NativeNpcGun::close_detached(){auto&s=*p_;if(!s.world)return 0;if(uc_entity_event_lookup(&s.world->registry(),entity.id))return 1;
    if(entity.identity.listeners[0]){if(uc_entity_listener_list_remove_first(s.world->listeners(),entity.id))return 1;
        for(auto&listener:entity.identity.listeners)listener=nullptr;}
    uc_entity_subscriptions_close(s.subscriptions);s.subscriptions=nullptr;s.model.reset();entity.model=nullptr;
    if(entity.aux&&uc_entity_fresh_aux_destroy(entity.aux))return 1;entity.aux=nullptr;s.ready=false;s.world=nullptr;return 0;
}
bool NativeNpcGun::ready()const{return p_->ready&&!p_->failure&&!p_->removed;}
const UCNpcGun&NativeNpcGun::state()const{return p_->gun;}
void NativeNpcGun::set_shot_services(const NativeNpcGunShotServices&s){p_->shot_services=s;}
int NativeNpcGun::fire_position(float*out){auto&s=*p_;if(!ready()||!out)return 1;float local[16];unsigned found=0;auto h=s.services();
    if(s.model&&h.pose_tag(&s,&entity.view,s.gun.tags[2],local,&found))return s.failure=2;
    return uc_npc_gun_fire_position(entity.transform,found?local+12:nullptr,found,out);}
int NativeNpcGun::gun_matrix(float*out){auto&s=*p_;if(!ready())return 1;unsigned found=0;auto h=s.services();int result=uc_npc_gun_matrix(&s.gun,&entity.view,&h,out,&found);return result?result:found?0:2;}
int NativeNpcGun::reload_sound(){auto&s=*p_;if(!ready())return 1;auto h=s.effects();return uc_npc_gun_reload_sound(&s.gun,&h)?s.failure=2:0;}
int NativeNpcGun::shoot(const float*d,float scale,unsigned hit){auto&s=*p_;if(!ready())return 1;auto h=s.services();auto effects=s.effects();return uc_npc_gun_shoot(&s.gun,&entity.view,d,0,1,1,0,scale,hit,&h,&effects)?s.failure=2:0;}
void NativeNpcGun::draw_effects(NativeRenderer&r,const float*vp,float exposure){if(ready()&&p_->flare)p_->flare->draw(r,vp,exposure);}
void NativeNpcGun::draw(NativeRenderer&r,const float*vp,float exposure,float fraction,NativeModelLighting *lighting,const NativeLightingFrame *frame){if(!ready())return;
    float transform[16];native_interpolate_transform(p_->previous,entity.transform,fraction,transform);DirectX::XMFLOAT4X4 a,b,result;std::memcpy(&a,transform,64);std::memcpy(&b,vp,64);
    DirectX::XMStoreFloat4x4(&result,DirectX::XMLoadFloat4x4(&a)*DirectX::XMLoadFloat4x4(&b));const float white[4]={1,1,1,1};NativeTextureContext texture{r.material_phase};
    if(lighting&&frame)p_->model->draw_lit(r,*lighting,*frame,transform,&result._11,white,&texture,exposure);else p_->model->draw(r,&result._11,white,&texture,exposure);
}
