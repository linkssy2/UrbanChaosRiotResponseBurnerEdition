#include "level_particles.h"
#include "firstperson_animation.h"
#include "mission_world.h"
#include "player_audio.h"
extern "C" {
#include "../particle_asset.h"
#include "../particle_registry.h"
#include "../particle_render_state.h"
#include "../particle_stretch.h"
#include "../particle_rotating_quad.h"
#include "../fire_visual.h"
#include "../texture_resources.h"
#include "../animation_particle.h"
#include "../animating_prop.h"
#include "../animating_prop_state.h"
#include "../particle_attachment.h"
}
#include "particle_billboards.h"
#include "particle_quads.h"
#include "heat_haze.h"
#include "character_navigation.h"
extern "C" {
#include "../rigid_prop.h"
#include "../character_collision.h"
}
#include <DirectXMath.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <stdexcept>
using namespace DirectX;
namespace {
void need(bool value,const char*message){if(!value)throw std::runtime_error(message);}
std::string shader(const std::filesystem::path&p){auto b=read_file(p,1024*1024);return {b.begin(),b.end()};}
XMMATRIX matrix(const float*p){XMFLOAT4X4 v;memcpy(&v,p,64);return XMLoadFloat4x4(&v);}
void store(float*p,FXMMATRIX m){XMFLOAT4X4 v;XMStoreFloat4x4(&v,m);memcpy(p,&v,64);}
constexpr unsigned maximum=4096;
}
struct NativeLevelParticles::Impl {
    struct Entry {uint32_t descriptor;const UCLevelEntity*entity;UCParticleAsset asset{};ComPtr<ID3D11ShaderResourceView> image;bool active=false,fire=false,started=false;float source[16]{},target[16]{};unsigned link=0;bool prop=false;const UCTextureResource *texture=nullptr;const char *resource=nullptr;};
    std::vector<Entry> entries;
    struct Actor {NativeMissionEntity entity;NativeLevelParticles *owner;uint32_t descriptor;const float*target;
        static int update(void*p){return ((Actor*)p)->entity.script.update();}
        static int late(void*p){auto&a=*(Actor*)p;return memcmp(a.entity.transform,a.target,64)?1:0;}
        static int close(void*p){auto&a=*(Actor*)p;if(a.entity.script.close_detached())return 1;
            a.owner->deactivate(a.descriptor);
            if(a.entity.aux&&uc_entity_fresh_aux_destroy(a.entity.aux))return 1;a.entity.aux=nullptr;return 0;}
    };
    std::vector<std::unique_ptr<Actor>> actors;
    struct Prop;
    struct Source {Prop *prop;UCParticleAttachment attachment{};float fixed[16]{};bool attached=false;const UCParticleLifecycleView *view=nullptr;};
    std::vector<std::unique_ptr<Source>> sources;
    std::vector<std::unique_ptr<Prop>> props;
    Source *registering=nullptr;
    NativeRenderer *renderer=nullptr;NativeMissionWorld *world=nullptr;
    std::filesystem::path game,shaders;bool quads_ready=false;
    UCAnimationBlendMath math{};
    NativeLevelParticleCounts counts;
    UCTextureResources*textures=nullptr,*prop_textures=nullptr,*helicopter_textures=nullptr;
    UCParticleRegistry*registry=nullptr;UCParticleLifecycle*lifetime=nullptr;
    NativeParticleBillboards gpu;
    NativeParticleQuads quads;
    std::unique_ptr<NativeHeatHaze> heat;
    ComPtr<ID3D11BlendState> blends[7];ComPtr<ID3D11SamplerState> sampler;
    ComPtr<ID3D11DepthStencilState> depth;
    float table[16]{},axis_sine[16]{},axis_cosine[16]{};
    std::array<UCParticlePlainSprite,maximum> plain;
    std::array<UCParticleSprite,maximum> sprites;
    std::array<UCParticleQuad,maximum> quad_scratch;
    ~Impl(){if(lifetime)uc_particle_lifecycle_close(lifetime);if(registry)uc_particle_registry_close(registry);uc_texture_resources_close(textures);uc_texture_resources_close(prop_textures);uc_texture_resources_close(helicopter_textures);}
    static uintptr_t add(void*p,const UCParticleLifecycleView*v){auto&s=*(Impl*)p;uintptr_t id=0;need(!uc_particle_registry_add(s.registry,v,&id)&&id,"Level particle registration");if(s.registering)s.registering->view=v;return id;}
    static void remove(void*p,uintptr_t id,const UCParticleLifecycleView*v){auto&s=*(Impl*)p;need(uc_particle_registry_view(s.registry,id)==v&&!uc_particle_registry_remove(s.registry,id),"Level particle removal");auto it=std::find_if(s.sources.begin(),s.sources.end(),[&](const auto&q){return q->view==v;});if(it!=s.sources.end())s.sources.erase(it);}
    struct Prop {
        Impl *owner;Entry *entry;std::vector<Entry*> emitters;NativeMissionEntity entity;
        NativeAnimatedModel model;NativeFirstpersonClips bank;NativeFirstpersonAnimations animations;
        UCParticleAttachmentServices attachment{};uint8_t finished=0,loop=0;bool ready=false,vehicle=false,deferred=false,pending=false;
        UCAnimatingPropState selection{};std::array<std::vector<int32_t>,9> conditions;
        bool helicopter=false,courier=false,empty_collision=false;
        UCModelCollisionShape collision{};UCSolidBlockGeometry box{};uint16_t collision_bone=0;bool box_ready=false;uint8_t disabled=0;
        std::unique_ptr<NativePlayerAudio::MissionSource> sound;
        Prop(Impl*s,Entry*e):owner(s),entry(e){}
        static int32_t find(void*u,UCEntityEventTarget*t,const char*n){auto&p=*(Prop*)u;need(t==&p.entity.identity,"Prop bone owner");for(uint32_t i=0;i<p.model.source.bone_count;i++)if(!_strnicmp((const char*)p.model.source.bones[i].source_metadata+4,n,16))return (int32_t)i;return -1;}
        static uint8_t bone(void*u,UCEntityEventTarget*t,int32_t index,float*out){auto&p=*(Prop*)u;need(t==&p.entity.identity,"Prop pose owner");if(index<0||(uint32_t)index>=p.model.source.bone_count||!p.model.prepared.world_valid[index])return 0;memcpy(out,p.model.prepared.world[index],64);return 1;}
        static const float *world(void*u,UCEntityEventTarget*t){auto&p=*(Prop*)u;need(t==&p.entity.identity,"Prop world owner");return p.entity.transform;}
        static const uint32_t *flags(void*u,UCEntityEventTarget*t){auto&p=*(Prop*)u;need(t==&p.entity.identity,"Prop flags owner");return &p.entity.flags;}
        static int start(void*u){auto&p=*(Prop*)u;auto next=std::make_unique<NativeFirstpersonController>();const auto&clip=p.bank.find((p.helicopter||p.courier)?uc_animating_prop_action(p.selection.current):"Idle");next->resource=&clip;
            need(!uc_animation_sample_load(clip.bytes.data(),clip.bytes.size(),&next->sample)&&!uc_animating_prop_controller(&next->sample.clip,&next->timing),"Prop Idle controller");
            std::array<UCAnimationControllerEntry,32> entries{};need(p.animations.list.size()<32,"Prop controller capacity");for(size_t i=0;i<p.animations.list.size();i++){auto&c=*p.animations.list[i];entries[i]={&c.timing,&c.sample.clip,&c.resource->annotations};}uint32_t at=0;
            need(!uc_animation_controller_list_insert_index(entries.data(),(uint32_t)p.animations.list.size(),&next->timing,&at),"Prop controller order");p.animations.list.reserve(p.animations.list.size()+1);if(p.animations.current)p.animations.current->timing.flags&=~9u;p.animations.current=next.get();p.animations.list.insert(p.animations.list.begin()+at,std::move(next));return 0;}
        static int dead(void*u,uint8_t*out){auto&s=*(Source*)u;return uc_particle_attachment_dead(&s.prop->attachment,&s.attachment,out);}
        static int sample(void*u,const UCParticleBounds*,float*out){auto&s=*(Source*)u;if(!s.attached){memcpy(out,s.fixed,64);return 0;}int r=uc_particle_attachment_matrix(&s.prop->attachment,&s.attachment,out);if(!r)++s.prop->owner->counts.prop_source_updates;return r;}
        static const float *marker_world(void*u,const void*t){auto&p=*(Prop*)u;need(t==&p.entity.identity,"Prop marker owner");return p.entity.transform;}
        static uint8_t marker_named(void*u,const void*t,const char*n,float*out){auto&p=*(Prop*)u;need(t==&p.entity.identity,"Prop marker pose");int32_t index=find(u,&p.entity.identity,n);float raw[16];if(!bone(u,&p.entity.identity,index,raw))return 0;
            memcpy(out,raw,16);memcpy(out+4,raw+8,16);memcpy(out+8,raw+4,16);memcpy(out+12,raw+12,16);return 1;}
        static void *marker_attached(void*u,int32_t id,const char*n,uint8_t hidden){auto&p=*(Prop*)u;auto s=std::make_unique<Source>();s->prop=&p;s->attached=true;need(*n&& !uc_particle_attachment_bind(&p.attachment,id,n,hidden,&s->attachment),"Prop attached particle source");return s.release();}
        static void *marker_fixed(void*u,int32_t id,const float*m){auto&p=*(Prop*)u;auto s=std::make_unique<Source>();s->prop=&p;s->attachment={id,-1,0};memcpy(s->fixed,m,64);return s.release();}
        static void marker_submit(void*u,void*raw,const char*file){auto&p=*(Prop*)u;auto&o=*p.owner;std::unique_ptr<Source>s((Source*)raw);auto at=std::find_if(p.emitters.begin(),p.emitters.end(),[&](const Entry*e){return !strcmp(file,e->resource);});need(at!=p.emitters.end(),"Prop marker resource changed");
            UCParticleSource source{s.get(),dead,sample};o.registering=s.get();int result=uc_particle_lifecycle_append_source(o.lifetime,&(*at)->asset,&source);o.registering=nullptr;need(!result,"Prop particle lifetime append");o.sources.push_back(std::move(s));++o.counts.prop_markers;}
        static int update(void*u){auto&p=*(Prop*)u;need(p.ready,"Prop update before construction");float dt=*p.owner->world->script_services().delta;need(std::isfinite(dt)&&dt>=0&&dt<=1,"Prop update delta");
            if(p.helicopter||p.courier){const auto&s=p.owner->world->script_services();
                UCAnimatingPropStateServices h{&p,
                    [](void*u)->unsigned{auto&p=*(Prop*)u;return !p.animations.current||!(p.animations.current->timing.flags&0x40000000u);},
                    [](void*u,int32_t i)->unsigned{auto&p=*(Prop*)u;uint32_t n;return !uc_animation_template_find_clip(&p.bank.catalog,uc_animating_prop_action(i),&n);},
                    [](void*u,void*m)->int32_t{auto&p=*(Prop*)u;need(m==&p.model,"Helicopter model owner");uint32_t groups;memcpy(&groups,p.model.source.source_header+8,4);return (int32_t)groups;},
                    [](void*u,int32_t){return start(u);},
                    [](void*u,int32_t i,uint8_t grouped){auto&p=*(Prop*)u;uint8_t accepted;int r=p.entity.script.event((uint32_t)i,grouped,&accepted);
                        if(r)throw std::runtime_error("Helicopter script event source="+std::to_string(p.entity.source)+" event="+std::to_string(i));return 0;},
                    [](void*u,void*,const char*n){((Prop*)u)->sound->stop(n);return 0;},
                    [](void*u,void*,const char*n){((Prop*)u)->sound->play(n);return 0;},
                    [](void*u,void*,const char*n,float v){((Prop*)u)->sound->volume(n,v);return 0;},
                    [](void*u,int32_t group){auto&p=*(Prop*)u;uint32_t groups;memcpy(&groups,p.model.source.source_header+8,4);
                        need(group==0&&groups==1&&(p.empty_collision||p.courier),"Unsupported animated prop model group");
                        //1FB5E0 detaches/rebinds the selector, clearing its current
                        //controller. B5510 leaves a single-group model unchanged.
                        //The verified empty mdlhk creates no collision shapes;
                        //218210/21AF80 still publish these actual actor fields.
                        p.animations.current=nullptr;p.entity.flags|=0x800;
                        p.entity.actor.distance88=10000;p.entity.actor.distance8C=100;++p.owner->counts.helicopter_group_resets;return 0;}};
                need(!uc_animating_prop_state_update(&p.selection,s.flags,s.flag_count,&h),"Helicopter authored action");p.sound->step(dt);
            }
            if((p.vehicle||p.deferred)&&!p.courier){const auto&services=p.owner->world->script_services();need(!uc_animating_prop_select(&p.selection,services.flags,services.flag_count),"Vehicle shared conditions");
                const bool pending=p.selection.desired!=8;auto&count=p.deferred?p.owner->counts.deferred_prop_pending:p.owner->counts.vehicle_pending;if(p.pending!=pending){if(pending)++count;else --count;p.pending=pending;}
                // Explicit PC suspension before the unconnected action effects.
                // No action clip/marker/audio/OnStart is falsely completed.
                if(pending){++(p.deferred?p.owner->counts.deferred_prop_pending_ticks:p.owner->counts.vehicle_pending_ticks);return p.entity.script.update();}}
            auto *current=p.animations.current;const uint8_t done=!current||!(current->timing.flags&0x40000000u);const auto before=current;
            if(!p.helicopter&&!p.courier)need(!uc_animating_prop_idle(done,p.loop,&p.finished,start,&p),"Prop Idle restart");if(p.animations.current!=before)++p.owner->counts.prop_restarts;
            UCAnimationEventQueue queue{};p.animations.update(*p.owner->renderer,p.model,p.bank,p.owner->math,dt,queue);
            UCAnimationParticleServices h{&p,marker_world,marker_named,marker_attached,marker_fixed,marker_submit};
            for(uint32_t i=0;i<queue.count;i++){auto&ref=queue.events[i];uint8_t handled=0;need(!uc_animation_particle(ref.owner->events+ref.index,&p.entity.identity,&p.entity.id,&h,&handled)&&handled,"Unhandled prop animation event");}
            if((p.helicopter||p.courier)&&p.animations.current&&(p.animations.current->timing.flags&0x40000000u)){
                float movement[16];need(!uc_animation_binding_movement(&p.model.binding,dt,movement),"Helicopter root movement");store(p.entity.transform,matrix(movement)*matrix(p.entity.transform));
            }
            if(p.courier)p.refresh_box();
            return p.entity.script.update();}
        void refresh_box(){
            need(model.prepared.ready&&model.prepared.world_valid[collision_bone],"Van collision pose");float pose[16];
            need(!uc_character_collision_pose(model.source.bones[collision_bone].bind_correction,model.prepared.world[collision_bone],pose),"Van collision normalized pose");
            const auto swap=XMMatrixSet(1,0,0,0,0,0,1,0,0,1,0,0,0,0,0,1);
            auto w=swap*matrix(pose)*matrix(entity.transform)*swap;w.r[3]=XMVectorMultiply(w.r[3],XMVectorSet(.01f,.01f,.01f,1));
            auto composed=matrix(collision.transform)*w;XMStoreFloat3((XMFLOAT3*)box.center,composed.r[3]);
            XMStoreFloat4((XMFLOAT4*)box.rotation,XMQuaternionNormalize(XMQuaternionRotationMatrix(composed)));
            memcpy(box.half,collision.dimensions,12);box.radius=collision.radius;box_ready=true;
        }
        static int late(void*u){auto&p=*(Prop*)u;/* Admitted stationary visual actor: actual default zero-velocity tail. */if(!(p.entity.actor.word90&1)){for(unsigned i=12;i<15;i++)p.entity.transform[i]+=0.f;p.entity.transform[15]=1.f;}return 0;}
        static int remove(void*u){auto&p=*(Prop*)u;if(!p.helicopter&&!p.courier)return 1;
            p.sound.reset();p.entity.model=nullptr;if(uc_entity_fresh_aux_clear(p.entity.aux))return 1;
            p.entity.flags|=0x8100;uint8_t queued=0;if(uc_entity_retire(&p.owner->world->retirement(),p.entity.id,&queued)||!queued)return 2;
            if(p.entity.flags&0x1000){auto*f=p.owner->world->slot((uint32_t)p.entity.source).flags;*f=(*f&~15u)|16u;}p.entity.flags|=0x80000;return 0;}
        static int close(void*u){auto&p=*(Prop*)u;if(p.entity.script.close_detached())return 1;p.sound.reset();p.ready=false;p.box_ready=false;p.entity.model=nullptr;if(p.entity.aux&&uc_entity_fresh_aux_destroy(p.entity.aux))return 1;p.entity.aux=nullptr;return 0;}
    };
};
NativeLevelParticles::NativeLevelParticles(NativeRenderer&r,const std::filesystem::path&game,
    const std::filesystem::path&shaders,const UCLevel&level,bool include_fire):impl(new Impl){
    auto&s=*impl;need(level.entities&&level.entity_count,"Level particle descriptors");s.entries.reserve(128);s.sources.reserve(128);s.game=game;s.shaders=shaders;
    for(uint32_t i=0;i<level.entity_count;i++)if(!strcmp(level.entities[i].class_name,"ParticleSystem")){
        const auto&e=level.entities[i];const uint32_t expected[]={0,0,1,0,0};
        need(e.properties.size==sizeof expected&&!memcmp(e.properties.data,expected,sizeof expected)&&e.link_count==1&&e.active<=1,"Unsupported level particle attachment/gate");
        s.entries.push_back({i,&e});++s.counts.authored;if(e.active)++s.counts.deferred;else ++s.counts.initial;
    }
    for(uint32_t i=0;i<level.entity_count;i++)if(!strcmp(level.entities[i].class_name,"FXFire")){
        const auto&e=level.entities[i];++s.counts.fire_authored;if(e.active)++s.counts.fire_deferred;else ++s.counts.fire_initial;
        if(!include_fire)continue;
        // Exact Westside infinite, visible, no-child visual branch. Retain the
        // bank/hurt/obstacle fields as unconnected effects, never report success.
        const uint32_t tail[]={0,1,2,3,4,5,0xbf800000,1,6,1,0,0,0,0,1};
        need(e.properties.size==72&&e.link_count==7&&e.active<=1&&!memcmp((const char*)e.properties.data+12,tail,sizeof tail),"Unsupported FXFire visual properties");
        for(unsigned link=0;link<6;link++){Impl::Entry entry{i,&e};entry.fire=true;entry.link=link;s.entries.push_back(std::move(entry));}
    }
    need(!s.entries.empty()&&s.entries.size()<=4096,"Level particle capacity");
    uint8_t hash[32];auto archive=read_archive(game/"1-01.ent",hash);std::vector<const char*> names;
    for(auto&e:s.entries){BytesOwner b;auto at=find_resource(archive.get(),e.entity->links[e.link]);
        need(at!=SIZE_MAX&&uc_archive_read(archive.get(),at,UC_AR_LOCAL_PAYLOAD,&b.value)==UC_AR_OK&&!uc_particle_asset_load(b.value.data,b.value.size,&e.asset),"Level particle asset");
        // Retain the admitted asset families; heat follows the source mask
        // and end-of-view composition instead of drawing into scene color.
        need((e.asset.particle_type<=3||e.asset.particle_type==5)&&
            (e.asset.render_type==0||(e.fire&&e.asset.render_type==1))&&e.asset.blend<7&&!e.asset.emitter_type,"Unsupported level particle family");
        memcpy(e.source,e.entity->transform,64);memcpy(e.target,e.entity->transform,64);
        names.push_back(e.asset.texture);
    }
    for(size_t i=0;i<s.entries.size();i++)if(s.entries[i].fire&&s.entries[i].link==0){float rates[6],dimensions[3];for(unsigned k=0;k<6;k++)rates[k]=s.entries[i+k].asset.rate;
        memcpy(dimensions,s.entries[i].entity->properties.data,12);UCFireVisual visual{};
        need(!uc_fire_visual(dimensions,s.entries[i].entity->transform+12,rates,-1,&visual),"FXFire visual recipe");
        for(unsigned k=0;k<6;k++){auto&e=s.entries[i+k];e.asset.rate=visual.rate[k];memcpy(e.asset.simulation.size,visual.size,12);memcpy(e.source,visual.matrix,64);}
    }
    for(auto&e:s.entries)need(e.asset.rate*e.asset.simulation.lifetime<=maximum,"Level particle estimate capacity");
    need(!uc_texture_resources_create(archive.get(),1,names.data(),names.size(),&s.textures),"Level particle textures");
    for(size_t i=0;i<s.entries.size();i++){auto*t=uc_texture_resources_at(s.textures,i);need(t&&t->image&&!t->sequence,"Level particle resident texture");s.entries[i].image=r.upload(*t->image);s.entries[i].texture=t;}
    need(!uc_particle_registry_create(128,&s.registry),"Level particle registry");UCParticleLifecycleHooks hooks{Impl::add,Impl::remove};
    need(!uc_particle_lifecycle_create(128,&hooks,&s,&s.lifetime),"Level particle lifetime");
    uint32_t seed=1;need(!uc_particle_sprite_random(&seed,s.table),"Level particle rotation table");
    need(!uc_particle_quad_axes(&seed,s.axis_sine,s.axis_cosine),"Level particle quad axes");
    s.gpu.open(r.device.Get(),r.context.Get(),shader(shaders/"particle_billboard.hlsl"),shader(shaders/"particle.hlsl"));
    if(include_fire||std::any_of(s.entries.begin(),s.entries.end(),[](const auto&e){return e.asset.particle_type==1||e.asset.particle_type==2||e.asset.particle_type==5;})){s.quads.open(r.device.Get(),r.context.Get(),shader(shaders/"particle_vertex.hlsl"),shader(shaders/"particle.hlsl"));s.quads_ready=true;}
    D3D11_SAMPLER_DESC sd{};sd.Filter=D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;sd.AddressU=sd.AddressV=sd.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;sd.MaxLOD=D3D11_FLOAT32_MAX;
    need(SUCCEEDED(r.device->CreateSamplerState(&sd,&s.sampler)),"Level particle sampler");
    D3D11_DEPTH_STENCIL_DESC dd{};dd.DepthEnable=TRUE;dd.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ZERO;dd.DepthFunc=D3D11_COMPARISON_LESS_EQUAL;
    need(SUCCEEDED(r.device->CreateDepthStencilState(&dd,&s.depth)),"Level particle depth");
    const D3D11_BLEND factors[]={D3D11_BLEND_ZERO,D3D11_BLEND_ONE,D3D11_BLEND_SRC_COLOR,D3D11_BLEND_INV_SRC_COLOR,D3D11_BLEND_SRC_ALPHA,D3D11_BLEND_INV_SRC_ALPHA};
    for(unsigned i=0;i<7;i++){UCParticleRenderState state{};need(uc_particle_render_begin(&state,i)!=0,"Level particle blend");D3D11_BLEND_DESC d{};auto&b=d.RenderTarget[0];
        b.BlendEnable=state.blend_enabled;b.SrcBlend=factors[state.source_factor];b.DestBlend=factors[state.destination_factor];b.BlendOp=state.operation?D3D11_BLEND_OP_REV_SUBTRACT:D3D11_BLEND_OP_ADD;
        b.SrcBlendAlpha=D3D11_BLEND_ZERO;b.DestBlendAlpha=D3D11_BLEND_ONE;b.BlendOpAlpha=D3D11_BLEND_OP_ADD;b.RenderTargetWriteMask=7;need(SUCCEEDED(r.device->CreateBlendState(&d,&s.blends[i])),"Level particle blend state");}
    for(auto&e:s.entries)if(!e.entity->active)need(activate(e.descriptor),"Level particle startup");
}
NativeLevelParticles::~NativeLevelParticles()=default;
const NativeLevelParticleCounts&NativeLevelParticles::counts()const{return impl->counts;}
void NativeLevelParticles::prepare_props(NativeRenderer&r,const std::filesystem::path&assets,NativeMissionWorld&w,bool include_firebarrels,bool include_vehicles,bool include_deferred){auto&s=*impl;
    need(!s.world&&!s.prop_textures,"Prop resources already prepared");s.renderer=&r;s.world=&w;
    auto coefficients=read_file(assets/"animation-coefficients.bin",16384);need(!uc_animation_blend_math_load(coefficients.data(),coefficients.size(),&s.math),"Prop animation coefficients");
    // The archive is the same level source, not a guessed replacement model.
    uint8_t hash[32];auto archive=read_archive(s.game/"1-01.ent",hash);
    std::vector<const char*> names;std::vector<Impl::Entry*> prepared;
    for(uint32_t i=0;i<w.source_count();i++){
        const auto&d=*w.metadata(i).definition;if(!d.class_name||strcmp(d.class_name,"AnimatingProp"))continue;++s.counts.prop_authored;if(!d.asset_name)continue;
        const bool wind=!_stricmp(d.asset_name,"Assets_Scene_items\\generic_fx\\SIS_windysmoke\\SIS_windysmoke_skin.amf");
        const bool mist=!_stricmp(d.asset_name,"Assets_Scene_items\\01-01\\SIS_mist\\SIS_mist_skin.amf");
        const bool barrel=include_firebarrels&&!_stricmp(d.asset_name,"Assets_Scene_items\\01-01\\SIS_firebarrel\\SIS_firebarrel_skin.amf");
        const bool truck=include_vehicles&&!_stricmp(d.asset_name,"Assets_Scene_items\\01-01\\SIL_trucksliding\\SIL_trucksliding_skin.amf");
        const bool ambulance=include_vehicles&&!_stricmp(d.asset_name,"Assets_Scene_items\\01-01\\SIL_new_ambulance\\SIL_newambulance_skin.amf");const bool vehicle=truck||ambulance;
        const bool courier=include_deferred&&!_stricmp(d.asset_name,"Assets_Scene_items\\01-01\\SIS_Courier_Van\\SIS_CourierVan_skin.amf");
        const bool endpart=include_deferred&&!_stricmp(d.asset_name,"Assets_Scene_items\\01-01\\SIL_endpart\\SIL_endpart_skin.amf");const bool deferred=courier||endpart;
        if(!wind&&!mist&&!barrel&&!vehicle&&!deferred)continue;
        need(d.active==(deferred?1u:0u)&&d.link_count==1&&d.properties.size==(truck?316u:ambulance||i==556?312u:deferred?308u:304u),"Unsupported visual prop gate/bank");
        const char*bank=truck?"Banks\\Setpiece\\01-01\\BurnersHitCopCar.amp":ambulance?"Banks\\Setpiece\\01-01\\ExplodingAmbulance.amp":courier?"Banks\\Setpiece\\01-01\\ExplodingVan.amp":endpart?"Banks\\Setpiece\\01-01\\HostageAirCon.amp":"Banks\\\\Default.amp";
        need(!_stricmp(d.links[0],bank),"Unsupported visual prop bank");
        // Exact source-decoded Westside profile. Other conditions, state changes,
        // sound restart and physics profiles must acquire their real owners.
        uint32_t expected[76]{};expected[1]=UINT32_MAX;expected[2]=expected[22]=expected[72]=1;expected[13]=wind?1:0;
        for(unsigned j=32;j<=66;j+=2)expected[j]=UINT32_MAX;expected[68]=0x41200000;expected[71]=0x3e4ccccd;
        need(s.entries.size()<s.entries.capacity(),"Prop resource pointer capacity");s.entries.push_back({i,&d});auto&e=s.entries.back();e.prop=true;
        auto prop=std::make_unique<Impl::Prop>(&s,&e);prop->loop=wind||deferred?1:0;prop->courier=courier;prop->vehicle=vehicle;prop->deferred=deferred;prop->selection.current=prop->selection.desired=8;
        if(vehicle||deferred){std::vector<uint32_t> normalized;auto*raw=(const uint8_t*)d.properties.data;size_t at=0;auto word=[&](){need(at+4<=d.properties.size,"Vehicle property bounds");uint32_t value;memcpy(&value,raw+at,4);at+=4;return value;};
            for(unsigned j=0;j<4;j++)normalized.push_back(word());
            for(unsigned j=0;j<9;j++){unsigned action=j?j-1:8,count=word();need(count<=1,"Unsupported vehicle conditions");normalized.push_back(0);for(unsigned k=0;k<count;k++)prop->conditions[action].push_back((int32_t)word());prop->selection.conditions[action]={(int32_t)count,prop->conditions[action].data()};}
            while(at<d.properties.size)normalized.push_back(word());need(normalized.size()==76,"Vehicle property projection");expected[2]=0;
            if(truck){expected[37]=1;expected[38]=0;}else if(ambulance){expected[3]=1;expected[53]=1;expected[54]=0;expected[75]=1;}else{expected[13]=1;if(endpart)expected[2]=1;if(i==556||endpart){expected[33]=1;expected[34]=0;}}
            const int32_t truck_flags[]={18,45,46},ambulance_flags[]={28,68};for(unsigned j=0;j<9;j++){
                int32_t flag=truck?(j<3?truck_flags[j]:-1):ambulance?(j<2?ambulance_flags[j]:-1):courier?(j==0&&i==556?30:j==1?44:-1):j==0?120:-1;
                need(prop->conditions[j].size()==(flag<0?0u:1u)&&(flag<0||prop->conditions[j][0]==flag),"Prop authored condition identity");}
            for(unsigned j=0;j<9;j++){unsigned order=j==8?0:j+1;
                prop->selection.loop[j]=(uint8_t)normalized[13+order];prop->selection.interrupt[j]=(uint8_t)normalized[22+order];
                prop->selection.start_event[j]=(int32_t)normalized[32+2*order];prop->selection.finish_event[j]=(int32_t)normalized[50+2*order];}
            prop->selection.stop_previous=(uint8_t)normalized[73];prop->selection.restart_audio=(uint8_t)normalized[3];prop->selection.volume=1;
            prop->selection.entity_flags=&prop->entity.flags;prop->selection.disabled=&prop->disabled;prop->selection.model=&prop->entity.model;prop->selection.sound_name="idle";
            prop->selection.return_to_idle=(uint8_t)expected[2];
            need(!memcmp(normalized.data(),expected,sizeof expected),"Unsupported vehicle behavior profile");
        }else need(!memcmp(expected,d.properties.data,sizeof expected),"Unsupported AnimatingProp behavior profile");
        prop->model.load(r,archive.get(),hash,assets/"1-01.ent.ucmat",d.asset_name,nullptr);
        for(const auto&m:prop->model.materials)need(m.supported(),"Unsupported visual prop material");
        prop->bank.load(archive.get(),prop->model.source.animation_template,prop->model.source.animation_template);
        need(prop->bank.catalog.clip_count==(truck?4u:ambulance||courier?3u:endpart?2u:1u),"Unsupported visual prop action table");
        need(!uc_animation_binding_init(&prop->model.source,&prop->bank.skeleton,&prop->model.binding),"Prop pose binding");
        if(courier){
            std::string collision_name=d.asset_name;collision_name.replace(collision_name.find_last_of('.'),std::string::npos,".mdlhk");
            BytesOwner bytes;size_t index=find_resource(archive.get(),collision_name.c_str());UCModelCollisionAsset asset{};
            need(index!=SIZE_MAX&&!uc_archive_read(archive.get(),index,UC_AR_LOCAL_PAYLOAD,&bytes.value)&&!uc_model_collision_load(bytes.value.data,bytes.value.size,0,&asset),"Van authored collision asset");
            std::unique_ptr<UCModelCollisionAsset,decltype(&uc_model_collision_free)> cleanup(&asset,uc_model_collision_free);
            need(asset.group_count==1&&asset.shape_count==1&&asset.shapes[0].group==0&&asset.shapes[0].object==0&&asset.shapes[0].metadata_size==6&&!uc_rigid_prop_box(asset.shapes,&prop->collision),"Van collision box");
            memcpy(&prop->collision_bone,asset.shapes[0].metadata,2);need(prop->collision_bone<prop->model.source.bone_count,"Van collision bone");
            for(const auto&clip:prop->bank.clips)for(uint32_t k=0;k<clip->annotations.event_count;k++){
                const auto&event=clip->annotations.events[k];need(event.code==35&&!_stricmp(event.type,"SimpleParticleSystem")&&event.parameter_count==3,"Van animation marker");
                const char*resource=event.parameters[0].value;
                if(std::any_of(prop->emitters.begin(),prop->emitters.end(),[&](const auto*x){return !strcmp(x->resource,resource);}))continue;
                Impl::Entry*emitter=&e;if(!prop->emitters.empty()){need(s.entries.size()<s.entries.capacity(),"Van particle capacity");s.entries.push_back({i,&d});emitter=&s.entries.back();emitter->prop=true;}
                emitter->resource=resource;BytesOwner particle;index=find_resource(archive.get(),resource);
                need(index!=SIZE_MAX&&!uc_archive_read(archive.get(),index,UC_AR_LOCAL_PAYLOAD,&particle.value)&&!uc_particle_asset_load(particle.value.data,particle.value.size,&emitter->asset),"Van particle asset");
                if(!((emitter->asset.particle_type<=3||emitter->asset.particle_type==5)&&emitter->asset.blend<7&&emitter->asset.emitter_type<=1))throw std::runtime_error(std::string("Van particle ")+resource+" type="+std::to_string(emitter->asset.particle_type)+" emitter="+std::to_string(emitter->asset.emitter_type)+" blend="+std::to_string(emitter->asset.blend));
                names.push_back(emitter->asset.texture);prepared.push_back(emitter);prop->emitters.push_back(emitter);
            }
        }
        const auto&a=prop->bank.find("Idle").annotations;need(a.event_count==(vehicle||deferred?0u:barrel?3u:1u),"Unsupported visual prop event count");
        for(uint32_t marker=0;marker<a.event_count;marker++){const auto&event=a.events[marker];
         if(marker){need(s.entries.size()<s.entries.capacity(),"Prop resource pointer capacity");s.entries.push_back({i,&d});s.entries.back().prop=true;}
         need(event.phase==0&&event.code==35&&!_stricmp(event.type,"SimpleParticleSystem")&&event.parameter_count==3&&
            !strcmp(event.parameters[1].value,wind?"1":"0")&&!strcmp(event.parameters[2].value,"fx01"),"Unsupported visual prop marker");
         auto&emitter=marker?s.entries.back():e;emitter.resource=event.parameters[0].value;
         BytesOwner bytes;auto at=find_resource(archive.get(),emitter.resource);need(at!=SIZE_MAX&&uc_archive_read(archive.get(),at,UC_AR_LOCAL_PAYLOAD,&bytes.value)==UC_AR_OK&&!uc_particle_asset_load(bytes.value.data,bytes.value.size,&emitter.asset),"Prop particle asset");
        // Original property loading halves the authored constant rate (5 -> 2.5).
         const float rates[]={15,2.5f,1.5f};const char*files[]={"01-01\\firebarrel_fire01.simpleparticlesystem","01-01\\firebarrel_blaze01.simpleparticlesystem","01-01\\firebarrel_ember01.simpleparticlesystem"};
         need((!barrel||!strcmp(emitter.resource,files[marker]))&&emitter.asset.particle_type==(barrel&&marker==0?5u:3u)&&emitter.asset.blend<7&&!emitter.asset.emitter_type&&emitter.asset.rate==(barrel?rates[marker]:2.5f)&&emitter.asset.emitter_lifetime==(wind?5.f:0.f),"Unsupported prop particle recipe");
         names.push_back(emitter.asset.texture);prepared.push_back(&emitter);prop->emitters.push_back(&emitter);
        }
        s.props.push_back(std::move(prop));++s.counts.prop_supported;if(deferred)++s.counts.deferred_prop_supported;
    }
    need(s.props.size()==6u+(include_firebarrels?2u:0u)+(include_vehicles?2u:0u)+(include_deferred?3u:0u),"Unexpected authored visual prop inventory");need(!uc_texture_resources_create(archive.get(),1,names.data(),names.size(),&s.prop_textures),"Prop particle textures");
    for(size_t i=0;i<prepared.size();i++){auto&e=*prepared[i];e.texture=uc_texture_resources_at(s.prop_textures,i);need(e.texture&&e.texture->image&&!e.texture->sequence,"Prop resident image");e.image=r.upload(*e.texture->image);}
    if(include_firebarrels&&!s.quads_ready){s.quads.open(r.device.Get(),r.context.Get(),shader(s.shaders/"particle_vertex.hlsl"),shader(s.shaders/"particle.hlsl"));s.quads_ready=true;}
}
void NativeLevelParticles::prepare_helicopters(NativePlayerAudio&audio,const std::filesystem::path&assets){auto&s=*impl;
    need(s.world&&s.renderer&&!s.helicopter_textures,"Helicopter preparation order");
    uint8_t hash[32];auto archive=read_archive(s.game/"1-01.ent",hash);
    for(auto&p:s.props)if(p->courier){size_t index=find_resource(archive.get(),p->entry->entity->links[0]);need(index!=SIZE_MAX,"Van sound bank");
        p->sound=std::make_unique<NativePlayerAudio::MissionSource>(audio,archive.get(),index,s.game/"5.saf");}
    std::vector<const char*> names;std::vector<Impl::Entry*> prepared;
    for(uint32_t i:{530u,996u}){
        const auto&d=*s.world->metadata(i).definition;
        need(!strcmp(d.class_name,"AnimatingPropIgnorePVS")&&d.active==1&&d.link_count==1&&
            !_stricmp(d.asset_name,"Assets_Scene_items\\01-01\\SIL_interiorhelicopter\\SIL_interiorhelicopter_skin.amf"),"Helicopter authored identity");
        need(s.entries.size()<s.entries.capacity(),"Helicopter entry capacity");s.entries.push_back({i,&d});auto&e=s.entries.back();e.prop=true;
        auto p=std::make_unique<Impl::Prop>(&s,&e);p->helicopter=true;p->selection.current=p->selection.desired=8;
        std::vector<uint32_t> normalized;size_t at=0;auto word=[&](){need(at+4<=d.properties.size,"Helicopter property bounds");uint32_t n;memcpy(&n,d.properties.data+at,4);at+=4;return n;};
        for(unsigned j=0;j<4;j++)normalized.push_back(word());
        for(unsigned j=0;j<9;j++){unsigned action=j?j-1:8,n=word();need(n<=1,"Helicopter condition count");normalized.push_back(0);for(unsigned k=0;k<n;k++)p->conditions[action].push_back((int32_t)word());p->selection.conditions[action]={(int32_t)n,p->conditions[action].data()};}
        while(at<d.properties.size)normalized.push_back(word());need(normalized.size()==75,"Helicopter property size");
        uint32_t expected[75]{};expected[1]=UINT32_MAX;expected[2]=expected[3]=expected[15]=expected[17]=expected[22]=expected[24]=expected[26]=expected[55]=expected[72]=1;
        for(unsigned j=32;j<=66;j+=2)expected[j]=UINT32_MAX;expected[68]=0x41200000;expected[71]=0x3e4ccccd;expected[56]=i==530?0:1;
        if(i==530)expected[30]=1;else {expected[33]=1;expected[34]=0;expected[73]=1;}
        need(!memcmp(normalized.data(),expected,sizeof expected),"Unsupported helicopter profile");
        const int32_t flags[]={112,113,114,136,137};for(unsigned j=0;j<9;j++){int32_t flag=i==530?(j==7?116:-1):(j<5?flags[j]:-1);
            need(p->conditions[j].size()==(flag<0?0u:1u)&&(flag<0||p->conditions[j][0]==flag),"Helicopter condition identity");
            unsigned order=j==8?0:j+1;p->selection.loop[j]=(uint8_t)normalized[13+order];p->selection.interrupt[j]=(uint8_t)normalized[22+order];
            p->selection.start_event[j]=(int32_t)normalized[32+2*order];p->selection.finish_event[j]=(int32_t)normalized[50+2*order];
        }
        p->selection.return_to_idle=1;p->selection.stop_previous=(uint8_t)normalized[73];p->selection.volume=1;
        p->selection.entity_flags=&p->entity.flags;p->selection.disabled=&p->disabled;p->selection.model=&p->entity.model;p->selection.sound_name="idle";
        size_t prefix=0;p->model.load(*s.renderer,archive.get(),hash,assets/"1-01.ent.ucmat",d.asset_name,nullptr,nullptr,nullptr,UINT32_MAX,&prefix);
        BytesOwner full;size_t model_index=find_resource(archive.get(),d.asset_name);
        const uint8_t tail[]={2,0,0,0,0xc5,0x99,0xbb,0x3f,0x11,0,0,0,0xc5,0x99,0xbb,0x3f};
        need(model_index!=SIZE_MAX&&!uc_archive_read(archive.get(),model_index,UC_AR_LOCAL_PAYLOAD,&full.value)&&prefix==267787&&
            full.value.size==prefix+sizeof tail&&!memcmp(full.value.data+prefix,tail,sizeof tail),"Helicopter model prefix changed");
        const uint32_t empty_collision[]={1,1,0,0,0,0};BytesOwner collision;
        std::string collision_name=d.asset_name;collision_name.replace(collision_name.find_last_of('.'),std::string::npos,".mdlhk");
        size_t collision_index=find_resource(archive.get(),collision_name.c_str());
        need(collision_index!=SIZE_MAX&&!uc_archive_read(archive.get(),collision_index,UC_AR_LOCAL_PAYLOAD,&collision.value)&&
            collision.value.size==sizeof empty_collision&&!memcmp(collision.value.data,empty_collision,sizeof empty_collision),"Helicopter empty collision profile");p->empty_collision=true;
        for(const auto&m:p->model.materials)need(m.supported(),"Helicopter material");
        p->bank.load(archive.get(),p->model.source.animation_template,p->model.source.animation_template);
        need(p->bank.catalog.clip_count==7&&!uc_animation_binding_init(&p->model.source,&p->bank.skeleton,&p->model.binding),"Helicopter animation binding");
        p->sound=std::make_unique<NativePlayerAudio::MissionSource>(audio,archive.get(),find_resource(archive.get(),d.links[0]),s.game/"5.saf");
        for(const auto&clip:p->bank.clips)for(uint32_t k=0;k<clip->annotations.event_count;k++){
            const auto&event=clip->annotations.events[k];need(event.code==35&&!_stricmp(event.type,"SimpleParticleSystem")&&event.parameter_count==3,"Helicopter animation marker");
            const char*resource=event.parameters[0].value;
            if(std::any_of(p->emitters.begin(),p->emitters.end(),[&](const auto*x){return !strcmp(x->resource,resource);}))continue;
            Impl::Entry*emitter=&e;if(!p->emitters.empty()){need(s.entries.size()<s.entries.capacity(),"Helicopter emitter capacity");s.entries.push_back({i,&d});emitter=&s.entries.back();emitter->prop=true;}
            emitter->resource=resource;BytesOwner bytes;size_t index=find_resource(archive.get(),resource);
            need(index!=SIZE_MAX&&!uc_archive_read(archive.get(),index,UC_AR_LOCAL_PAYLOAD,&bytes.value)&&!uc_particle_asset_load(bytes.value.data,bytes.value.size,&emitter->asset),"Helicopter particle resource");
            need((emitter->asset.particle_type<=3||emitter->asset.particle_type==5)&&emitter->asset.blend<7&&!emitter->asset.emitter_type,"Helicopter particle type");
            names.push_back(emitter->asset.texture);prepared.push_back(emitter);p->emitters.push_back(emitter);
        }
        s.props.push_back(std::move(p));++s.counts.prop_supported;
    }
    need(!uc_texture_resources_create(archive.get(),1,names.data(),names.size(),&s.helicopter_textures),"Helicopter particle textures");
    for(size_t i=0;i<prepared.size();i++){auto&e=*prepared[i];e.texture=uc_texture_resources_at(s.helicopter_textures,i);need(e.texture&&e.texture->image&&!e.texture->sequence,"Helicopter texture");e.image=s.renderer->upload(*e.texture->image);}
    if(!s.quads_ready){s.quads.open(s.renderer->device.Get(),s.renderer->context.Get(),shader(s.shaders/"particle_vertex.hlsl"),shader(s.shaders/"particle.hlsl"));s.quads_ready=true;}
}
int NativeLevelParticles::construct_particle(NativeMissionWorld&w,uint32_t index){auto&s=*impl;
    need(s.world==&w&&index<w.source_count(),"Particle actor world ownership");
    const auto&m=w.metadata(index);const auto&d=*m.definition;
    auto at=std::find_if(s.entries.begin(),s.entries.end(),[&](const auto&e){return e.descriptor==index&&!e.fire&&!e.prop;});
    need(at!=s.entries.end()&&*w.active_descriptor()==&m,"Particle actor descriptor ownership");
    need(!m.asset_present&&!m.condition_count&&!d.script_count,"Particle actor unsupported asset/conditions/script");
    for(const auto&a:s.actors)need(a->descriptor!=index,"Particle actor duplicate construction");
    auto owned=std::make_unique<Impl::Actor>();auto&a=*owned;a.owner=this;a.descriptor=index;a.target=at->target;s.actors.push_back(std::move(owned));
    need(!w.bind(index,{&a.entity.view,&a,Impl::Actor::update,Impl::Actor::late,Impl::Actor::close,&a.entity.script}),"Particle actor world binding");
    need(!uc_entity_fresh_construct_authored(&w.context(),&a.entity.view,w.active_descriptor()),"Particle actor registration");
    need(!a.entity.script.attach_actor(a.entity.view,m,w.script_services()),"Particle actor script attachment");
    *w.property_cursor(index)=d.properties.data+d.properties.size;
    // Qualified stationary self attachment (156920/219530). General moving,
    // gated and destructible particle actors still require their own lifecycle.
    return activate(index)?0:2;
}
int NativeLevelParticles::construct_prop(NativeMissionWorld&w,uint32_t index){auto&s=*impl;if(s.world!=&w)return 1;
    auto at=std::find_if(s.props.begin(),s.props.end(),[&](const auto&p){return p->entry->descriptor==index;});if(at==s.props.end()||(*at)->ready)return 1;auto&p=**at;
    if(*w.active_descriptor()!=&w.metadata(index))return 1;
    NativeMissionBinding b{&p.entity.view,&p,Impl::Prop::update,Impl::Prop::late,Impl::Prop::close,&p.entity.script};if(p.helicopter||p.courier){b.remove=Impl::Prop::remove;b.destroy=Impl::Prop::close;}if(w.bind(index,b))return 1;
    if(uc_entity_fresh_construct_authored(&w.context(),&p.entity.view,w.active_descriptor()))return 2;
    p.entity.model=&p.model;if(p.entity.script.attach_actor(p.entity.view,w.metadata(index),w.script_services()))return 2;
    *w.property_cursor(index)=p.entry->entity->properties.data+p.entry->entity->properties.size;
    p.attachment={&p,&w.registry(),Impl::Prop::find,Impl::Prop::bone,Impl::Prop::world,Impl::Prop::flags};
    const float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};p.model.bind_pose(*s.renderer,identity);Impl::Prop::start(&p);if(p.courier)p.refresh_box();p.ready=true;++s.counts.prop_active;if(p.vehicle)++s.counts.vehicle_active;if(p.deferred)++s.counts.deferred_prop_active;return 0;
}
int32_t NativeLevelParticles::pending_prop_action(uint32_t descriptor)const{for(auto&p:impl->props)if(p->ready&&p->entry->descriptor==descriptor)return p->pending?p->selection.desired:-1;return -1;}
void NativeLevelParticles::draw_props(NativeRenderer&r,const float vp[16],float exposure,uint32_t descriptor,NativeModelLighting*lighting,const NativeLightingFrame*frame){auto&s=*impl;for(auto&p:s.props)if(p->ready&&(descriptor==UINT32_MAX||p->entry->descriptor==descriptor)){float composed[16];store(composed,matrix(p->entity.transform)*matrix(vp));const float tint[4]={1,1,1,1};const NativeTextureContext context{r.material_phase};
 if(lighting&&frame)p->model.draw_lit(r,*lighting,*frame,p->entity.transform,composed,tint,&context,exposure);
 else p->model.draw(r,composed,tint,&context,exposure);}}
