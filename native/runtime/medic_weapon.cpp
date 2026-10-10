#include "medic_weapon.h"
#include "audio.h"
#include <DirectXMath.h>
#include <cstring>
#include <cmath>

struct NativeMedicWeapon::Impl {
 NativeMedicWeapon*owner;NativeMissionWorld*world=nullptr;NativeRenderer*renderer=nullptr;
 NativeMedicWeaponParent parent{};const UCArchive*archive=nullptr;const uint8_t*hash=nullptr;
 std::filesystem::path catalog;UCArchiveLookup*lookup=nullptr;UCMedicWeapon state{};
 NativeAudioBank bank;std::unique_ptr<NativeAnimatedModel> model;UCEntitySubscriptions*subscriptions=nullptr;
 uint32_t default_material[8]{},weapon_type=34;float previous[16]{},creation[16]{};bool loaded=false,removed=false,pillbox=false;int failure=0;
 void*player_owner=nullptr;int(*player_get)(void*,int32_t*)=nullptr;
 explicit Impl(NativeMedicWeapon*p):owner(p){}~Impl(){uc_entity_subscriptions_close(subscriptions);}
 NativeMissionEntity&entity(){return owner->entity;}
 UCMedicWeaponServices services(){UCMedicWeaponServices h{};h.owner=this;
  h.health_base=[](void*p){auto&s=*(Impl*)p;auto c=s.world->context();const void*definition="MeleeWeapon";c.definition=&definition;
   *c.registration->creation={0,1,s.parent.entity->id};return uc_entity_health_construct_matrix(&c,&s.entity().view,s.creation);};
  h.listener_reset=[](void*p){auto&s=*(Impl*)p;return uc_entity_subscriptions_create(&s,&s.subscriptions);};
  //1FB330 creates one neutral1FB410 material row. This type34 path
  //never adds the type9 animated row or mutates its retained default.
  h.material_reset=[](void*p){const uint32_t neutral[8]={0,0,0,UINT32_MAX,UINT32_MAX,0,UINT32_MAX,0};
   std::memcpy(((Impl*)p)->default_material,neutral,sizeof neutral);return 0;};
  h.property=[](void*p,unsigned t,const char*n,UCMedicWeaponProperty*out,unsigned*found){auto&s=*(Impl*)p;*found=1;
   if(t==1&&!strcmp(n,"WeaponType"))out->integer=(int32_t)s.weapon_type;else if(t==1&&!strcmp(n,"Bone"))out->integer=s.pillbox?1:0;
   else if(t==3&&!strcmp(n,"Collision"))out->integer=0;else if(t==3&&!strcmp(n,"HasModel"))out->integer=1;
   else if(t==6&&!strcmp(n,"SoundBank"))out->string=s.weapon_type==8?"Banks\\Objects\\Cleaver.amp":"Banks\\Default.amp";
   else if(t==1&&!strcmp(n,"Texture variation")){if(s.weapon_type==8)out->integer=1;else *found=0;}else return 1;return 0;};
  h.bank=[](void*p,const char*n,void**out){auto&s=*(Impl*)p;auto at=find_resource(s.archive,n);BytesOwner bytes;
   if(at==SIZE_MAX||uc_archive_read(s.archive,at,UC_AR_LOCAL_PAYLOAD,&bytes.value)!=UC_AR_OK)return 1;
   s.bank.load(bytes.value.data,bytes.value.size);*out=&s.bank;return 0;};
  h.resolve=[](void*p,int32_t id,const UCEntityFreshView**out){auto&s=*(Impl*)p;auto*t=uc_entity_event_lookup(&s.world->registry(),id);
   auto&c=s.world->context();*out=t?c.resolve(c.owner,t):nullptr;return t&&!*out?1:0;};
  h.asset=[](void*p,const char**out){auto&s=*(Impl*)p;*out=s.pillbox?"Assets_Characters/CHA_Paramedic_Female/paramedic_pillbox_skin.amf":s.weapon_type==8?"Assets_Characters\\CHA_BurnerBrute\\CHA_BurnerBrute_Cleaver_Skin.AMF":"Assets_Characters\\CHA_Paramedic\\CHA_MedKit_skin.amf";return 0;};
  h.model_load=[](void*p,const char*n,int32_t*blade){auto&s=*(Impl*)p;s.model=std::make_unique<NativeAnimatedModel>();
   s.model->load(*s.renderer,s.archive,s.hash,s.catalog,n,nullptr,nullptr,s.lookup);
   for(const auto&m:s.model->materials)if(!m.supported())return 1;
   const float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};s.model->bind_pose(*s.renderer,identity);s.entity().model=s.model.get();*blade=-1;
   for(uint32_t i=0;i<s.model->source.bone_count;i++)if(!_strnicmp((const char*)s.model->source.bones[i].source_metadata+4,"chainsaw_blade",16)){*blade=(int32_t)i;break;}return 0;};
  h.tag=[](void*p,const UCEntityFreshView*v,const char*n,int32_t*out){auto&s=*(Impl*)p;if(v!=&s.parent.entity->view)return 1;*out=s.parent.bone(s.parent.owner,n);return 0;};
  h.listener_register=[](void*p,int32_t id){auto&s=*(Impl*)p;int inserted=0;return int(uc_entity_listener_register(&s.world->registry(),id,id,
   uc_entity_subscriptions_listener(s.subscriptions),s.world->listeners(),&inserted)||!inserted);};
  h.listener_bind=[](void*p,unsigned i){auto&s=*(Impl*)p;UCEntityEventSubscription row{};
   if(i==0){row.event=215;row.receive=[](void*q,const void*,uint32_t*){return ((Impl*)q)->failure=2;};}
   else if(i==1){row.event=218;row.receive=[](void*q,const void*,uint32_t*){return ((Impl*)q)->owner->set_flags(1);};}
   else if(i==2){row.event=216;row.receive=[](void*q,const void*v,uint32_t*){if(!v)return 1;uc_medic_weapon_pause(&((Impl*)q)->state,*(const uint8_t*)v);return 0;};}
   else return 1;return uc_entity_subscriptions_append(s.subscriptions,&row);};
  h.priority=[](void*p,int32_t id,unsigned priority){return ((Impl*)p)->world->set_priority(id,(int32_t)priority);};
  h.skin=[](void*p,int32_t v){((Impl*)p)->model->select_skin(v);return 0;};
  //The same immutable model/material set remains resident. Source rig dirty
  //flag only changes on an actual model replacement, which reload resets.
  h.material_dirty=[](void*,unsigned*out){*out=0;return 0;};
  h.health_update=[](void*p){auto&e=((Impl*)p)->entity();if(e.flags&0x800||e.helper||!e.aux)return 1;return uc_entity_fresh_aux_clear(e.aux);};
  h.pose=[](void*p,const UCEntityFreshView*v,int32_t index,float*out){auto&s=*(Impl*)p;
   return v==&s.parent.entity->view&&s.parent.pose(s.parent.owner,index,out)?0:1;};
  h.set_world=[](void*p,const float*m){std::memcpy(((Impl*)p)->entity().transform,m,64);return 0;};
  h.time=[](void*p,float*out){*out=((Impl*)p)->world->time();return 0;};
  h.player_count=[](void*,int32_t*out){*out=1;return 0;};
  //The missing-parent path removes attached type34 in this same call regardless
  //of player lookup. Require the real player provider if that query is reached.
  h.player=[](void*p,int32_t*out){auto&s=*(Impl*)p;return s.player_get?s.player_get(s.player_owner,out):1;};
  h.delta=[](void*p,float*out){*out=*(((Impl*)p)->world->script_services().delta);return 0;};
  h.remove=[](void*p){return ((Impl*)p)->owner->remove();};return h;
 }
};
NativeMedicWeapon::NativeMedicWeapon():p_(std::make_unique<Impl>(this)){}
NativeMedicWeapon::~NativeMedicWeapon()=default;
int NativeMedicWeapon::construct(NativeRenderer&r,NativeMissionWorld&w,const NativeMedicWeaponParent&parent,const UCArchive*a,
 const uint8_t hash[32],const std::filesystem::path&catalog,UCArchiveLookup&lookup,uint32_t type,const float*pillbox_world){
 auto&s=*p_;if((pillbox_world&&type!=34)||(type!=34&&type!=8)||s.world||!a||!hash||lookup.archive!=a||!parent.entity||!parent.bone||!parent.pose||
  uc_entity_event_lookup(&w.registry(),parent.entity->id)!=&parent.entity->identity)return 1;
 s.pillbox=pillbox_world!=nullptr;float dummy[16];int32_t bone=parent.bone(parent.owner,"gundummy");
 if(pillbox_world){for(unsigned i=0;i<16;i++)if(!std::isfinite(pillbox_world[i]))return 1;std::memcpy(s.creation,pillbox_world,64);}
 else
 if(!parent.pose(parent.owner,bone,dummy)||uc_medic_weapon_matrix(dummy,parent.entity->transform,s.creation))
  throw std::runtime_error("Melee weapon parent gundummy pose type="+std::to_string(type)+" bone="+std::to_string(bone));
 s.world=&w;s.renderer=&r;s.parent=parent;s.archive=a;s.hash=hash;s.catalog=catalog;s.lookup=&lookup;s.weapon_type=type;
 if(w.bind_dynamic({&entity.view,this,[](void*p){return ((NativeMedicWeapon*)p)->update();},[](void*p){return ((NativeMedicWeapon*)p)->late();},
  [](void*p){return ((NativeMedicWeapon*)p)->close_detached();},nullptr,[](void*p){return ((NativeMedicWeapon*)p)->remove();},[](void*p){return ((NativeMedicWeapon*)p)->close_detached();}}))return s.failure=2;
 auto h=s.services();if(uc_medic_weapon_construct(&s.state,&entity.view,&h))
  throw std::runtime_error("Melee weapon construction type="+std::to_string(type)+" model="+std::to_string(entity.model!=nullptr)+" blade="+std::to_string(s.state.blade)+" bank="+std::to_string(s.state.bank!=nullptr));
 std::memcpy(s.previous,entity.transform,64);s.loaded=true;return 0;
}
int NativeMedicWeapon::update(){auto&s=*p_;if(!ready())return s.failure?s.failure:1;auto h=s.services();return uc_medic_weapon_update(&s.state,&entity.view,&h)?s.failure=2:0;}
int NativeMedicWeapon::late(){auto&s=*p_;if(!ready())return s.failure?s.failure:1;std::memcpy(s.previous,entity.transform,64);auto h=s.services();return uc_medic_weapon_late(&s.state,&entity.view,&h)?s.failure=2:0;}
int NativeMedicWeapon::set_flags(uint32_t flags){auto&s=*p_;if(!s.world)return 1;entity.flags|=flags;if((flags&1)&&(entity.flags&8)){
 auto&r=*s.world->context().registration;*r.creation={0,1,entity.parent};if(r.insert_record(r.owner,&entity.view.target,entity.id,1))return s.failure=2;entity.flags&=~8u;}return 0;}
