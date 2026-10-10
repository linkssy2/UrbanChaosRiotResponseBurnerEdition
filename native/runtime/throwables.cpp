#include "throwables.h"
#include "model.h"
#include "mission_world.h"
extern "C" {
#include "../bullet_impact.h"
#include "../particle_registry.h"
#include "../particle_stretch.h"
#include "../particle_render_state.h"
#include "../texture_resources.h"
}
#include "particle_billboards.h"
#include "particle_quads.h"
#include <DirectXMath.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <stdexcept>
using namespace DirectX;
namespace {
void need(bool yes,const char*text){if(!yes)throw std::runtime_error(text);}
std::string source(const std::filesystem::path&p){auto b=read_file(p,1024*1024);return {b.begin(),b.end()};}
XMMATRIX matrix(const float*p){XMFLOAT4X4 m;memcpy(&m,p,64);return XMLoadFloat4x4(&m);}
void store(float*p,FXMMATRIX m){XMFLOAT4X4 out;XMStoreFloat4x4(&out,m);memcpy(p,&out,64);}
constexpr unsigned capacity=256,geometry_capacity=4096;
}
struct NativeIntegrationThrowables::Impl {
 struct Particle {const char*name;UCParticleAsset asset{};ComPtr<ID3D11ShaderResourceView> image;};
 struct Projectile {
  bool live=false;float age=0,expires=100,no_collision=0,previous[3]{},transform[16]{},velocity[3]{};
  std::unique_ptr<NativeMissionEntity> entity;NativeMissionWorld *registry=nullptr;
  Impl*owner=nullptr;
  int late(){if(!owner||!registry)return 1;if(live)owner->advance(*this,*registry->script_services().delta);return 0;}
  int close(){if(!entity)return 1;if(entity->aux&&uc_entity_fresh_aux_destroy(entity->aux))return 1;if(live){live=false;--owner->counts.live_projectiles;}entity->aux=nullptr;entity->model=nullptr;entity.reset();registry=nullptr;return 0;}
  int remove(){if(!entity||!registry)return 1;auto&e=*entity;if(uc_entity_fresh_aux_clear(e.aux))return 1;e.flags|=0x8100;uint8_t queued=0;
   if(uc_entity_retire(&registry->retirement(),e.id,&queued)||!queued)return 2;e.flags|=0x80000;return 0;}
 };
 NativeThrowableCounts counts;const UCCollisionAsset *world=nullptr;
 std::array<Projectile,32> projectiles;NativeAnimatedModel bottle;
 std::vector<Particle> particles;UCTextureResources*textures=nullptr;
 UCParticleRegistry*registry=nullptr;UCParticleLifecycle*lifetime=nullptr;
 NativeParticleQuads quads;NativeParticleBillboards sprites;
 ComPtr<ID3D11BlendState> blends[7];ComPtr<ID3D11SamplerState> sampler;
 ComPtr<ID3D11DepthStencilState> depth;ComPtr<ID3D11RasterizerState> raster;
 float table[16]{};
 std::array<UCParticleQuad,geometry_capacity> quad_scratch;
 std::array<UCParticleSprite,geometry_capacity> sprite_scratch;
 std::array<UCParticlePlainSprite,geometry_capacity> plain_scratch;
 ~Impl(){if(lifetime)uc_particle_lifecycle_close(lifetime);if(registry)uc_particle_registry_close(registry);uc_texture_resources_close(textures);}
 static uintptr_t add(void*p,const UCParticleLifecycleView*v){auto&s=*(Impl*)p;uintptr_t token=0;need(!uc_particle_registry_add(s.registry,v,&token)&&token,"Molotov registry full");return token;}
 static void remove(void*p,uintptr_t token,const UCParticleLifecycleView*v){auto&s=*(Impl*)p;need(uc_particle_registry_view(s.registry,token)==v&&!uc_particle_registry_remove(s.registry,token),"Molotov registry lifetime");}
 void advance(Projectile&p,float delta);
};
NativeIntegrationThrowables::NativeIntegrationThrowables(NativeRenderer&r,const std::filesystem::path&game,
 const std::filesystem::path&catalogs,const std::filesystem::path&shaders,const UCCollisionAsset&world):impl(new Impl){
 auto&s=*impl;s.world=&world;uint8_t hash[32];auto archive=read_archive(game/"1-01.ent",hash);
 s.bottle.load(r,archive.get(),hash,catalogs/"1-01.ent.ucmat","Assets_Characters\\CHA_BurnerBlazer\\CHA_BurnerBlazer_Molotov_Skin.AMF",nullptr);
 for(auto&m:s.bottle.materials)need(m.supported(),"Molotov world model material unsupported");
 float identity[16]{};identity[0]=identity[5]=identity[10]=identity[15]=1;s.bottle.bind_pose(r,identity);
 const char*names[]={"MolotovBurstFlame1.simpleparticlesystem","MolotovBurstFlame2.simpleparticlesystem","MolotovBurstFlame3.simpleparticlesystem","MolotovBurstFlame4.simpleparticlesystem","MolotovBurstFlame5.simpleparticlesystem"};
 s.particles.reserve(5);std::vector<const char*> texture_names;
 for(auto*n:names){s.particles.push_back({n});auto&p=s.particles.back();BytesOwner bytes;auto at=find_resource(archive.get(),n);
  need(at!=SIZE_MAX&&uc_archive_read(archive.get(),at,UC_AR_LOCAL_PAYLOAD,&bytes.value)==UC_AR_OK&&!uc_particle_asset_load(bytes.value.data,bytes.value.size,&p.asset),"Molotov particle missing");
  need((p.asset.particle_type==0||p.asset.particle_type==1||p.asset.particle_type==3||p.asset.particle_type==5)&&p.asset.blend<7,"Molotov particle family");texture_names.push_back(p.asset.texture);
 }
 need(!uc_texture_resources_create(archive.get(),1,texture_names.data(),texture_names.size(),&s.textures),"Molotov texture owner");
 for(size_t i=0;i<s.particles.size();i++){auto*t=uc_texture_resources_at(s.textures,i);need(t&&t->image&&!t->sequence,"Molotov image required");s.particles[i].image=r.upload(*t->image);}
 need(!uc_particle_registry_create(capacity,&s.registry),"Molotov registry allocation");UCParticleLifecycleHooks hooks{Impl::add,Impl::remove};need(!uc_particle_lifecycle_create(capacity,&hooks,&s,&s.lifetime),"Molotov lifetime allocation");
 uint32_t seed=1;need(!uc_particle_sprite_random(&seed,s.table),"Molotov startup table");
 s.quads.open(r.device.Get(),r.context.Get(),source(shaders/"particle_vertex.hlsl"),source(shaders/"particle.hlsl"));
 s.sprites.open(r.device.Get(),r.context.Get(),source(shaders/"particle_billboard.hlsl"),source(shaders/"particle.hlsl"));
 D3D11_SAMPLER_DESC sd{};sd.Filter=D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;sd.AddressU=sd.AddressV=sd.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;sd.MaxLOD=D3D11_FLOAT32_MAX;need(SUCCEEDED(r.device->CreateSamplerState(&sd,&s.sampler)),"Molotov sampler");
 D3D11_DEPTH_STENCIL_DESC dd{};dd.DepthEnable=TRUE;dd.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ZERO;dd.DepthFunc=D3D11_COMPARISON_LESS_EQUAL;need(SUCCEEDED(r.device->CreateDepthStencilState(&dd,&s.depth)),"Molotov depth");
 D3D11_RASTERIZER_DESC rd{};rd.FillMode=D3D11_FILL_SOLID;rd.CullMode=D3D11_CULL_NONE;rd.DepthClipEnable=TRUE;need(SUCCEEDED(r.device->CreateRasterizerState(&rd,&s.raster)),"Molotov raster");
 const D3D11_BLEND factors[]={D3D11_BLEND_ZERO,D3D11_BLEND_ONE,D3D11_BLEND_SRC_COLOR,D3D11_BLEND_INV_SRC_COLOR,D3D11_BLEND_SRC_ALPHA,D3D11_BLEND_INV_SRC_ALPHA};
 for(unsigned i=0;i<7;i++){UCParticleRenderState state{};need(uc_particle_render_begin(&state,i)!=0,"Molotov blend policy");D3D11_BLEND_DESC d{};auto&b=d.RenderTarget[0];b.BlendEnable=state.blend_enabled;b.SrcBlend=factors[state.source_factor];b.DestBlend=factors[state.destination_factor];b.BlendOp=state.operation?D3D11_BLEND_OP_REV_SUBTRACT:D3D11_BLEND_OP_ADD;b.SrcBlendAlpha=D3D11_BLEND_ZERO;b.DestBlendAlpha=D3D11_BLEND_ONE;b.BlendOpAlpha=D3D11_BLEND_OP_ADD;b.RenderTargetWriteMask=7;need(SUCCEEDED(r.device->CreateBlendState(&d,&s.blends[i])),"Molotov blend state");}
}
NativeIntegrationThrowables::~NativeIntegrationThrowables()=default;
const NativeThrowableCounts&NativeIntegrationThrowables::counts()const{return impl->counts;}
bool NativeIntegrationThrowables::throw_molotov(const float eye[16],float pitch,float charge){
 auto&s=*impl;if(!eye||!std::isfinite(pitch)||!std::isfinite(charge))return false;
 for(unsigned i=0;i<16;i++)if(!std::isfinite(eye[i]))return false;
 auto slot=std::find_if(s.projectiles.begin(),s.projectiles.end(),[](const Impl::Projectile&p){return !p.live&&!p.entity;});
 if(slot==s.projectiles.end()||uc_particle_lifecycle_count(s.lifetime)+5*(s.counts.live_projectiles+1)>capacity)return false;
 UCMolotovLaunchInput in{};memcpy(in.origin,eye+12,12);memcpy(in.forward,eye+8,12);in.pitch=pitch;in.charge=charge;
 float from[3],to[3],target[3];for(unsigned i=0;i<3;i++)target[i]=in.origin[i]+in.forward[i]*50000.f;
 uc_collision_to_physics(in.origin,from);uc_collision_to_physics(target,to);UCCollisionRayHit hit{};int result=uc_collision_raycast(s.world,from,to,&hit);need(result>=0,"Molotov aim query");
 if(result)uc_collision_to_render(hit.position,in.target);else memcpy(in.target,target,12);
 double distance=0;for(unsigned i=0;i<3;i++)distance+=(double)(in.target[i]-in.origin[i])*(in.target[i]-in.origin[i]);in.target_distance=(float)sqrt(distance);
 UCMolotovLaunch launch{};if(uc_molotov_launch(&in,&launch))return false;
 slot->live=true;slot->age=0;slot->expires=100;slot->no_collision=0;memcpy(slot->transform,launch.transform,64);memcpy(slot->previous,launch.transform+12,12);memcpy(slot->velocity,launch.velocity,12);
 ++s.counts.throws;++s.counts.live_projectiles;return true;
}
void NativeIntegrationThrowables::Impl::advance(Projectile&p,float delta){
 auto&s=*this;need(std::isfinite(delta)&&delta>0&&delta<=1,"Molotov delta");
  memcpy(p.previous,p.transform+12,12);float next[3];for(unsigned i=0;i<3;i++)next[i]=p.transform[12+i]+p.velocity[i]*delta;next[1]+=(delta*-980.f)*delta*.5f;p.velocity[1]+=-980.f*delta;p.age+=delta;
  float from[3],to[3];uc_collision_to_physics(p.previous,from);uc_collision_to_physics(next,to);UCCollisionRayHit hit{};int result=p.age<p.no_collision?0:uc_collision_raycast(s.world,from,to,&hit);need(result>=0,"Molotov flight query");
  if(result){float point[3],normal[]={hit.normal[0],hit.normal[2],hit.normal[1]},frame[16];uc_collision_to_render(hit.position,point);need(!uc_bullet_impact_frame(normal,point,frame),"Molotov burst frame");
   for(auto&a:s.particles){need(!uc_particle_lifecycle_append(s.lifetime,&a.asset,frame),"Molotov burst append");++s.counts.particle_requests;}++s.counts.hits;p.live=false;--s.counts.live_projectiles;
  }else if(p.age>=p.expires){p.live=false;--s.counts.live_projectiles;++s.counts.expired;}else memcpy(p.transform+12,next,12);
  if(!p.live&&p.entity)p.entity->flags|=1;
}
void NativeIntegrationThrowables::step(float delta,UCGameRandom&rng){
 auto&s=*impl;need(std::isfinite(delta)&&delta>0&&delta<=1,"Molotov delta");
 for(auto&p:s.projectiles)if(p.live&&!p.entity)s.advance(p,delta);
 const float wind[3]{};need(!uc_particle_lifecycle_step(s.lifetime,delta,wind,&rng),"Molotov particle lifetime");s.counts.live_systems=uc_particle_lifecycle_count(s.lifetime);s.counts.live_particles=0;
 for(uint32_t i=0;i<s.counts.live_systems;i++)s.counts.live_particles+=uc_particle_lifecycle_at(s.lifetime,i)->system->count;
}
void NativeIntegrationThrowables::draw(NativeRenderer&r,const float view[16],const float projection[16],float inverse_exposure,float fraction,NativeModelLighting*lighting,const NativeLightingFrame*frame){
 auto&s=*impl;need(std::isfinite(inverse_exposure)&&inverse_exposure>0&&std::isfinite(fraction)&&fraction>=0&&fraction<=1,"Molotov draw input");s.counts.model_draws=s.counts.particle_draws=0;
 float camera[16],clip[16];store(camera,XMMatrixInverse(nullptr,matrix(view)));store(clip,matrix(view)*matrix(projection));
 ComPtr<ID3D11BlendState> old_blend;float blend_factor[4];UINT mask;ComPtr<ID3D11DepthStencilState> old_depth;UINT reference;ComPtr<ID3D11RasterizerState> old_raster;
 r.context->OMGetBlendState(&old_blend,blend_factor,&mask);r.context->OMGetDepthStencilState(&old_depth,&reference);r.context->RSGetState(&old_raster);
 const float white[]={1,1,1,1};NativeTextureContext model_texture{};
 for(auto&p:s.projectiles)if(p.live){float world[16],m[16];memcpy(world,p.transform,64);for(unsigned i=0;i<3;i++)world[12+i]=p.previous[i]+(p.transform[12+i]-p.previous[i])*fraction;store(m,matrix(world)*matrix(clip));
  if(lighting&&frame)s.bottle.draw_lit(r,*lighting,*frame,world,m,white,&model_texture,inverse_exposure);
  else s.bottle.draw(r,m,white,&model_texture,inverse_exposure);++s.counts.model_draws;}
 r.context->OMSetDepthStencilState(s.depth.Get(),0);r.context->RSSetState(s.raster.Get());
    NativeParticleQuadView qv{};NativeParticleBillboardView bv{};for(unsigned i=0;i<4;i++)for(unsigned j=0;j<4;j++)qv.clip_rows[i][j]=bv.clip_rows[i][j]=clip[j*4+i];memcpy(bv.right,camera,16);memcpy(bv.up,camera+4,16);qv.fog[3]=1;
    for(auto token=uc_particle_registry_first(s.registry);token;token=uc_particle_registry_next(s.registry,token)){
        auto*v=uc_particle_registry_view(s.registry,token);auto it=std::find_if(s.particles.begin(),s.particles.end(),[&](const Impl::Particle&p){return &p.asset==v->asset;});need(it!=s.particles.end()&&v->system->count<=geometry_capacity,"Molotov geometry capacity");unsigned count=v->system->count;UCParticleSpriteAdmission admission{};auto*texture=uc_texture_resources_at(s.textures,size_t(it-s.particles.begin()));
        if(v->asset->particle_type==0){UCParticleColorCurve curve{};need(!uc_particle_plain_prepare(v->asset,0,1,0,count,texture,&curve,&admission)&&!uc_particle_plain_draw(v->asset,&curve,v->system->particles,admission.count,camera,0,0,0,s.plain_scratch.data()),"Molotov plain geometry");for(unsigned i=0;i<admission.count;i++){auto&a=s.plain_scratch[i];auto&b=s.sprite_scratch[i];memcpy(b.center,a.center,12);b.radius=a.radius;b.angle=0;memcpy(b.color,a.color,16);}}
        else if(v->asset->particle_type==5){UCParticleStretchState state{};need(!uc_particle_stretch_prepare(v->asset,0,1,count,texture,&state,&admission)&&!uc_particle_stretched_quads(v->asset,&state,v->system->particles,admission.count,camera,s.table,0,s.quad_scratch.data()),"Molotov stretched geometry");}
        else need(!uc_particle_lifecycle_geometry(v,camera,s.table,texture,0,1,0,0,s.quad_scratch.data(),s.sprite_scratch.data(),geometry_capacity,&admission),"Molotov geometry failed");
        if(!admission.count)continue;UCParticleRenderState state{};need(uc_particle_render_begin(&state,admission.blend)!=0,"Molotov render state");qv.alpha_test=bv.alpha_test=state.alpha_test;qv.texture_alpha_kill=bv.texture_alpha_kill=state.texture_alpha_kill;r.context->OMSetBlendState(s.blends[admission.blend].Get(),nullptr,~0u);
        if(v->asset->particle_type==1||v->asset->particle_type==5){for(unsigned start=0;start<admission.count;start+=128){unsigned n=std::min(128u,admission.count-start);UCParticleQuadVertex out[128*4];need(!uc_particle_quad_batch(s.quad_scratch.data()+start,n,out),"Molotov quad batch");s.quads.draw(out,n,qv,it->image.Get(),s.sampler.Get());}}
        else for(unsigned start=0;start<admission.count;start+=39){unsigned n=std::min(39u,admission.count-start);UCParticleBillboard out[39];need(!uc_particle_billboard_batch(s.sprite_scratch.data()+start,n,out),"Molotov sprite batch");s.sprites.draw(out,n,bv,it->image.Get(),s.sampler.Get());}++s.counts.particle_draws;
    }
    r.context->VSSetShader(r.vertex_shader.Get(),nullptr,0);r.context->PSSetShader(r.pixel_shader.Get(),nullptr,0);r.context->IASetInputLayout(r.layout.Get());r.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);ID3D11Buffer*cb=r.constants.Get();r.context->VSSetConstantBuffers(0,1,&cb);r.context->PSSetConstantBuffers(0,1,&cb);
    r.context->OMSetBlendState(old_blend.Get(),blend_factor,mask);r.context->OMSetDepthStencilState(old_depth.Get(),reference);r.context->RSSetState(old_raster.Get());
}

