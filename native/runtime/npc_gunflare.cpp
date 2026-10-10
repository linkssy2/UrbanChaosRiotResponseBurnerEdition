#include "npc_gunflare.h"
extern "C" {
#include "../texture_resources.h"
}
#include <cstring>
#include <DirectXMath.h>

static const char *const names[]={
 "Assets_PlayerWeaponsAndEquipment\\EQP_Police_Assault_Riffle\\EQP_AssaultRiffle_muzzleflash_FP_01_Skin.amf",
 "Assets_PlayerWeaponsAndEquipment\\EQP_AssaultRifle\\EQP_AssaultRifle_Muzzleflash_TP.mtxt",
 "Assets_PlayerWeaponsAndEquipment\\EQP_ShotGun\\EQP_doubleshotgun_muzzleflash_FP_01_Skin.amf"};
struct NativeNpcFlare::Impl {
    struct Instance {NativeAnimatedModel model;float world[16]{},tint[4]{1,1,1,1};bool visible=false;};
    NativeNpcFlare *owner;NativeMissionWorld *world=nullptr;UCParticleRegistry *registry=nullptr;UCGameRandom *random=nullptr;
    UCGunFlare flare{};UCGunFlareModels groups[3]{};void *handles[3]{};
    std::unique_ptr<Instance> models[3];UCEntitySubscriptions *subscriptions=nullptr;
    UCTextureAsset texture{};UCTextureResource resource{};const UCTextureResource *sprite_texture=nullptr;
    ComPtr<ID3D11ShaderResourceView> uploaded_texture;
    float radius=0;UCGunFlareSpriteSource sprite{&flare.visible,&flare.sprite_enabled,&sprite_texture,flare.sprite_position,&radius};
    uintptr_t token=0;bool ready=false,removed=false;int failure=0;
    const UCGunFlareServices *particle_service=nullptr;
    explicit Impl(NativeNpcFlare *o):owner(o){flare.selected=UINT32_MAX;}
    ~Impl(){uc_texture_free(&texture);uc_entity_subscriptions_close(subscriptions);}
    static int unsupported(void*p,const void*,uint32_t*){((Impl*)p)->failure=2;return 1;}
    static int shot(void*p,const void*payload,uint32_t*){auto&s=*(Impl*)p;uint32_t item=0;
        if(payload)std::memcpy(&item,payload,4);
        if(!s.ready||s.removed||s.failure||item!=2||!s.particle_service||!s.particle_service->particles)return s.failure=2;
        std::memcpy(s.flare.world,s.owner->entity.transform,64);auto h=s.services();
        return uc_gunflare_shot(&s.flare,s.groups,3,item,s.random,&h)?s.failure=2:0;}
    static int particles(void*p,const char*name,const float*matrix,UCGameRandom*random){auto&s=*(Impl*)p;
        return s.particle_service->particles(s.particle_service->user,name,matrix,random);}
    static int tint(void*p,void*model,const float*rgba){auto&s=*(Impl*)p;
        for(auto&i:s.models)if(i&&i.get()==model){std::memcpy(i->tint,rgba,16);return 0;}return 1;}
    static int visible(void*p,void*model,unsigned enabled){auto&s=*(Impl*)p;
        for(auto&i:s.models)if(i&&i.get()==model){i->visible=enabled!=0;return 0;}return 1;}
    static int transform(void*p,void*model,const float*matrix){auto&s=*(Impl*)p;
        for(auto&i:s.models)if(i&&i.get()==model){std::memcpy(i->world,matrix,64);return 0;}return 1;}
    static int parent(void*p,int32_t id,unsigned*out){auto&s=*(Impl*)p;*out=uc_entity_event_lookup(&s.world->registry(),id)!=nullptr;return 0;}
    static int remove(void*p){return ((Impl*)p)->owner->remove();}
    UCGunFlareServices services(){return {this,particles,tint,visible,transform,parent,remove};}
};
NativeNpcFlare::NativeNpcFlare():p_(std::make_unique<Impl>(this)){entity.view.actor=nullptr;entity.view.aux84=nullptr;}
NativeNpcFlare::~NativeNpcFlare()=default;
int NativeNpcFlare::construct(NativeRenderer&r,NativeMissionWorld&w,int32_t parent,const UCArchive*a,
 const uint8_t hash[32],const std::filesystem::path&catalog,UCArchiveLookup&lookup,UCGameRandom&random,UCParticleRegistry&registry,
 const UCGunFlareServices*particle_service){
    auto&s=*p_;const auto&ctx=w.context();
    if(s.world||!a||!hash||lookup.archive!=a||random.first>=250||random.second>=250||parent==-1||
       !ctx.registration||ctx.registration->creation->parent_id!=parent||!uc_entity_event_lookup(&w.registry(),parent))return 1;
    s.world=&w;s.registry=&registry;s.random=&random;s.particle_service=particle_service;
    if(w.bind_dynamic({&entity.view,this,[](void*p){return ((NativeNpcFlare*)p)->update();},
       [](void*p){return ((NativeNpcFlare*)p)->late();},[](void*p){return ((NativeNpcFlare*)p)->close_detached();},nullptr,
       [](void*p){return ((NativeNpcFlare*)p)->remove();},[](void*p){return ((NativeNpcFlare*)p)->close_detached();}}))return s.failure=2;
    const void *source=nullptr;
    if(uc_entity_base_construct_parent(&ctx,&entity.view,&source)||uc_entity_subscriptions_create(&s,&s.subscriptions)||
       uc_gunflare_sprite_construct(&registry,&s.sprite,&s.token))return s.failure=2;
    int inserted=0;
    if(uc_entity_listener_register(&w.registry(),entity.id,entity.id,uc_entity_subscriptions_listener(s.subscriptions),w.listeners(),&inserted)||!inserted)return s.failure=2;
    const UCEntityEventSubscription shot{0x52,Impl::shot},alternate{0x51,Impl::unsupported};
    if(uc_entity_subscriptions_append(s.subscriptions,&shot)||uc_entity_subscriptions_append(s.subscriptions,&alternate))return s.failure=2;
    const float identity[]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    try{
        //186BC0 prepends each completed group: shotgun, assault, police rifle.
        for(unsigned order=0;order<3;++order){unsigned i=2-order;s.models[i]=std::make_unique<Impl::Instance>();auto&m=*s.models[i];
            m.model.load(r,a,hash,catalog,names[i],nullptr,nullptr,&lookup);
            for(const auto&material:m.model.materials)if(!material.supported())return s.failure=2;
            m.model.bind_pose(r,identity);std::memcpy(m.world,entity.transform,64);
            s.handles[i]=&m;s.groups[i]={names[i],s.handles+i,1};
        }
        size_t selected=SIZE_MAX;
        if(uc_archive_lookup(&lookup,"Particles\\light\\flare.tga",&selected)||selected==SIZE_MAX||
           uc_texture_load(a,selected,UC_AR_LOCAL_PAYLOAD,&s.texture))return s.failure=2;
        s.resource.image=&s.texture;s.sprite_texture=&s.resource;s.uploaded_texture=r.upload(s.texture);
    }catch(...){s.failure=2;throw;}
    if(w.set_priority(entity.id,2))return s.failure=2;
    std::memcpy(s.flare.world,entity.transform,64);s.flare.parent=entity.parent;s.ready=true;return 0;
}
int NativeNpcFlare::update(){auto&s=*p_;if(!s.ready||s.failure||s.removed)return s.failure?s.failure:1;
    std::memcpy(s.flare.world,entity.transform,64);s.flare.parent=entity.parent;auto h=s.services();
    if(uc_gunflare_tick(&s.flare,s.groups,3,&h))return s.failure=2;return 0;}
