extern "C" {
#include "../particle_asset.h"
#include "../particle_sprite.h"
}
#include "npc_thrown_particles.h"
#include "npc_thrown_weapon.h"
#include "rigid_prop.h"
#include "mission_character.h"
#include "particle_billboards.h"
#include "particle_quads.h"
#include "heat_haze.h"
extern "C" {
#include "../particle_registry.h"
#include "../particle_stretch.h"
#include "../particle_rotating_quad.h"
#include "../particle_render_state.h"
#include "../particle_attachment.h"
#include "../npc_thrown_trigger.h"
#include "../texture_resources.h"
#include "../explosion_children.h"
}
#include <DirectXMath.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <cmath>
#include <stdexcept>
using namespace DirectX;
namespace {
void need(bool v,const char*t){if(!v)throw std::runtime_error(t);}
std::string source(const std::filesystem::path&p){auto b=read_file(p,1024*1024);return {b.begin(),b.end()};}
XMMATRIX matrix(const float*p){XMFLOAT4X4 m;memcpy(&m,p,64);return XMLoadFloat4x4(&m);}
void store(float*p,FXMMATRIX m){XMFLOAT4X4 out;XMStoreFloat4x4(&out,m);memcpy(p,&out,64);}
constexpr unsigned capacity=256,geometry_capacity=4096;
}
struct NativeNpcThrownParticles::Impl {
 struct Particle {const char*name;UCParticleAsset asset{};ComPtr<ID3D11ShaderResourceView> image;};
 struct Source {NativeNpcThrownWeapon*weapon=nullptr;NativeRigidProp*prop=nullptr;NativeMissionCharacter*character=nullptr;NativeMissionEntity*entity=nullptr;NativeMissionWorld*world=nullptr;UCParticleAttachment attachment{};UCParticleAttachmentServices services{};const UCParticleLifecycleView*view=nullptr;bool ring=false,cold=false;UCParticleAsset ring_asset{};float fixed[16]{};};
 NativeMissionWorld*world;std::vector<Particle>particles;std::vector<std::unique_ptr<Source>>sources;Source*registering=nullptr;
 UCTextureResources*textures=nullptr;UCParticleRegistry*registry=nullptr;UCParticleLifecycle*lifetime=nullptr;
 NativeParticleQuads quads;NativeParticleBillboards sprites;std::unique_ptr<NativeHeatHaze>heat;std::filesystem::path shaders;
 ComPtr<ID3D11BlendState>blends[7];ComPtr<ID3D11SamplerState>sampler;ComPtr<ID3D11DepthStencilState>depth;ComPtr<ID3D11RasterizerState>raster;
 float table[16]{},axis_sine[16]{},axis_cosine[16]{};
 std::array<UCParticleQuad,geometry_capacity>quad_scratch;std::array<UCParticleSprite,geometry_capacity>sprite_scratch;std::array<UCParticlePlainSprite,geometry_capacity>plain_scratch;
 ~Impl(){if(lifetime)uc_particle_lifecycle_close(lifetime);if(registry)uc_particle_registry_close(registry);uc_texture_resources_close(textures);}
 static uintptr_t add(void*p,const UCParticleLifecycleView*v){auto&s=*(Impl*)p;uintptr_t token=0;need(!uc_particle_registry_add(s.registry,v,&token)&&token,"Thrown particle registry full");if(s.registering)s.registering->view=v;return token;}
 static void remove(void*p,uintptr_t token,const UCParticleLifecycleView*v){auto&s=*(Impl*)p;need(uc_particle_registry_view(s.registry,token)==v&&!uc_particle_registry_remove(s.registry,token),"Thrown particle registry lifetime");auto at=std::find_if(s.sources.begin(),s.sources.end(),[&](const auto&x){return x->view==v;});if(at!=s.sources.end())s.sources.erase(at);}
 static int32_t find(void*p,UCEntityEventTarget*t,const char*n){auto&s=*(Source*)p;need(t==&s.entity->identity,"Attached particle bone identity");return s.character?s.character->bone_index(n):s.weapon?s.weapon->bone_index(n):s.prop->bone_index(n);}
 static uint8_t bone(void*p,UCEntityEventTarget*t,int32_t i,float*out){auto&s=*(Source*)p;need(t==&s.entity->identity,"Attached particle pose identity");return s.character?s.character->posed_bone_local(i,out):s.weapon?s.weapon->posed_bone(i,out):s.prop->posed_bone(i,out);}
 static const float*transform(void*p,UCEntityEventTarget*t){auto&s=*(Source*)p;return t==&s.entity->identity?s.entity->transform:nullptr;}
 static const uint32_t*flags(void*p,UCEntityEventTarget*t){auto&s=*(Source*)p;return t==&s.entity->identity?&s.entity->flags:nullptr;}
 static int dead(void*p,uint8_t*out){auto&s=*(Source*)p;return uc_particle_attachment_dead(&s.services,&s.attachment,out);}
 static int sample(void*p,const UCParticleBounds*,float*out){auto&s=*(Source*)p;int result=uc_particle_attachment_matrix(&s.services,&s.attachment,out);if(!result&&s.cold)for(unsigned i=0;i<4;i++)std::swap(out[i],out[8+i]);return result;}
 static int fixed_dead(void*,uint8_t*out){*out=0;return 0;}
 static int fixed_sample(void*p,const UCParticleBounds*,float*out){memcpy(out,((Source*)p)->fixed,64);return 0;}
 static int ring_sample(void*p,const UCParticleBounds*,float*out){auto&s=*(Source*)p;auto*t=uc_entity_event_lookup(&s.world->registry(),s.attachment.entity);
  if(t){need(t==&s.entity->identity,"Warp ring source identity");memcpy(out,s.entity->transform,64);}
  else {std::fill_n(out,16,0.f);for(unsigned i=0;i<4;i++)out[i*5]=1;}return 0;}
 static const UCNpcThrownWeapon*resolve(void*p,int32_t id){auto&s=*(Source*)p;auto*t=uc_entity_event_lookup(&s.world->registry(),id);if(!t)return nullptr;need(t==&s.weapon->entity.identity,"Thrown particle generation identity");return &s.weapon->state;}
 static int emit(void*p,UCParticleTrigger*t,float delta,uint32_t*out){auto&s=*(Source*)p;UCNpcThrownTriggerServices h{p,resolve};return uc_npc_thrown_trigger_emit(t,s.attachment.entity,delta,&h,out);}
};
NativeNpcThrownParticles::NativeNpcThrownParticles(NativeRenderer&r,const UCArchive*archive,NativeMissionWorld&w,const std::filesystem::path&shaders):p_(new Impl){
 auto&s=*p_;s.world=&w;need(archive,"Thrown particle archive");const char*names[]={"MolotovTrail1.simpleparticlesystem","MolotovTrail2.simpleparticlesystem","MolotovTrail3.simpleparticlesystem","redgasfire.simpleparticlesystem","Shield-bullethitsparkbase.simpleparticlesystem","Shield-bullethitspark1.simpleparticlesystem","Shield-bullethitspark2.simpleparticlesystem","Shield-bullethitspark3.simpleparticlesystem","coldbreath.simpleparticlesystem"};s.particles.reserve(10);std::vector<const char*>texture_names;
 for(auto*n:names){s.particles.push_back({n});auto&p=s.particles.back();BytesOwner b;size_t i=find_resource(archive,n);need(i!=SIZE_MAX&&uc_archive_read(archive,i,UC_AR_LOCAL_PAYLOAD,&b.value)==UC_AR_OK&&!uc_particle_asset_load(b.value.data,b.value.size,&p.asset),"Thrown particle asset");need((p.asset.particle_type<=3||p.asset.particle_type==5)&&p.asset.render_type<=1&&p.asset.blend<7,"Thrown particle family");texture_names.push_back(p.asset.texture);}
 s.particles.push_back({"WorldWarpRing"});UCExplosionVisualChild ring{};ring.start_size=100;ring.end_size=5000;ring.lifetime=ring.strength=1;
 need(!uc_world_warp_asset(&ring,&s.particles.back().asset),"Warp ring descriptor");texture_names.push_back(s.particles.back().asset.texture);
 need(!uc_texture_resources_create(archive,1,texture_names.data(),texture_names.size(),&s.textures),"Thrown particle textures");for(size_t i=0;i<s.particles.size();i++){auto*t=uc_texture_resources_at(s.textures,i);need(t&&t->image&&!t->sequence,"Thrown particle resident image");s.particles[i].image=r.upload(*t->image);}
 need(!uc_particle_registry_create(capacity,&s.registry),"Molotov registry allocation");UCParticleLifecycleHooks hooks{Impl::add,Impl::remove};need(!uc_particle_lifecycle_create(capacity,&hooks,&s,&s.lifetime),"Molotov lifetime allocation");
 uint32_t seed=1;need(!uc_particle_sprite_random(&seed,s.table),"Molotov startup table");need(!uc_particle_quad_axes(&seed,s.axis_sine,s.axis_cosine),"Molotov quad axes");
 s.shaders=shaders;s.quads.open(r.device.Get(),r.context.Get(),source(shaders/"particle_vertex.hlsl"),source(shaders/"particle.hlsl"));
 s.sprites.open(r.device.Get(),r.context.Get(),source(shaders/"particle_billboard.hlsl"),source(shaders/"particle.hlsl"));
 D3D11_SAMPLER_DESC sd{};sd.Filter=D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;sd.AddressU=sd.AddressV=sd.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;sd.MaxLOD=D3D11_FLOAT32_MAX;need(SUCCEEDED(r.device->CreateSamplerState(&sd,&s.sampler)),"Molotov sampler");
 D3D11_DEPTH_STENCIL_DESC dd{};dd.DepthEnable=TRUE;dd.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ZERO;dd.DepthFunc=D3D11_COMPARISON_LESS_EQUAL;need(SUCCEEDED(r.device->CreateDepthStencilState(&dd,&s.depth)),"Molotov depth");
 D3D11_RASTERIZER_DESC rd{};rd.FillMode=D3D11_FILL_SOLID;rd.CullMode=D3D11_CULL_NONE;rd.DepthClipEnable=TRUE;need(SUCCEEDED(r.device->CreateRasterizerState(&rd,&s.raster)),"Molotov raster");
 const D3D11_BLEND factors[]={D3D11_BLEND_ZERO,D3D11_BLEND_ONE,D3D11_BLEND_SRC_COLOR,D3D11_BLEND_INV_SRC_COLOR,D3D11_BLEND_SRC_ALPHA,D3D11_BLEND_INV_SRC_ALPHA};
 for(unsigned i=0;i<7;i++){UCParticleRenderState state{};need(uc_particle_render_begin(&state,i)!=0,"Molotov blend policy");D3D11_BLEND_DESC d{};auto&b=d.RenderTarget[0];b.BlendEnable=state.blend_enabled;b.SrcBlend=factors[state.source_factor];b.DestBlend=factors[state.destination_factor];b.BlendOp=state.operation?D3D11_BLEND_OP_REV_SUBTRACT:D3D11_BLEND_OP_ADD;b.SrcBlendAlpha=D3D11_BLEND_ZERO;b.DestBlendAlpha=D3D11_BLEND_ONE;b.BlendOpAlpha=D3D11_BLEND_OP_ADD;b.RenderTargetWriteMask=7;need(SUCCEEDED(r.device->CreateBlendState(&d,&s.blends[i])),"Molotov blend state");}
}
NativeNpcThrownParticles::~NativeNpcThrownParticles()=default;
int NativeNpcThrownParticles::append(const char*asset,NativeNpcThrownWeapon&weapon,bool custom){auto&s=*p_;if(!asset||!custom)return 1;auto at=std::find_if(s.particles.begin(),s.particles.end(),[&](const auto&p){return !strcmp(p.name,asset);});if(at==s.particles.end())return 1;
 auto source=std::make_unique<Impl::Source>();source->weapon=&weapon;source->entity=&weapon.entity;source->world=s.world;source->services={source.get(),&s.world->registry(),Impl::find,Impl::bone,Impl::transform,Impl::flags};if(uc_particle_attachment_bind(&source->services,weapon.entity.id,"FirePos",1,&source->attachment))return 1;
 UCParticleSource h{source.get(),Impl::dead,Impl::sample};UCParticleEmission emission{source.get(),Impl::emit};s.registering=source.get();int result=uc_particle_lifecycle_append_emission(s.lifetime,&at->asset,&h,&emission);s.registering=nullptr;if(result)return result;s.sources.push_back(std::move(source));return 0;
}
int NativeNpcThrownParticles::append(const char*asset,NativeRigidProp&prop,const char*bone,uint8_t hidden){auto&s=*p_;if(!asset||!bone||strcmp(asset,"redgasfire.simpleparticlesystem"))return 1;
 auto at=std::find_if(s.particles.begin(),s.particles.end(),[&](const auto&p){return !strcmp(p.name,asset);});if(at==s.particles.end())return 1;
 auto source=std::make_unique<Impl::Source>();source->prop=&prop;source->entity=&prop.entity;source->world=s.world;source->services={source.get(),&s.world->registry(),Impl::find,Impl::bone,Impl::transform,Impl::flags};
 if(uc_particle_attachment_bind(&source->services,prop.entity.id,bone,hidden,&source->attachment))return 1;
 UCParticleSource h{source.get(),Impl::dead,Impl::sample};s.registering=source.get();int result=uc_particle_lifecycle_append_source(s.lifetime,&at->asset,&h);s.registering=nullptr;if(result)return result;s.sources.push_back(std::move(source));return 0;
}
void NativeNpcThrownParticles::step(float delta,UCGameRandom&rng){const float wind[3]={0,0,0};need(!uc_particle_lifecycle_step(p_->lifetime,delta,wind,&rng),"Thrown particle update");}
int NativeNpcThrownParticles::append_cold(NativeMissionCharacter&character){auto&s=*p_;
 auto at=std::find_if(s.particles.begin(),s.particles.end(),[](const auto&p){return !strcmp(p.name,"coldbreath.simpleparticlesystem");});if(at==s.particles.end())return 1;
 auto source=std::make_unique<Impl::Source>();source->character=&character;source->entity=&character.entity;source->world=s.world;source->cold=true;
 source->services={source.get(),&s.world->registry(),Impl::find,Impl::bone,Impl::transform,Impl::flags};if(uc_particle_attachment_bind(&source->services,character.entity.id,"Bip01 Head",0,&source->attachment))return 1;
 UCParticleSource h{source.get(),Impl::dead,Impl::sample};s.registering=source.get();int result=uc_particle_lifecycle_append_source(s.lifetime,&at->asset,&h);s.registering=nullptr;if(result)return result;s.sources.push_back(std::move(source));return 0;
}
int NativeNpcThrownParticles::append_ring(NativeMissionEntity&entity,const UCParticleAsset&asset){auto&s=*p_;
 if(asset.particle_type||asset.render_type!=1||strcmp(asset.texture,"Particles\\Shapes\\Ring.tga"))return 1;
 auto source=std::make_unique<Impl::Source>();source->entity=&entity;source->world=s.world;source->ring=true;source->ring_asset=asset;source->attachment={entity.id,-1,0};
 source->services={source.get(),&s.world->registry(),Impl::find,Impl::bone,Impl::transform,Impl::flags};UCParticleSource h{source.get(),Impl::dead,Impl::ring_sample};
 s.registering=source.get();int result=uc_particle_lifecycle_append_source(s.lifetime,&source->ring_asset,&h);s.registering=nullptr;if(result)return result;s.sources.push_back(std::move(source));return 0;
}
int NativeNpcThrownParticles::append_shield(const char*asset,const float*matrix){auto&s=*p_;
 if(!asset||!matrix)return 1;auto at=std::find_if(s.particles.begin()+4,s.particles.begin()+8,[&](const auto&p){return !strcmp(p.name,asset);});if(at==s.particles.begin()+8)return 1;
 for(unsigned i=0;i<16;i++)if(!std::isfinite(matrix[i]))return 1;
 auto source=std::make_unique<Impl::Source>();memcpy(source->fixed,matrix,64);UCParticleSource h{source.get(),Impl::fixed_dead,Impl::fixed_sample};
 s.registering=source.get();int result=uc_particle_lifecycle_append_source(s.lifetime,&at->asset,&h);s.registering=nullptr;if(result)return result;s.sources.push_back(std::move(source));return 0;
}
uint32_t NativeNpcThrownParticles::systems()const{return uc_particle_lifecycle_count(p_->lifetime);}
uint32_t NativeNpcThrownParticles::particles()const{uint32_t n=0;for(uint32_t i=0;i<systems();i++)n+=uc_particle_lifecycle_at(p_->lifetime,i)->system->count;return n;}
void NativeNpcThrownParticles::draw(NativeRenderer&r,const float view[16],const float projection[16]){
 auto&s=*p_;
 float camera[16],clip[16];store(camera,XMMatrixInverse(nullptr,matrix(view)));store(clip,matrix(view)*matrix(projection));
 ComPtr<ID3D11BlendState> old_blend;float blend_factor[4];UINT mask;ComPtr<ID3D11DepthStencilState> old_depth;UINT reference;ComPtr<ID3D11RasterizerState> old_raster;
 r.context->OMGetBlendState(&old_blend,blend_factor,&mask);r.context->OMGetDepthStencilState(&old_depth,&reference);r.context->RSGetState(&old_raster);
 r.context->OMSetDepthStencilState(s.depth.Get(),0);r.context->RSSetState(s.raster.Get());
    NativeParticleQuadView qv{};NativeParticleBillboardView bv{};for(unsigned i=0;i<4;i++)for(unsigned j=0;j<4;j++)qv.clip_rows[i][j]=bv.clip_rows[i][j]=clip[j*4+i];memcpy(bv.right,camera,16);memcpy(bv.up,camera+4,16);qv.fog[3]=1;
    bool heat_started=false;for(auto token=uc_particle_registry_first(s.registry);token;token=uc_particle_registry_next(s.registry,token)){
        auto*v=uc_particle_registry_view(s.registry,token);auto it=std::find_if(s.particles.begin(),s.particles.end(),[&](const Impl::Particle&p){return &p.asset==v->asset;});
        if(it==s.particles.end()){auto source=std::find_if(s.sources.begin(),s.sources.end(),[&](const auto&x){return x->ring&&x->view==v&&&x->ring_asset==v->asset;});if(source!=s.sources.end())it=s.particles.end()-1;}
        need(it!=s.particles.end()&&v->system->count<=geometry_capacity,"Molotov geometry capacity");unsigned count=v->system->count;UCParticleSpriteAdmission admission{};auto*texture=uc_texture_resources_at(s.textures,size_t(it-s.particles.begin()));
        if(v->asset->particle_type==0){UCParticleColorCurve curve{};need(!uc_particle_plain_prepare(v->asset,0,1,0,count,texture,&curve,&admission)&&!uc_particle_plain_draw(v->asset,&curve,v->system->particles,admission.count,camera,0,0,0,s.plain_scratch.data()),"Molotov plain geometry");for(unsigned i=0;i<admission.count;i++){auto&a=s.plain_scratch[i];auto&b=s.sprite_scratch[i];memcpy(b.center,a.center,12);b.radius=a.radius;b.angle=0;memcpy(b.color,a.color,16);}}
        else if(v->asset->particle_type==2){UCParticleSpriteState state{};need(!uc_particle_sprite_prepare(v->asset,0,1,0,count,texture,&state,&admission)&&!uc_particle_rotating_quads(v->asset,&state,v->system->particles,admission.count,camera,s.table,s.axis_sine,s.axis_cosine,0,s.quad_scratch.data()),"Molotov rotating geometry");}
        else if(v->asset->particle_type==5){UCParticleStretchState state{};need(!uc_particle_stretch_prepare(v->asset,0,1,count,texture,&state,&admission)&&!uc_particle_stretched_quads(v->asset,&state,v->system->particles,admission.count,camera,s.table,0,s.quad_scratch.data()),"Molotov stretched geometry");}
        else need(!uc_particle_lifecycle_geometry(v,camera,s.table,texture,0,1,0,0,s.quad_scratch.data(),s.sprite_scratch.data(),geometry_capacity,&admission),"Molotov geometry failed");
        if(!admission.count)continue;UCParticleRenderState state{};need(uc_particle_render_begin(&state,admission.blend)!=0,"Molotov render state");qv.alpha_test=bv.alpha_test=state.alpha_test;qv.texture_alpha_kill=bv.texture_alpha_kill=state.texture_alpha_kill;const bool heat=v->asset->render_type==1;if(heat){if(!heat_started){if(!s.heat||s.heat->width()!=r.width||s.heat->height()!=r.height){s.heat=std::make_unique<NativeHeatHaze>();s.heat->open(r,source(s.shaders/"environment_postprocess.hlsl"));}s.heat->begin(r);heat_started=true;}s.heat->bind_mask(r);}else r.context->OMSetBlendState(s.blends[admission.blend].Get(),nullptr,~0u);
        if(v->asset->particle_type==1||v->asset->particle_type==2||v->asset->particle_type==5){for(unsigned start=0;start<admission.count;start+=128){unsigned n=std::min(128u,admission.count-start);UCParticleQuadVertex out[128*4];need(!uc_particle_quad_batch(s.quad_scratch.data()+start,n,out),"Molotov quad batch");s.quads.draw(out,n,qv,it->image.Get(),s.sampler.Get(),heat);}}
        else for(unsigned start=0;start<admission.count;start+=39){unsigned n=std::min(39u,admission.count-start);UCParticleBillboard out[39];need(!(heat&&v->asset->particle_type==3?uc_particle_billboard_heat_batch(s.sprite_scratch.data()+start,n,out):uc_particle_billboard_batch(s.sprite_scratch.data()+start,n,out)),"Molotov sprite batch");s.sprites.draw(out,n,bv,it->image.Get(),s.sampler.Get(),heat);}
        if(heat){ID3D11RenderTargetView*target=r.target.Get();r.context->OMSetRenderTargets(1,&target,r.depth_view.Get());}
    }
    if(heat_started)s.heat->finish(r,r.material_phase);
    r.context->VSSetShader(r.vertex_shader.Get(),nullptr,0);r.context->PSSetShader(r.pixel_shader.Get(),nullptr,0);r.context->IASetInputLayout(r.layout.Get());r.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);ID3D11Buffer*cb=r.constants.Get();r.context->VSSetConstantBuffers(0,1,&cb);r.context->PSSetConstantBuffers(0,1,&cb);
    r.context->OMSetBlendState(old_blend.Get(),blend_factor,mask);r.context->OMSetDepthStencilState(old_depth.Get(),reference);r.context->RSSetState(old_raster.Get());
}