namespace {
struct ScriptedThrower {
 NativeMissionEntity entity;NativeMissionWorld*world=nullptr;void*launch_owner=nullptr;NativeScriptedMolotovCreate launch=nullptr;UCGameRandom*random=nullptr;
 std::vector<uint32_t> targets,stop_flags;float origin[16]{},time=3,no_collision=0,interval=5,elapsed=3600;
 int32_t remaining=1,first=-1;uint32_t target=0;
 const UCEntityFreshView*first_view()const{auto*t=uc_entity_event_lookup(&world->registry(),first);return t?world->context().resolve(world->context().owner,t):nullptr;}
 int remove(){if(entity.model||entity.helper||uc_entity_fresh_aux_clear(entity.aux))return 1;entity.flags|=0x8100;uint8_t queued=0;
  if(uc_entity_retire(&world->retirement(),entity.id,&queued)||!queued)return 2;
  if(entity.flags&0x1000){auto*f=world->slot((uint32_t)entity.source).flags;*f=(*f&~15u)|16u;}entity.flags|=0x80000;return 0;}
 int close(){if(entity.aux&&uc_entity_fresh_aux_destroy(entity.aux))return 1;entity.aux=nullptr;delete this;return 0;}
 int late(){auto*v=first_view();if(!v)return 0;memcpy(entity.transform,v->transform,64);return uc_entity_fresh_aux_clear(entity.aux);}
 int update(){
  if(!first_view()){first=-1;if(!remaining)return remove();}
  elapsed=(float)((double)elapsed+(double)*world->script_services().delta);
  if((remaining>0||remaining==-1)&&elapsed>interval){
   const UCLevelNode*node=nullptr;need(uc_level_node_find(world->script_services().logic,targets[target],&node)==UC_LEVEL_OK,"Scripted projectile target lookup");
   const float*point=node?node->position:nullptr;
   target=(target+1)%(uint32_t)targets.size();int32_t id=-1;
   if(point)id=launch(launch_owner,entity.id,origin,point,time,no_collision);
   if(first==-1)first=id;
   float bound=(interval*.01f)*15.f;int32_t word;if(uc_game_random_next(random,&word))return 2;
   elapsed=(float)-(((double)word*(double)bound)*0x1p-31);if(remaining>0)--remaining;
  }
  bool stop=!stop_flags.empty();const auto&s=world->script_services();for(auto flag:stop_flags)stop=stop&&s.flags[flag]!=0;
  if(stop)return remove();return uc_entity_fresh_aux_clear(entity.aux);
 }
};
}
int native_construct_scripted_thrower(NativeMissionWorld&w,uint32_t i,UCGameRandom&random,void*owner,NativeScriptedMolotovCreate launch){
 if(!launch||i>=w.source_count())return 1;const auto&m=w.metadata(i);const auto*d=m.definition;
 if(!d||strcmp(d->class_name,"ThrowProjectile")||m.asset_present||d->script_count||*w.active_descriptor()!=&m)return 1;
 auto p=std::make_unique<ScriptedThrower>();auto&s=*p;const uint8_t*at=*w.property_cursor(i);int found=0;
 auto read=[&](uint32_t kind,void*out){return !uc_entity_stream_get(d,&at,kind,out,&found)&&found;};
 auto list=[&](std::vector<uint32_t>&out){int32_t count=0;if(!read(1,&count)||count<0||count>64)return false;out.resize((size_t)count);for(auto&n:out)if(!read(1,&n))return false;return true;};
 uint32_t type=0,random_area=0,random_target=0,avoid=0;float radius=0;
 if(!list(s.targets)||s.targets.empty()||!read(2,&s.time)||!read(2,&s.no_collision)||!read(1,&type)||!read(1,&s.remaining)||!read(2,&s.interval)||!list(s.stop_flags)||
    !read(3,&random_area)||!read(3,&random_target)||!read(3,&avoid)||!read(2,&radius)||at!=d->properties.data+d->properties.size)return 1;
 if(type||random_area||random_target||avoid||!std::isfinite(s.time)||s.time<=0||s.time>100||!std::isfinite(s.no_collision)||s.no_collision<0||
    !std::isfinite(s.interval)||s.interval<0||s.remaining<-1)return 1;
 for(auto target:s.targets)if(!(target&0xf0000u))return 1;for(auto f:s.stop_flags)if(f>=w.script_services().flag_count)return 1;
 s.world=&w;s.launch_owner=owner;s.launch=launch;s.random=&random;
 if(w.bind(i,{&s.entity.view,&s,[](void*p){return ((ScriptedThrower*)p)->update();},[](void*p){return ((ScriptedThrower*)p)->late();},
    [](void*p){return ((ScriptedThrower*)p)->close();},nullptr,[](void*p){return ((ScriptedThrower*)p)->remove();},[](void*p){return ((ScriptedThrower*)p)->close();}}))return 1;
 p.release();if(uc_entity_health_construct_authored(&w.context(),&s.entity.view,w.active_descriptor()))return 2;
 *w.property_cursor(i)=at;memcpy(s.origin,s.entity.transform,64);return 0;
}