int NativeMedicWeapon::remove(){auto&s=*p_;if(!s.world||s.removed||entity.helper||!entity.aux)return 1;
 if(uc_entity_fresh_aux_clear(entity.aux))return s.failure=2;entity.flags|=0x8100;uint8_t queued=0;
 if(uc_entity_retire(&s.world->retirement(),entity.id,&queued)||!queued)return s.failure=2;
 if(entity.identity.listeners[0]){if(uc_entity_listener_list_remove_first(s.world->listeners(),entity.id))return s.failure=2;for(auto&v:entity.identity.listeners)v=nullptr;}
 entity.flags|=0x80000;s.removed=true;return 0;
}
int NativeMedicWeapon::close_detached(){auto&s=*p_;if(!s.world)return 0;if(uc_entity_event_lookup(&s.world->registry(),entity.id))return 1;
 if(entity.identity.listeners[0]){if(uc_entity_listener_list_remove_first(s.world->listeners(),entity.id))return 1;for(auto&v:entity.identity.listeners)v=nullptr;}
 uc_entity_subscriptions_close(s.subscriptions);s.subscriptions=nullptr;s.model.reset();entity.model=nullptr;
 if(entity.aux&&uc_entity_fresh_aux_destroy(entity.aux))return 1;entity.aux=nullptr;s.loaded=false;s.world=nullptr;return 0;
}
bool NativeMedicWeapon::ready()const{return p_->loaded&&!p_->removed&&!p_->failure;}
const UCMedicWeapon&NativeMedicWeapon::state()const{return p_->state;}
void NativeMedicWeapon::player_provider(void*p,int(*get)(void*,int32_t*)){p_->player_owner=p;p_->player_get=get;}
int NativeMedicWeapon::property(const char*n,int32_t&out)const{return uc_medic_weapon_property(&p_->state,n,&out);}
void NativeMedicWeapon::draw(NativeRenderer&r,const float*vp,float exposure,float fraction,NativeModelLighting *lighting,const NativeLightingFrame *frame){if(!ready())return;
 float m[16];native_interpolate_transform(p_->previous,entity.transform,fraction,m);DirectX::XMFLOAT4X4 a,b,o;std::memcpy(&a,m,64);std::memcpy(&b,vp,64);
 DirectX::XMStoreFloat4x4(&o,DirectX::XMLoadFloat4x4(&a)*DirectX::XMLoadFloat4x4(&b));const float white[4]={1,1,1,1};NativeTextureContext texture{r.material_phase};if(lighting&&frame)p_->model->draw_lit(r,*lighting,*frame,m,&o._11,white,&texture,exposure);else p_->model->draw(r,&o._11,white,&texture,exposure);
}