int NativeNpcFlare::late(){auto&s=*p_;if(!s.ready||s.failure||s.removed)return s.failure?s.failure:1;
    std::memcpy(s.flare.world,entity.transform,64);auto h=s.services();
    if(uc_gunflare_transform(&s.flare,s.groups,3,s.random,&h))return s.failure=2;return 0;}
int NativeNpcFlare::remove(){auto&s=*p_;if(!s.world||s.removed||entity.model||entity.helper||entity.aux||entity.source!=-1)return 1;
    for(auto&i:s.models)i.reset(); //187AF0 releases groups before20A620.
    entity.flags|=0x8100;uint8_t queued=0;
    if(uc_entity_retire(&s.world->retirement(),entity.id,&queued)||!queued)return s.failure=2;
    if(entity.identity.listeners[0]){if(uc_entity_listener_list_remove_first(s.world->listeners(),entity.id))return s.failure=2;
        for(auto&listener:entity.identity.listeners)listener=nullptr;}
    s.removed=true;return 0;
}
int NativeNpcFlare::close_detached(){auto&s=*p_;if(!s.world)return 0;
    if(uc_entity_event_lookup(&s.world->registry(),entity.id))return 1;
    if(s.token&&uc_particle_registry_remove(s.registry,s.token))return 1;
    if(entity.identity.listeners[0]){if(uc_entity_listener_list_remove_first(s.world->listeners(),entity.id))return 1;
        for(auto&listener:entity.identity.listeners)listener=nullptr;}
    uc_entity_subscriptions_close(s.subscriptions);s.subscriptions=nullptr;
    for(auto&i:s.models)i.reset();s.uploaded_texture.Reset();uc_texture_free(&s.texture);s.sprite_texture=nullptr;
    s.world=nullptr;s.ready=false;return 0;
}
bool NativeNpcFlare::ready()const{return p_->ready&&!p_->failure&&!p_->removed;}
const UCGunFlare&NativeNpcFlare::state()const{return p_->flare;}
const UCGunFlareModels*NativeNpcFlare::groups()const{return p_->groups;}
const UCGunFlareSpriteSource&NativeNpcFlare::sprite()const{return p_->sprite;}
uintptr_t NativeNpcFlare::sprite_token()const{return p_->token;}
void NativeNpcFlare::draw(NativeRenderer&r,const float*vp,float exposure){if(!ready())return;
    DirectX::XMFLOAT4X4 projection;std::memcpy(&projection,vp,64);
    for(auto&i:p_->models)if(i&&i->visible){DirectX::XMFLOAT4X4 world,clip;std::memcpy(&world,i->world,64);
        DirectX::XMStoreFloat4x4(&clip,DirectX::XMLoadFloat4x4(&world)*DirectX::XMLoadFloat4x4(&projection));
        NativeTextureContext texture{r.material_phase};i->model.draw(r,&clip._11,i->tint,&texture,exposure);}
}