bool NativeLevelParticles::activate(uint32_t descriptor){auto&s=*impl;
    auto it=std::find_if(s.entries.begin(),s.entries.end(),[&](const Impl::Entry&e){return e.descriptor==descriptor;});if(it==s.entries.end()||it->prop)return false;if(it->active)return true;
    const bool fire=it->fire;do{if(!it->started){UCParticleSource source{&*it,
        [](void*p,uint8_t*out){*out=!((Impl::Entry*)p)->active;return 0;},
        [](void*p,const UCParticleBounds*,float*out){memcpy(out,((Impl::Entry*)p)->source,64);return 0;}};
        need(!uc_particle_lifecycle_append_source(s.lifetime,&it->asset,&source),"Level particle append");it->started=true;}it->active=true;++it;}while(fire&&it!=s.entries.end()&&it->descriptor==descriptor);
    if(fire)++s.counts.fire_active;else ++s.counts.active;return true;
}
bool NativeLevelParticles::deactivate(uint32_t descriptor){bool found=false,active=false,fire=false;
    for(auto&e:impl->entries)if(e.descriptor==descriptor&&!e.prop){active|=e.active;fire=e.fire;e.active=false;found=true;}
    if(active){if(fire)--impl->counts.fire_active;else --impl->counts.active;}return found;}
