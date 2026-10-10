#include "integration_impacts.h"
#include "character_navigation.h"
extern "C" {
#include "../bullet_impact.h"
#include "../effect_quota.h"
#include "../particle_registry.h"
#include "../particle_collection.h"
#include "../particle_stretch.h"
#include "../particle_render_state.h"
#include "../material_resources.h"
#include "../decal_quad.h"
#include "../footprint.h"
#include "../decal_ring.h"
#include "../explosion.h"
#include "../explosion_effects.h"
#include "../molotov_receiver.h"
}
#include "particle_billboards.h"
#include "particle_quads.h"
#include "decal_quads.h"
#include <DirectXMath.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <stdexcept>
using namespace DirectX;
namespace {
void need(bool yes,const char *message){if(!yes)throw std::runtime_error(message);}
std::string source(const std::filesystem::path&p){auto b=read_file(p,1024*1024);return {b.begin(),b.end()};}
XMMATRIX matrix(const float*p){XMFLOAT4X4 m;memcpy(&m,p,64);return XMLoadFloat4x4(&m);}
void store(float*p,FXMMATRIX m){XMFLOAT4X4 out;XMStoreFloat4x4(&out,m);memcpy(p,&out,64);}
constexpr unsigned capacity=256,geometry_capacity=4096;
}
struct NativeIntegrationImpacts::Impl {
    struct Particle {const char*name;UCParticleAsset asset{};ComPtr<ID3D11ShaderResourceView> image;};
    struct Mark {const char*name;const UCMaterialResource*material;ComPtr<ID3D11ShaderResourceView> image;};
    NativeImpactCounts counts;
    std::vector<Particle> particles;std::vector<Mark> materials;
    UCTextureResources *textures=nullptr;UCMaterialResources *resources=nullptr;
    UCParticleRegistry *registry=nullptr;UCParticleLifecycle *lifetime=nullptr;
    UCEffectQuota quota{};UCDecalRing ring{};std::array<UCDecalEntry,257> marks{};
    NativeParticleQuads quads;NativeParticleBillboards sprites;NativeDecalQuads decals;
    ComPtr<ID3D11BlendState> blends[7];ComPtr<ID3D11SamplerState> sampler;
    ComPtr<ID3D11DepthStencilState> depth;ComPtr<ID3D11RasterizerState> raster;
    float table[16]{};
    // Fixed scratch avoids frame allocations and admits every loaded estimate.
    std::array<UCParticleQuad,geometry_capacity> quad_scratch;
    std::array<UCParticleSprite,geometry_capacity> sprite_scratch;
    std::array<UCParticlePlainSprite,geometry_capacity> plain_scratch;
    ~Impl(){if(lifetime)uc_particle_lifecycle_close(lifetime);if(registry)uc_particle_registry_close(registry);uc_texture_resources_close(textures);uc_texture_resources_close(resources);}
    static uintptr_t add(void*p,const UCParticleLifecycleView*v){auto&s=*(Impl*)p;uintptr_t token=0;need(!uc_particle_registry_add(s.registry,v,&token)&&token,"Impact registry full");return token;}
    static void remove(void*p,uintptr_t token,const UCParticleLifecycleView*v){auto&s=*(Impl*)p;need(uc_particle_registry_view(s.registry,token)==v&&!uc_particle_registry_remove(s.registry,token),"Impact registry lifetime mismatch");}
    Particle&particle(const char*n){for(auto&a:particles)if(!strcmp(a.name,n))return a;throw std::runtime_error("Unprepared impact particle");}
    Mark&mark(const char*n){for(auto&m:materials)if(!_stricmp(m.name,n))return m;throw std::runtime_error("Unprepared impact decal");}
    static int ready(void*,const char*,const float[4][3]){return 1;}
    static int material(void*p,const char*n,void**out){*out=&((Impl*)p)->mark(n);return 0;}
    // Explicit neutral PC ambient parent, not an original light-manager claim.
    static int ambient(void*,const float*,uint32_t*out){*out=0xffffffffu;return 0;}
};
NativeIntegrationImpacts::NativeIntegrationImpacts(NativeRenderer&r,const std::filesystem::path&game,
    const std::filesystem::path&catalogs,const std::filesystem::path&shaders):impl(new Impl){
    auto&s=*impl;uint8_t hash[32];auto archive=read_archive(game/"1-01.ent",hash);
    s.particles.reserve(32);s.materials.reserve(32);
    for(unsigned id=0;id<=35;id++)if(auto*m=uc_bullet_impact_recipe(id)){
        for(unsigned k=0;m->particles[k];k++)if(std::none_of(s.particles.begin(),s.particles.end(),[&](const Impl::Particle&p){return !strcmp(p.name,m->particles[k]);}))s.particles.push_back({m->particles[k]});
        if(m->decal&&std::none_of(s.materials.begin(),s.materials.end(),[&](const Impl::Mark&p){return !strcmp(p.name,m->decal);}))s.materials.push_back({m->decal,nullptr,{}});
    }
    for(int type:{0,2})for(unsigned i=0;i<4;i++)s.particles.push_back({uc_explosion_particle_asset(type,i)});
    for(unsigned i=0;i<8;i++)s.particles.push_back({uc_molotov_particle_asset(i)});
    for(auto name:{"bullet_hit\\bloodheadshot.simpleparticlesystem","bullet_hit\\bloodheadshot2.simpleparticlesystem","AM-body-burn.simpleparticlesystem"})s.particles.push_back({name});
    for(auto name:{"FX\\FX_BloodSplats\\FX_BloodSplat_Wall_1.shader","FX\\FX_BloodSplats\\FX_BloodSplat_Wall_2.shader","FX\\FX_BloodSplats\\FX_BloodSplat_Wall_3.shader","FX\\FX_BloodSplats\\FX_BloodSplat_Wall_4.shader"})s.materials.push_back({name,nullptr,{}});
    for(auto name:{"FX\\FX_Footprints\\FX_Footprint_left.shader","FX\\FX_Footprints\\FX_Footprint_right.shader"})s.materials.push_back({name,nullptr,{}});
    s.materials.push_back({"FX\\FX_Molotov_Scorch\\FX_Molotov_Scorch.shader",nullptr,{}});
    std::vector<const char*> texture_names,material_names;
    for(auto&p:s.particles){BytesOwner bytes;auto at=find_resource(archive.get(),p.name);
        need(at!=SIZE_MAX&&uc_archive_read(archive.get(),at,UC_AR_LOCAL_PAYLOAD,&bytes.value)==UC_AR_OK&&!uc_particle_asset_load(bytes.value.data,bytes.value.size,&p.asset),"Impact particle asset missing");
        need((p.asset.particle_type==0||p.asset.particle_type==1||p.asset.particle_type==3||p.asset.particle_type==5)&&p.asset.blend<7&&p.asset.render_type==0,"Impact particle family unsupported");texture_names.push_back(p.asset.texture);
    }
    need(!uc_texture_resources_create(archive.get(),1,texture_names.data(),texture_names.size(),&s.textures),"Impact texture owner failed");
    for(size_t i=0;i<s.particles.size();i++){auto*t=uc_texture_resources_at(s.textures,i);need(t&&t->image&&!t->sequence,"Impact texture must be resident image");s.particles[i].image=r.upload(*t->image);}
    for(auto&m:s.materials)material_names.push_back(m.name);
    auto catalog=read_file(catalogs/"1-01.ent.ucmat",64*1024*1024);
    need(!uc_material_resources_create(archive.get(),1,material_names.data(),material_names.size(),catalog.data(),catalog.size(),&s.resources),"Impact material owner failed");
    for(size_t i=0;i<s.materials.size();i++){auto*m=uc_material_resources_at(s.resources,i);need(m&&m->asset&&m->texture&&m->texture->image&&!m->texture->sequence,"Impact decal resource missing");s.materials[i].material=m;s.materials[i].image=r.upload(*m->texture->image);}
    need(!uc_particle_registry_create(capacity,&s.registry),"Impact registry allocation");UCParticleLifecycleHooks hooks{Impl::add,Impl::remove};
    need(!uc_particle_lifecycle_create(capacity,&hooks,&s,&s.lifetime)&&!uc_effect_quota_init(&s.quota)&&!uc_decal_ring_init(&s.ring,s.marks.data(),unsigned(s.marks.size())),"Impact owners failed");
    uint32_t seed=1;need(!uc_particle_sprite_random(&seed,s.table),"Impact startup table");
    s.quads.open(r.device.Get(),r.context.Get(),source(shaders/"particle_vertex.hlsl"),source(shaders/"particle.hlsl"));
    s.sprites.open(r.device.Get(),r.context.Get(),source(shaders/"particle_billboard.hlsl"),source(shaders/"particle.hlsl"));
    s.decals.open(r.device.Get(),r.context.Get(),source(shaders/"base_alpha_vertex.hlsl"));
    D3D11_SAMPLER_DESC sd{};sd.Filter=D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;sd.AddressU=sd.AddressV=sd.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;sd.MaxLOD=D3D11_FLOAT32_MAX;
    need(SUCCEEDED(r.device->CreateSamplerState(&sd,&s.sampler)),"Impact sampler");
    D3D11_DEPTH_STENCIL_DESC dd{};dd.DepthEnable=TRUE;dd.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ZERO;dd.DepthFunc=D3D11_COMPARISON_LESS_EQUAL;
    need(SUCCEEDED(r.device->CreateDepthStencilState(&dd,&s.depth)),"Impact depth state");
    D3D11_RASTERIZER_DESC rd{};rd.FillMode=D3D11_FILL_SOLID;rd.CullMode=D3D11_CULL_NONE;rd.DepthClipEnable=TRUE;need(SUCCEEDED(r.device->CreateRasterizerState(&rd,&s.raster)),"Impact raster state");
    const D3D11_BLEND factors[]={D3D11_BLEND_ZERO,D3D11_BLEND_ONE,D3D11_BLEND_SRC_COLOR,D3D11_BLEND_INV_SRC_COLOR,D3D11_BLEND_SRC_ALPHA,D3D11_BLEND_INV_SRC_ALPHA};
    for(unsigned i=0;i<7;i++){UCParticleRenderState state{};need(uc_particle_render_begin(&state,i)!=0,"Impact blend policy");D3D11_BLEND_DESC d{};auto&b=d.RenderTarget[0];b.BlendEnable=state.blend_enabled;b.SrcBlend=factors[state.source_factor];b.DestBlend=factors[state.destination_factor];b.BlendOp=state.operation?D3D11_BLEND_OP_REV_SUBTRACT:D3D11_BLEND_OP_ADD;b.SrcBlendAlpha=D3D11_BLEND_ZERO;b.DestBlendAlpha=D3D11_BLEND_ONE;b.BlendOpAlpha=D3D11_BLEND_OP_ADD;b.RenderTargetWriteMask=7;need(SUCCEEDED(r.device->CreateBlendState(&d,&s.blends[i])),"Impact blend state");}
}
NativeIntegrationImpacts::~NativeIntegrationImpacts()=default;
const NativeImpactCounts&NativeIntegrationImpacts::counts()const{return impl->counts;}
int NativeIntegrationImpacts::scorch(const NativeCharacterWorld&world,const float*point,const float*normal,
    const float*size,uint32_t mask,UCVectorRandom&vector){
    if(mask!=0x4000)return 1;
    return surface_decal(world,"FX\\FX_Molotov_Scorch\\FX_Molotov_Scorch.shader",point,normal,size,mask,0,vector);
}
int NativeIntegrationImpacts::surface_decal(const NativeCharacterWorld&world,const char*shader,const float*point,const float*normal,
    const float*size,uint32_t mask,uint32_t flags,UCVectorRandom&vector){
    if(!shader||(mask!=0x4000&&mask!=0x80)||(flags!=0&&flags!=2))return 1;auto&s=*impl;float corners[4][3];
    if(uc_decal_quad(point,normal,size,&vector,corners))return 1;++s.counts.decal_requests;
    for(unsigned i=0;i<4;i++){float from[3],to[3];for(unsigned j=0;j<3;j++){
        const float delta=normal[j]*2;from[j]=delta+corners[i][j];to[j]=corners[i][j]-delta;}
        UCCollisionResult hit{};int32_t entity=0;int result=world.query(from,to,1,65535,mask,0,hit,entity);
        if(result<0)return 2;if(!result){++s.counts.decal_rejected;return 0;}}
    UCDecalRingServices hooks{&s,Impl::ready,Impl::material,Impl::ambient};
    if(uc_decal_ring_insert(&s.ring,shader,corners,flags,0,&hooks))return 2;
    ++s.counts.decal_marks;return 0;
}
int NativeIntegrationImpacts::explosion_scorch(const NativeCharacterWorld&world,const float*point,UCVectorRandom&vector){
    struct Context{NativeIntegrationImpacts*self;const NativeCharacterWorld*world;UCVectorRandom*vector;}context{this,&world,&vector};
    UCExplosionScorchServices h{&context,
        [](void*q,const float*from,const float*to,uint32_t type,uint32_t group,uint32_t mask,float impulse,float*position,float*normal,uint8_t*found){
            auto&c=*static_cast<Context*>(q);UCCollisionResult hit{};int32_t entity=0;
            int result=c.world->query(from,to,type,group,mask,impulse,hit,entity);if(result<0)return 1;*found=(uint8_t)result;
            if(result){std::memcpy(position,hit.point,12);std::memcpy(normal,hit.normal,12);}return 0;},
        [](void*q,const char*shader,const float*position,const float*normal,const float*size,uint32_t mask){
            if(std::strcmp(shader,"FX\\FX_Molotov_Scorch\\FX_Molotov_Scorch.shader"))return 1;auto&c=*static_cast<Context*>(q);
            return c.self->scorch(*c.world,position,normal,size,mask,*c.vector);}};
    return uc_explosion_scorch(point,&h);
}
int NativeIntegrationImpacts::bullet_impact(const NativeCharacterWorld&world,const UCEntityFreshView&view,
    UCBulletState&bullet,const UCBulletImpact&packet,UCGameRandom&random,UCVectorRandom&vector,
    const NativeBulletImpactServices&parent){
    if(!parent.blood_enabled||!parent.session_mode||*parent.session_mode||!parent.camera||!parent.sound)return 1;
    struct Context {Impl*s;const NativeCharacterWorld*world;UCVectorRandom*vector;const NativeBulletImpactServices*parent;} context{impl.get(),&world,&vector,&parent};
    UCBulletImpactServices h{};h.context=&context;h.blood_enabled=parent.blood_enabled;h.session_mode=parent.session_mode;
    h.preflight=[](void*q,const UCEntityFreshView*,const UCBulletState*,const UCBulletImpact*){auto&s=*static_cast<Context*>(q)->s;
        // Capacity matters only if source quota/blood gates actually emit.
        // Append reports committed failure at that real source frontier.
        return int(s.lifetime&&s.registry&&s.resources&&s.textures);};
    h.quota_test=[](void*q,const float*point,uint8_t*blocked){return uc_effect_quota_test(&static_cast<Context*>(q)->s->quota,point,blocked);};
    h.camera=[](void*q,float*frame,uint8_t*present){auto&c=*static_cast<Context*>(q);return c.parent->camera(c.parent->owner,frame,present);};
    h.quota_add=[](void*q,const float*point,float radius,float duration){uint8_t added;return uc_effect_quota_add(&static_cast<Context*>(q)->s->quota,point,radius,duration,&added);};
    h.particle=[](void*q,const char*name,const float*frame){auto&s=*static_cast<Context*>(q)->s;auto&p=s.particle(name);
        if(uc_particle_lifecycle_append(s.lifetime,&p.asset,frame))return 1;++s.counts.particle_requests;return 0;};
    h.decal=[](void*q,const char*name,const float*point,const float*normal,float size){auto&c=*static_cast<Context*>(q);auto&s=*c.s;
        ++s.counts.decal_requests;float corners[4][3],sizes[2]={size,size};if(uc_decal_quad(point,normal,sizes,c.vector,corners))return 1;
        for(unsigned i=0;i<4;i++){float from[3],to[3];for(unsigned j=0;j<3;j++){float delta=normal[j]*2;from[j]=delta+corners[i][j];to[j]=corners[i][j]-delta;}
            UCCollisionResult hit{};int32_t entity=0;int r=c.world->query(from,to,1,65535,0x4000,0,hit,entity);if(r<0)return 1;if(!r){++s.counts.decal_rejected;return 0;}}
        UCDecalRingServices hooks{&s,Impl::ready,Impl::material,Impl::ambient};if(uc_decal_ring_insert(&s.ring,name,corners,0,0,&hooks))return 1;
        ++s.counts.decal_marks;return 0;};
    h.sound=[](void*q,const char*bank,const char*event,uint32_t material,const float*point){auto&c=*static_cast<Context*>(q);
        return c.parent->sound(c.parent->owner,bank,event,material,point);};
    const bool alive=!bullet.dead1F5;int result=uc_bullet_static_impact(&h,&view,&bullet,&packet,&random);
    if(!result&&alive)++impl->counts.hits;return result;
}
void NativeIntegrationImpacts::footprint(const NativeCharacterWorld&world,const UCPlayerFootprint&request){
    need(request.shader&&request.mask==0x80&&!request.flags,"Unsupported NPC footprint policy");
    auto&s=*impl;float corners[4][3];need(!uc_footprint_quad(&request,corners),"NPC footprint geometry");++s.counts.decal_requests;
    for(unsigned i=0;i<4;i++){float start[3],end[3];for(unsigned j=0;j<3;j++){
        const float delta=request.normal[j]*2;start[j]=delta+corners[i][j];end[j]=corners[i][j]-delta;}
        UCCollisionResult hit{};int32_t entity=0;const int result=world.query(start,end,1,65535,request.mask,0,hit,entity);
        need(result>=0,"NPC footprint corner query");if(!result){++s.counts.decal_rejected;return;}
    }
    UCDecalRingServices hooks{&s,Impl::ready,Impl::material,Impl::ambient};
    need(!uc_decal_ring_insert(&s.ring,request.shader,corners,0,0,&hooks),"NPC footprint publication");++s.counts.decal_marks;
}
void NativeIntegrationImpacts::preview_explosion(const UCLevelEntity&e){
    UCExplosionParameters p{};need(!uc_explosion_parameters(&e,&p),"Unsupported authored Explosion");
    need(p.damage_type>=0&&p.damage_type<=1&&p.particle_type==0&&p.show_particles==1,"Unsupported explosion particle recipe");
    explosion_particles(e.transform+12);
}
void NativeIntegrationImpacts::explosion_particles(const float position[3]){
    need(position!=nullptr,"Explosion position missing");
    for(unsigned i=0;i<3;i++)need(std::isfinite(position[i])&&std::fabs(position[i])<=65536,"Explosion position invalid");
    auto&s=*impl;need(uc_particle_lifecycle_count(s.lifetime)<=capacity-4,"Explosion particle capacity");
    // E990E..E9995: fixed X/Y-swapped basis, live translation, no Scale field.
    float frame[16]={0,1,0,0,1,0,0,0,0,0,1,0,position[0],position[1],position[2],1};
    for(unsigned i=0;i<4;i++)need(!fixed_particle(uc_explosion_particle_asset(0,i),frame),"Explosion particle append failed");
    ++s.counts.explosion_visuals;
}
int NativeIntegrationImpacts::fixed_particle(const char*name,const float frame[16]){
    if(!name||!frame)return 1;
    for(unsigned i=0;i<16;i++)if(!std::isfinite(frame[i])||std::fabs(frame[i])>65536)return 1;
    auto&s=*impl;auto p=std::find_if(s.particles.begin(),s.particles.end(),[name](const Impl::Particle&p){return !strcmp(name,p.name);});
    if(p==s.particles.end())return 1;
    if(uc_particle_lifecycle_append(s.lifetime,&p->asset,frame))return 2;
    ++s.counts.particle_requests;return 0;
}
void NativeIntegrationImpacts::hit(const UCCollisionAsset&world,const UCCollisionRayHit&hit,uint32_t item,UCGameRandom&rng,UCVectorRandom&vector_rng){
    auto&s=*impl;need((item==2||item==17)&&hit.triangle<world.triangle_count,"Impact hit invalid");
    const unsigned axes[]={0,2,1};float point[3],normal[3],frame[16];for(unsigned i=0;i<3;i++){point[i]=hit.position[axes[i]]*100;normal[i]=hit.normal[axes[i]];}
    need(!uc_bullet_impact_frame(normal,point,frame),"Impact frame invalid");
    ++s.counts.hits;float size=10;if(item==17){int32_t word;need(!uc_game_random_next(&rng,&word),"Impact RNG invalid");size=(float)((double)word*0x1p-31+5.);}
    // Original result publication strips the six-bit primary/secondary masks.
    auto*m=uc_bullet_impact_recipe((uint16_t)(world.triangles[hit.triangle].material>>12));if(!m)return;
    uint8_t blocked=0;if(m->radius){need(!uc_effect_quota_test(&s.quota,point,&blocked),"Impact quota query");if(!blocked){uint8_t added;need(!uc_effect_quota_add(&s.quota,point,m->radius,m->steam?4.f:.6f,&added),"Impact quota add");}}
    if(!blocked)for(unsigned i=0;m->particles[i];i++){auto&p=s.particle(m->particles[i]);need(!uc_particle_lifecycle_append(s.lifetime,&p.asset,frame),"Impact particle append failed");++s.counts.particle_requests;}
    if(m->decal){++s.counts.decal_requests;int32_t word;need(!uc_game_random_next(&rng,&word),"Impact decal selection RNG");float corners[4][3],sizes[]={size,size};need(!uc_decal_quad(point,normal,sizes,&vector_rng,corners),"Impact decal geometry");
        // Actual four-corner geometric admission, PC closest-static-ray service.
        for(unsigned j=0;j<4;j++){float a[3],b[3];for(unsigned k=0;k<3;k++){a[axes[k]]=(corners[j][k]+normal[k]*2)*.01f;b[axes[k]]=(corners[j][k]-normal[k]*2)*.01f;}UCCollisionRayHit q{};int result=uc_collision_raycast(&world,a,b,&q);need(result>=0,"Impact corner ray invalid");if(!result){++s.counts.decal_rejected;return;}}
        UCDecalRingServices hooks{&s,Impl::ready,Impl::material,Impl::ambient};need(!uc_decal_ring_insert(&s.ring,m->decal,corners,0,0,&hooks),"Impact decal publication");++s.counts.decal_marks;
    }
}
void NativeIntegrationImpacts::step(float delta,UCGameRandom&rng){auto&s=*impl;need(delta>0&&delta<=1,"Impact delta invalid");const float wind[3]{};need(!uc_particle_lifecycle_step(s.lifetime,delta,wind,&rng)&&!uc_effect_quota_tick(&s.quota,delta)&&!uc_decal_ring_tick(&s.ring,delta),"Impact lifetime step failed");s.counts.live_systems=uc_particle_lifecycle_count(s.lifetime);s.counts.live_particles=0;for(uint32_t i=0;i<s.counts.live_systems;i++)s.counts.live_particles+=uc_particle_lifecycle_at(s.lifetime,i)->system->count;s.counts.marks=(s.ring.next+s.ring.capacity-s.ring.first)%s.ring.capacity;}
void NativeIntegrationImpacts::draw(NativeRenderer&r,const float view[16],const float projection[16],float inverse_exposure){
    auto&s=*impl;need(std::isfinite(inverse_exposure)&&inverse_exposure>0,"Impact exposure invalid");s.counts.particle_draws=s.counts.decal_draws=0;
    float camera[16],clip[16];store(camera,XMMatrixInverse(nullptr,matrix(view)));store(clip,matrix(view)*matrix(projection));
    ComPtr<ID3D11BlendState> old_blend;float blend_factor[4];UINT mask;ComPtr<ID3D11DepthStencilState> old_depth;UINT reference;ComPtr<ID3D11RasterizerState> old_raster;
    r.context->OMGetBlendState(&old_blend,blend_factor,&mask);r.context->OMGetDepthStencilState(&old_depth,&reference);r.context->RSGetState(&old_raster);
    r.context->OMSetDepthStencilState(s.depth.Get(),0);r.context->RSSetState(s.raster.Get());r.context->OMSetBlendState(s.blends[2].Get(),nullptr,~0u);
    NativeDecalView dv{};for(unsigned i=0;i<4;i++){dv.tint[i]=1;dv.exposure[i]=i==3?1.f:inverse_exposure;for(unsigned j=0;j<4;j++)dv.clip_rows[i][j]=clip[j*4+i];}
    const float uv[4][2]={{0,0},{1,0},{1,1},{0,1}};
    for(uint32_t i=s.ring.first;i!=s.ring.next;i=(i+1)%s.ring.capacity){auto&e=s.marks[i];auto*m=(Impl::Mark*)e.material;NativeDecalVertex vertices[4]{};for(unsigned j=0;j<4;j++){memcpy(vertices[j].position,e.corners[j],12);memcpy(vertices[j].color,e.color,12);vertices[j].color[3]=1;memcpy(vertices[j].uv,uv[j],8);}s.decals.draw(vertices,1,dv,m->image.Get(),s.sampler.Get());++s.counts.decal_draws;}
    NativeParticleQuadView qv{};NativeParticleBillboardView bv{};for(unsigned i=0;i<4;i++)for(unsigned j=0;j<4;j++)qv.clip_rows[i][j]=bv.clip_rows[i][j]=clip[j*4+i];memcpy(bv.right,camera,16);memcpy(bv.up,camera+4,16);qv.fog[3]=1;
    struct Ordered {uintptr_t token;float key;};std::array<Ordered,capacity> ordered{};unsigned used=0;
    for(auto token=uc_particle_registry_first(s.registry);token;token=uc_particle_registry_next(s.registry,token)){
        const auto*v=uc_particle_registry_view(s.registry,token);float key;
        if(!uc_particle_depth_key(v,camera,&key)){
            // PC zero-distance extension: avoid original RSQRT(0) NaN.
            for(unsigned i=12;i<16;i++)need(v->source_matrix[i]==camera[i],"Impact particle distance invalid");
            key=(float)((double)v->asset->depth_bias+900.);
        }
        need(used<capacity,"Impact draw capacity");unsigned at=used++;
        // ponytail: bounded256 insertion sort; source <=32 is stable too.
        // Larger sets use this PC stable policy, not an Xbox quicksort claim.
        while(at&&ordered[at-1].key>key){ordered[at]=ordered[at-1];--at;}ordered[at]={token,key};
    }
    for(unsigned order=0;order<used;order++){const auto token=ordered[order].token;
        auto*v=uc_particle_registry_view(s.registry,token);auto it=std::find_if(s.particles.begin(),s.particles.end(),[&](const Impl::Particle&p){return &p.asset==v->asset;});need(it!=s.particles.end()&&v->system->count<=geometry_capacity,"Impact geometry capacity");unsigned count=v->system->count;UCParticleSpriteAdmission admission{};auto*texture=uc_texture_resources_at(s.textures,size_t(it-s.particles.begin()));
        if(v->asset->particle_type==0){UCParticleColorCurve curve{};need(!uc_particle_plain_prepare(v->asset,0,1,0,count,texture,&curve,&admission)&&!uc_particle_plain_draw(v->asset,&curve,v->system->particles,admission.count,camera,0,0,0,s.plain_scratch.data()),"Impact plain geometry");for(unsigned i=0;i<admission.count;i++){auto&a=s.plain_scratch[i];auto&b=s.sprite_scratch[i];memcpy(b.center,a.center,12);b.radius=a.radius;b.angle=0;memcpy(b.color,a.color,16);}}
        else if(v->asset->particle_type==5){UCParticleStretchState state{};need(!uc_particle_stretch_prepare(v->asset,0,1,count,texture,&state,&admission)&&!uc_particle_stretched_quads(v->asset,&state,v->system->particles,admission.count,camera,s.table,0,s.quad_scratch.data()),"Impact stretched geometry");}
        else need(!uc_particle_lifecycle_geometry(v,camera,s.table,texture,0,1,0,0,s.quad_scratch.data(),s.sprite_scratch.data(),geometry_capacity,&admission),"Impact geometry failed");
        if(!admission.count)continue;UCParticleRenderState state{};need(uc_particle_render_begin(&state,admission.blend)!=0,"Impact render state");qv.alpha_test=bv.alpha_test=state.alpha_test;qv.texture_alpha_kill=bv.texture_alpha_kill=state.texture_alpha_kill;r.context->OMSetBlendState(s.blends[admission.blend].Get(),nullptr,~0u);
        if(v->asset->particle_type==1||v->asset->particle_type==5){for(unsigned start=0;start<admission.count;start+=128){unsigned n=std::min(128u,admission.count-start);UCParticleQuadVertex out[128*4];need(!uc_particle_quad_batch(s.quad_scratch.data()+start,n,out),"Impact quad batch");s.quads.draw(out,n,qv,it->image.Get(),s.sampler.Get());}}
        else for(unsigned start=0;start<admission.count;start+=39){unsigned n=std::min(39u,admission.count-start);UCParticleBillboard out[39];need(!uc_particle_billboard_batch(s.sprite_scratch.data()+start,n,out),"Impact sprite batch");s.sprites.draw(out,n,bv,it->image.Get(),s.sampler.Get());}++s.counts.particle_draws;
    }
    r.context->VSSetShader(r.vertex_shader.Get(),nullptr,0);r.context->PSSetShader(r.pixel_shader.Get(),nullptr,0);r.context->IASetInputLayout(r.layout.Get());r.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);ID3D11Buffer*cb=r.constants.Get();r.context->VSSetConstantBuffers(0,1,&cb);r.context->PSSetConstantBuffers(0,1,&cb);
    r.context->OMSetBlendState(old_blend.Get(),blend_factor,mask);r.context->OMSetDepthStencilState(old_depth.Get(),reference);r.context->RSSetState(old_raster.Get());
}
