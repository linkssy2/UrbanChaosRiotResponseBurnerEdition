#include "simple_cutscene.h"
#include <DirectXMath.h>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <string>
namespace {
constexpr float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
void require(bool b,const char*s){if(!b)throw std::runtime_error(s);}
void resource(const UCArchive*a,const char*n,BytesOwner&b){auto i=find_resource(a,n);require(i!=SIZE_MAX&&uc_archive_read(a,i,UC_AR_LOCAL_PAYLOAD,&b.value)==UC_AR_OK,"Missing cutscene resource");}
int32_t bone(const UCAnimationTemplate&s,const char*n){for(uint32_t i=0;i<s.bone_count;i++)if(!_stricmp(s.bone_names[i],n))return (int32_t)i;return -1;}
struct Reader {
 const UCLevelEntity*d;const uint8_t*cursor;
 template<class T> bool get(uint32_t type,T&out){int found=0;require(!uc_entity_stream_get(d,&cursor,type,&out,&found),"Cutscene property extent");return found!=0;}
 uint8_t boolean(uint8_t fallback){uint32_t value=0;return get(3,value)?uint8_t((value&255)!=0):fallback;}
 void flags(std::vector<int32_t>&out){int32_t count=0;get(1,count);require(count>=0&&count<=4096,"Cutscene flag count");out.resize((size_t)count);for(auto&v:out){v=0;get(1,v);}}
};
}
// Camera callbacks retain the session services, never a destroyed parent.
// The parent observer is used only while resolving that parent's live model.
struct NativeSimpleCutscene::Camera : NativeCutsceneCamera {
 NativeSimpleCutscene *parent;
 NativeCutsceneCameraServices external;
 Camera(NativeSimpleCutscene *p,const NativeCutsceneCameraServices&s):parent(p),external(s){}
 ~Camera(){if(parent)parent->camera_=nullptr;}
};
NativeCutsceneCamera *NativeSimpleCutscene::camera()const{return camera_;}
NativeSimpleCutscene::~NativeSimpleCutscene(){if(camera_)camera_->parent=nullptr;uc_animation_annotations_free(&annotations_);uc_animation_template_free(&skeleton_);}
int NativeSimpleCutscene::construct(NativeRenderer&r,NativeMissionWorld&w,uint32_t i,const UCArchive*a,const uint8_t hash[32],const std::filesystem::path&catalog,
 const UCAnimationBlendMath&math,NativePlayerAudio&audio,UCSceneCameraStack&stack,const uint32_t*be,const uint32_t*fe,UCFade&fade,const UCFadeServices&fs,const NativeSimpleCutsceneServices&s){
 if(world_||i>=w.source_count()||!a||!hash||!be||!fe||stack.entities!=&w.context()||fs.registry!=&w.registry()||
    !s.camera.preflight||!s.camera.current_provider||!s.camera.player||!s.camera.control||!s.camera.mode||!s.camera.return_control||
    !s.provider_ready||!s.skip_input||!s.restart||!s.history||s.camera.preflight(s.camera.owner))return 1;
 const auto&m=w.metadata(i);const auto*d=m.definition;
 if(!d||std::strcmp(d->class_name,"SimpleCutscene")||!m.asset_present||*w.active_descriptor()!=&m)return 1;
 const auto*binding=w.binding(i);
 if(!binding||binding->view!=&entity.view||!binding->remove||!binding->destroy)return 1;
 try {
  Reader p{d,*w.property_cursor(i)};const char*bank=nullptr;clip_=nullptr;p.get(8,clip_);p.get(6,bank);fov_=36;p.get(2,fov_);p.flags(finish_);
  fields.fade=p.boolean(1);fields.fade_up=p.boolean(1);const auto flash=p.boolean(0);p.flags(wait_);fields.can_skip=p.boolean(0);
  fields.restart=-1;p.get(1,fields.restart);fields.goto_restart=p.boolean(0);p.flags(prefade_);int32_t pair[2]={0,-1};p.get(5,pair);end_type_=(uint32_t)pair[0];fields.end_group=pair[1];
  require(clip_&&bank&&!fields.goto_restart&&std::isfinite(fov_)&&fov_>0&&fov_<180,"Cutscene resource/FOV/preload domain");
  require(p.cursor==d->properties.data+d->properties.size,"Cutscene trailing properties");
  size_t consumed=0;model_.load(r,a,hash,catalog,d->asset_name,clip_,nullptr,nullptr,UINT32_MAX,&consumed);
  for(const auto&v:model_.materials)require(v.supported(),"Cutscene complete material admission");
  BytesOwner bytes;resource(a,model_.source.animation_template,bytes);require(!uc_animation_template_load(bytes.value.data,bytes.value.size,&skeleton_),"Cutscene skeleton");
  require(!uc_animation_annotations_load(&model_.sampler.clip,&annotations_)&&!annotations_.event_count&&!annotations_.tag_count,"Cutscene event domain");
  require(bone(skeleton_,"CameraDummy")>=0&&bone(skeleton_,"SoundEmitter")==-1,"Cutscene camera/sound bone domain");
  BytesOwner bank_bytes;resource(a,bank,bank_bytes);bank_.assign(bank_bytes.value.data,bank_bytes.value.data+bank_bytes.value.size);
  for(const auto&v:{&finish_,&wait_,&prefade_})for(int32_t flag:*v)require(flag>=0&&(size_t)flag<w.script_services().flag_count,"Cutscene flag index");
  renderer_=&r;world_=&w;audio_=&audio;services_=s;fade_=&fade;fade_services_=&fs;stack_=&stack;bone_event_=be;fov_event_=fe;math_=math;
  if(uc_entity_fresh_construct_authored(&w.context(),&entity.view,w.active_descriptor()))return failure_=2;
  *w.property_cursor(i)=p.cursor;
  fields.finish_flags=finish_.empty()?nullptr:finish_.data();fields.finish_count=(int32_t)finish_.size();
  fields.wait_flags=wait_.empty()?nullptr:wait_.data();fields.wait_count=(int32_t)wait_.size();fields.prefade_flags=prefade_.empty()?nullptr:prefade_.data();fields.prefade_count=(int32_t)prefade_.size();
  fields.fade=fields.fade||flash;fields.fade_requested=!fields.fade;fields.waiting=1;fields.camera_id=-1;fields.sound_bone=-1;
  entity.flags|=0x2100;require(!uc_entity_names_bind(w.names(),"SUBTITLE",entity.id),"Cutscene subtitle name");
  require(!entity.script.attach_actor(entity.view,m,w.script_services()),"Cutscene actor scheduler");
  entity.model=&model_;model_.bind_pose(r,identity);ready_=true;
  if(start_camera())return failure_=2;return 0;
 }catch(...){return failure_=world_?2:1;}
}
int NativeSimpleCutscene::start_camera(){
 UCEntityProperties p{};UCEntityProperty scratch{};int32_t b=bone(skeleton_,"CameraDummy");uint32_t automatic=1;
 if(uc_entity_properties_begin(&p,-1)||uc_entity_properties_append(&p,&scratch,2,"fovCurrent",&fov_)||
    uc_entity_properties_append(&p,&scratch,1,"Bone",&b)||uc_entity_properties_append(&p,&scratch,3,"auto take focus",&automatic))return 1;
 camera_=new Camera(this,services_.camera);auto h=services_.camera;h.owner=camera_;
 h.preflight=[](void*q){auto&s=((Camera*)q)->external;return s.preflight(s.owner);};
 h.current_provider=[](void*q)->void*{auto&s=((Camera*)q)->external;return s.current_provider(s.owner);};
 h.player=[](void*q,void*p,int32_t i,int32_t*out){auto&s=((Camera*)q)->external;return s.player(s.owner,p,i,out);};
 h.control=[](void*q,int32_t id){auto&s=((Camera*)q)->external;return s.control(s.owner,id);};
 h.mode=[](void*q,void*p,int32_t m,int32_t a){auto&s=((Camera*)q)->external;return s.mode(s.owner,p,m,a);};
 h.bone_matrix=[](void*q,UCEntityEventTarget*t,int32_t b,float*out,uint8_t*found){auto*c=((Camera*)q)->parent;
  if(!c||t!=&c->entity.identity)return 1;*found=b>=0&&(uint32_t)b<c->model_.source.bone_count&&c->model_.prepared.world_valid[b];
  if(*found)std::memcpy(out,c->model_.prepared.world[b],64);return 0;};
 h.remove=nullptr;
 h.return_control=[](void*q,int32_t id,uint32_t sender){auto&s=((Camera*)q)->external;return s.return_control(s.owner,id,sender);};
 h.release_camera=[](void*q,NativeCutsceneCamera*p){auto*c=(Camera*)q;if(c!=p)std::terminate();delete c;};
 h.release_on_close=true;
 int result=camera_->create(*world_,&entity.id,p,*stack_,bone_event_,fov_event_,h);
 if(result==1){delete camera_;return result;} // No binding was committed.

 if(!result)fields.camera_id=camera_->entity.id;return result;
}
int NativeSimpleCutscene::advance(float delta){
 if(!animation_started_)return 0;
 UCAnimationControllerEntry entry{&animation_,&model_.sampler.clip,&annotations_};UCAnimationControllerListUpdate update{};
 if(uc_animation_controller_list_advance(&entry,1,&skeleton_,delta,&update)||update.events.count||update.removed_count)return 1;
 UCAnimationBindingSample sample{&model_.sampler,animation_.phase,animation_.weight,1};
 try{model_.sample(*renderer_,math_,&sample,(animation_.flags&0x80000000u)?1:0,identity);}catch(...){return 1;}return 0;
}
int NativeSimpleCutscene::finish_flags(){
 auto&h=world_->script_services();for(int32_t i=0;i<fields.finish_count;i++)h.flags[fields.finish_flags[i]]=1;
 fields.finish_count=0;return uc_environment_history_reset(services_.history)?0:1;
}
int NativeSimpleCutscene::handoff(){
 if(fields.handed_off)return 0;fields.handed_off=1;entity.flags|=0x100;
 auto&s=services_.camera;void*provider=s.current_provider(s.owner);int32_t player=-1;
 if(!provider||s.player(s.owner,provider,0,&player))return 1;
 uint32_t payload=0,sender=(uint32_t)entity.id;uint8_t handled=0;
 if(uc_entity_event_send(&world_->registry(),player,0x22,&payload,4,&sender,&handled))return 1;
 return finish_flags();
}
UCSimpleCutsceneServices NativeSimpleCutscene::hooks(){
 UCSimpleCutsceneServices h{};
 h.preflight=[](void*p)->int{auto&c=*(NativeSimpleCutscene*)p;return !c.ready_||c.failure_||c.services_.camera.preflight(c.services_.camera.owner);};
 h.triggers_update=[](void*p){auto&c=*(NativeSimpleCutscene*)p;if(c.trigger_bound_)c.fields.switch_camera=0;return 0;}; //actual mode1, empty animation notifications
 h.model_enable=[](void*p){((NativeSimpleCutscene*)p)->model_flags_|=0x40;return 0;}; //20A290->21AE00->B5F60->B5D60
 h.flags_ready=[](void*p,int32_t n,const int32_t*f,uint8_t*out){auto&c=*(NativeSimpleCutscene*)p;*out=1;for(int32_t i=0;i<n;i++)if(!c.world_->script_services().flags[f[i]]){*out=0;break;}return 0;};
 h.provider=[](void*p)->void*{auto&c=*(NativeSimpleCutscene*)p;return c.services_.camera.current_provider(c.services_.camera.owner);};
 h.provider_ready=[](void*p,void*v,uint8_t*out){auto&c=*(NativeSimpleCutscene*)p;return c.services_.provider_ready(c.services_.camera.owner,v,out);};
 h.resolve_clip=[](void*p,void**out){*out=&((NativeSimpleCutscene*)p)->model_.sampler.clip;return 0;};
 h.start_animation=[](void*p,void*clip){auto&c=*(NativeSimpleCutscene*)p;if(clip!=&c.model_.sampler.clip||c.animation_started_)return 1;
  if(uc_animation_controller_init(&c.model_.sampler.clip,0xC0000009u,&c.animation_))return 1;c.animation_.repeats=0;c.animation_.blend_out=-1;c.animation_.blend_in=0;c.animation_started_=true;return 0;};
 h.start_audio=[](void*p){auto&c=*(NativeSimpleCutscene*)p;try{if(c.sound_)return 1;c.sound_=c.audio_->create_empty_source(c.bank_.data(),c.bank_.size(),c.entity.transform+12);c.entity.helper=c.sound_;c.fields.sound_bone=-1;return c.audio_->play_empty_source(c.sound_,"anim-00")?1:0;}catch(...){return 1;}};
 h.bind_trigger=[](void*p,uint8_t*flag){auto&c=*(NativeSimpleCutscene*)p;if(flag!=&c.fields.switch_camera||c.trigger_bound_)return 1;*flag=0;c.trigger_bound_=true;return 0;};
 h.audio_busy=[](void*p,uint8_t*out){auto&c=*(NativeSimpleCutscene*)p;try{*out=c.sound_&&c.audio_->empty_source_busy(c.sound_);return 0;}catch(...){return 1;}};
 h.remove=[](void*p){return ((NativeSimpleCutscene*)p)->remove();};
 h.switch_bone=[](void*p,uint32_t index){auto&c=*(NativeSimpleCutscene*)p;int32_t b=bone(c.skeleton_,("CameraDummy"+std::to_string(index)).c_str());if(b<0)return 0;uint8_t handled;return c.camera_?c.camera_->send(*c.bone_event_,&b,4,&handled):0;};
 h.delta=[](void*p)->double{return *(((NativeSimpleCutscene*)p)->world_->script_services().delta);};
 h.skip_input=[](void*p,double*out){auto&c=*(NativeSimpleCutscene*)p;return c.services_.skip_input(c.services_.camera.owner,out);};
 h.animation_done=[](void*p,uint8_t*out){auto&c=*(NativeSimpleCutscene*)p;*out=!c.animation_started_||!(c.animation_.flags&0x40000000u);return 0;};
 h.animation_duration=[](void*p,double*out){auto&c=*(NativeSimpleCutscene*)p;*out=c.animation_started_?1.0/(double)c.animation_.phase_scale:0;return 0;};
 h.animation_time=[](void*p,double*out){auto&c=*(NativeSimpleCutscene*)p;*out=c.animation_started_?(double)c.animation_.phase/(double)c.animation_.phase_scale:0;return 0;};
 h.fade_request=[](void*p,uint32_t mode,float time,int32_t scope,uint8_t*done,int32_t owner,uint8_t white){auto&c=*(NativeSimpleCutscene*)p;return scope!=-1?1:uc_fade_request(c.fade_,c.fade_services_,mode,time,done,owner,white);};
 h.sound_position=[](void*p){return ((NativeSimpleCutscene*)p)->fields.sound_bone==-1?0:1;};
 h.advance_animation=[](void*p,float dt){return ((NativeSimpleCutscene*)p)->advance(dt);};
 h.after_animation=[](void*p){return ((NativeSimpleCutscene*)p)->entity.script.update();};
 h.set_flag=[](void*p,int32_t f){auto&c=*(NativeSimpleCutscene*)p;auto&s=c.world_->script_services();if(f<0||(size_t)f>=s.flag_count)return 1;s.flags[f]=1;return 0;};
 h.end_event=[](void*p,int32_t group){auto&c=*(NativeSimpleCutscene*)p;uint8_t accepted;return c.entity.script.event((uint32_t)group,1,&accepted);};
 h.remove_camera=[](void*p,int32_t id){auto&c=*(NativeSimpleCutscene*)p;auto*t=uc_entity_event_lookup(&c.world_->registry(),id);if(!t)return 0;auto*v=c.world_->context().resolve(c.world_->context().owner,t);if(!v)return 1;*v->target.flags|=1;return 0;};
 h.restart=[](void*p,int32_t index){auto&c=*(NativeSimpleCutscene*)p;return c.services_.restart(c.services_.camera.owner,index);};
 h.finish_flags=[](void*p){return ((NativeSimpleCutscene*)p)->finish_flags();};h.handoff=[](void*p){return ((NativeSimpleCutscene*)p)->handoff();};return h;
}
int NativeSimpleCutscene::update(){if(!ready_||failure_)return failure_?failure_:1;auto h=hooks();return uc_simple_cutscene_update(&entity.view,&fields,&h,this)?failure_=2:0;}
int NativeSimpleCutscene::late(){if(!ready_||failure_)return failure_?failure_:1;if(!(entity.actor.word90&1)){for(unsigned i=12;i<15;i++)entity.transform[i]+=0.f;entity.transform[15]=1;}return 0;}
int NativeSimpleCutscene::remove(){
 if(!ready_||failure_||!world_||entity.actor.handleBC||!entity.aux||
    entity.model!=&model_||entity.helper!=sound_||entity.source<0)return 1;
 //The fade service borrows completion storage. Mid-fade forced removal is
 //outside this profile until the original cancellation path is connected.
 if(fade_->completion&&fade_->completion_owner==entity.id)return 1;
 const auto*b=world_->binding((uint32_t)entity.source);
 if(!b||b->view!=&entity.view||!b->destroy)return 1;
 for(auto*l:entity.identity.listeners)if(l)return 1;
 if(handoff())return failure_=2;
 uc_entity_names_remove(world_->names(),"SUBTITLE",entity.id);
 trigger_bound_=false;
 std::vector<int32_t>().swap(prefade_);std::vector<int32_t>().swap(wait_);std::vector<int32_t>().swap(finish_);
 fields.prefade_flags=fields.wait_flags=fields.finish_flags=nullptr;
 fields.prefade_count=fields.wait_count=fields.finish_count=0;
 //218B30 destroys the active script command before model/aux/helper removal.
 uc_mission_command_destroy(nullptr,entity.actor.audioC0);entity.actor.audioC0=nullptr;
 //The PC model has no registered collision body. Hide it now; GPU resources
 //stay owned until pending destruction, as does the independent camera.
 entity.flags&=~0x800u;
 if(uc_entity_fresh_aux_clear(entity.aux))return failure_=2;
 entity.flags|=0x8100;
 if(sound_){try{audio_->close_empty_source(sound_);}catch(...){return failure_=2;}sound_=nullptr;}
 entity.helper=nullptr;
 uint8_t queued=0;if(uc_entity_retire(&world_->retirement(),entity.id,&queued)||!queued)return failure_=2;
 if(entity.flags&0x1000){auto*f=world_->slot((uint32_t)entity.source).flags;*f=(*f&~15u)|16u;}
 entity.flags|=0x80000;ready_=false;return 0;
}
int NativeSimpleCutscene::close_detached(){
 if(entity.script.close_detached())return 1;if(sound_){try{audio_->close_empty_source(sound_);}catch(...){return 1;}sound_=nullptr;}entity.helper=nullptr;
 if(world_)uc_entity_names_remove(world_->names(),"SUBTITLE",entity.id);
 if(entity.aux&&uc_entity_fresh_aux_destroy(entity.aux))return 1;entity.aux=nullptr;entity.model=nullptr;ready_=false;return 0;
}
void NativeSimpleCutscene::draw(NativeRenderer&r,const float*vp,float exposure,NativeModelLighting*lighting,const NativeLightingFrame*frame){if(!ready_||failure_||(entity.flags&0x100))return;
 using namespace DirectX;XMFLOAT4X4 matrix;XMStoreFloat4x4(&matrix,XMLoadFloat4x4((const XMFLOAT4X4*)entity.transform)*XMLoadFloat4x4((const XMFLOAT4X4*)vp));
 const float tint[4]={1,1,1,1};NativeTextureContext texture{fields.elapsed};
 if(lighting&&frame)model_.draw_lit(r,*lighting,*frame,entity.transform,&matrix._11,tint,&texture,exposure);
 else model_.draw(r,&matrix._11,tint,&texture,exposure);
}