void NativeLevelParticles::step(float delta,UCGameRandom&rng){auto&s=*impl;need(std::isfinite(delta)&&delta>0&&delta<=1,"Level particle delta");
    // These real attached targets remain alive and stationary in this adapter.
    // Reject a changed target instead of silently retaining a stale fixed source.
    for(uint32_t i=0;i<uc_particle_lifecycle_count(s.lifetime);i++){auto*v=uc_particle_lifecycle_at(s.lifetime,i);auto e=std::find_if(s.entries.begin(),s.entries.end(),[&](const Impl::Entry&e){return &e.asset==v->asset;});need(e!=s.entries.end(),"Level particle source identity");
        if(!e->prop)need(!memcmp(e->target,e->entity->transform,64),"Moving level particle target is unconnected");}
    const float wind[3]{};need(!uc_particle_lifecycle_step(s.lifetime,delta,wind,&rng),"Level particle step");++s.counts.ticks;s.counts.live_particles=s.counts.fire_live_particles=0;
    s.counts.prop_live_particles=0;for(uint32_t i=0;i<uc_particle_lifecycle_count(s.lifetime);i++){auto*v=uc_particle_lifecycle_at(s.lifetime,i);s.counts.live_particles+=v->system->count;auto e=std::find_if(s.entries.begin(),s.entries.end(),[&](const Impl::Entry&e){return &e.asset==v->asset;});if(e->fire)s.counts.fire_live_particles+=v->system->count;if(e->prop)s.counts.prop_live_particles+=v->system->count;}
}
void NativeLevelParticles::draw(NativeRenderer&r,const float view[16],const float projection[16],uint32_t descriptor){auto&s=*impl;s.counts.draws=0;bool heat_started=false;float camera[16],clip[16];store(camera,XMMatrixInverse(nullptr,matrix(view)));store(clip,matrix(view)*matrix(projection));
    ComPtr<ID3D11BlendState> old_blend;float factors[4];UINT mask;ComPtr<ID3D11DepthStencilState> old_depth;UINT reference;ComPtr<ID3D11RasterizerState> old_raster;
    r.context->OMGetBlendState(&old_blend,factors,&mask);r.context->OMGetDepthStencilState(&old_depth,&reference);r.context->RSGetState(&old_raster);r.context->OMSetDepthStencilState(s.depth.Get(),0);
    NativeParticleBillboardView frame{};for(unsigned i=0;i<4;i++)for(unsigned j=0;j<4;j++)frame.clip_rows[i][j]=clip[j*4+i];memcpy(frame.right,camera,16);memcpy(frame.up,camera+4,16);
    NativeParticleQuadView qv{};memcpy(qv.clip_rows,frame.clip_rows,sizeof qv.clip_rows);qv.fog[3]=1;
    for(auto token=uc_particle_registry_first(s.registry);token;token=uc_particle_registry_next(s.registry,token)){
        auto*v=uc_particle_registry_view(s.registry,token);auto e=std::find_if(s.entries.begin(),s.entries.end(),[&](const Impl::Entry&e){return &e.asset==v->asset;});need(e!=s.entries.end()&&v->system->count<=maximum,"Level particle geometry");
        if(descriptor!=UINT32_MAX&&e->descriptor!=descriptor)continue;
        UCParticleSpriteAdmission admission{};auto*texture=e->texture;
        if(v->asset->particle_type==0){UCParticleColorCurve curve{};need(!uc_particle_plain_prepare(v->asset,0,1,0,v->system->count,texture,&curve,&admission)&&!uc_particle_plain_draw(v->asset,&curve,v->system->particles,admission.count,camera,0,0,0,s.plain.data()),"Level smoke geometry");
            for(unsigned i=0;i<admission.count;i++){auto&a=s.plain[i];auto&b=s.sprites[i];memcpy(b.center,a.center,12);b.radius=a.radius;b.angle=0;memcpy(b.color,a.color,16);}}
        else if(v->asset->particle_type==2){UCParticleSpriteState state{};need(s.quads_ready&&!uc_particle_sprite_prepare(v->asset,0,1,0,v->system->count,texture,&state,&admission)&&!uc_particle_rotating_quads(v->asset,&state,v->system->particles,admission.count,camera,s.table,s.axis_sine,s.axis_cosine,0,s.quad_scratch.data()),"Level paper rotating geometry");}
        else if(v->asset->particle_type==5){UCParticleStretchState state{};need(!uc_particle_stretch_prepare(v->asset,0,1,v->system->count,texture,&state,&admission)&&!uc_particle_stretched_quads(v->asset,&state,v->system->particles,admission.count,camera,s.table,0,s.quad_scratch.data()),"Level fire stretched geometry");}
        else need(!uc_particle_lifecycle_geometry(v,camera,s.table,texture,0,1,0,0,s.quad_scratch.data(),s.sprites.data(),maximum,&admission),"Level particle geometry");
        if(!admission.count)continue;
        need(v->asset->render_type<=1,"Unsupported particle render type");const bool heat=v->asset->render_type==1;
        UCParticleRenderState state{};need(uc_particle_render_begin(&state,admission.blend)!=0,"Level particle draw state");qv.alpha_test=frame.alpha_test=state.alpha_test;qv.texture_alpha_kill=frame.texture_alpha_kill=state.texture_alpha_kill;
        if(heat){
            if(!heat_started){if(!s.heat||s.heat->width()!=r.width||s.heat->height()!=r.height){s.heat=std::make_unique<NativeHeatHaze>();s.heat->open(r,shader(s.shaders/"environment_postprocess.hlsl"));}s.heat->begin(r);heat_started=true;}
            s.heat->bind_mask(r);
        }else r.context->OMSetBlendState(s.blends[admission.blend].Get(),nullptr,~0u);
        if(v->asset->particle_type==1||v->asset->particle_type==2||v->asset->particle_type==5){for(unsigned start=0;start<admission.count;start+=128){unsigned n=std::min(128u,admission.count-start);UCParticleQuadVertex out[128*4];need(!uc_particle_quad_batch(s.quad_scratch.data()+start,n,out),"Level particle quad batch");s.quads.draw(out,n,qv,e->image.Get(),s.sampler.Get(),heat);}}
        else for(unsigned start=0;start<admission.count;start+=39){unsigned n=std::min(39u,admission.count-start);UCParticleBillboard out[39];need(!(heat&&v->asset->particle_type==3?uc_particle_billboard_heat_batch(s.sprites.data()+start,n,out):uc_particle_billboard_batch(s.sprites.data()+start,n,out)),"Level particle batch");s.gpu.draw(out,n,frame,e->image.Get(),s.sampler.Get(),heat);}++s.counts.draws;
        if(heat){ID3D11RenderTargetView*target=r.target.Get();r.context->OMSetRenderTargets(1,&target,r.depth_view.Get());}
    }
    if(heat_started)s.heat->finish(r,r.material_phase);
    r.context->VSSetShader(r.vertex_shader.Get(),nullptr,0);r.context->PSSetShader(r.pixel_shader.Get(),nullptr,0);r.context->IASetInputLayout(r.layout.Get());r.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);ID3D11Buffer*cb=r.constants.Get();r.context->VSSetConstantBuffers(0,1,&cb);r.context->PSSetConstantBuffers(0,1,&cb);
    r.context->OMSetBlendState(old_blend.Get(),factors,mask);r.context->OMSetDepthStencilState(old_depth.Get(),reference);r.context->RSSetState(old_raster.Get());
}

void NativeLevelParticles::append_blocks(std::vector<const UCSolidBlockGeometry*>&out)const{for(const auto&p:impl->props)if(p->ready&&p->box_ready)out.push_back(&p->box);}
void NativeLevelParticles::append_collision(NativeCharacterWorld&w)const{for(const auto&p:impl->props)if(p->ready&&p->box_ready){uint32_t material;memcpy(&material,p->collision.metadata+2,4);
    w.boxes.push_back({&p->box,{p->collision.root_kind,material,0x03800000u|p->entity.actor.group94,1,0,0,0},p->entity.id,{&p->box,1,1,0,0},&p->entity});}}

bool NativeLevelParticles::collision_owner(int32_t id)const{for(const auto&p:impl->props)if(p->ready&&p->box_ready&&p->entity.id==id)return true;return false;}
