#include "integration_weapon_effects.h"
#include "firstperson.h"
#include "gunflare_models.h"
extern "C" {
#include "../muzzle_placement.h"
#include "../particle_registry.h"
#include "../particle_render_state.h"
#include "../texture_resources.h"
}
#include "particle_quads.h"
#include "particle_billboards.h"
#include <DirectXMath.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <stdexcept>
#include <string>
using namespace DirectX;
namespace {
void require(bool b,const char *m){if(!b)throw std::runtime_error(m);}
constexpr float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
const char *const names[]={"GunFlare\\Pistol2.simpleparticlesystem","GunFlare\\Pistol3.simpleparticlesystem",
    "GunFlare\\Pistol1.simpleparticlesystem","GunFlare\\Shotgun3.simpleparticlesystem","GunFlare\\policeshotgunsmoke.simpleparticlesystem"};
XMMATRIX matrix(const float *p){XMFLOAT4X4 m;memcpy(&m,p,64);return XMLoadFloat4x4(&m);}
void store(float *p,FXMMATRIX m){XMFLOAT4X4 out;XMStoreFloat4x4(&out,m);memcpy(p,&out,64);}
std::string source(const std::filesystem::path &p){auto b=read_file(p,1024*1024);return {b.begin(),b.end()};}
}
struct NativeIntegrationWeaponEffects::Impl {
    NativeWeaponEffectsCounts counts;
    NativeGunFlareModels models;
    UCGunFlare flare{};
    UCParticleAsset assets[5]{};
    UCTextureResources *textures=nullptr;
    UCParticleRegistry *registry=nullptr;
    UCParticleLifecycle *lifetime=nullptr;
    NativeParticleQuads quads;
    NativeParticleBillboards sprites;
    ComPtr<ID3D11ShaderResourceView> images[5];
    ComPtr<ID3D11BlendState> blends[7];
    ComPtr<ID3D11SamplerState> sampler;
    ComPtr<ID3D11DepthStencilState> depth;
    float table[16]{},muzzle[16]{};
    uint32_t queued=0;
    const UCModel *bound=nullptr;int32_t bone=-1;
    ~Impl(){if(lifetime)uc_particle_lifecycle_close(lifetime);if(registry)uc_particle_registry_close(registry);uc_texture_resources_close(textures);}
    static uintptr_t registered(void *p,const UCParticleLifecycleView *v){auto&s=*(Impl*)p;uintptr_t token=0;require(!uc_particle_registry_add(s.registry,v,&token)&&token,"Muzzle particle registry full");return token;}
    static void unregistered(void *p,uintptr_t token,const UCParticleLifecycleView *v){auto&s=*(Impl*)p;require(uc_particle_registry_view(s.registry,token)==v&&!uc_particle_registry_remove(s.registry,token),"Muzzle particle lifetime mismatch");}
    static int particles(void *p,const char *name,const float *m,UCGameRandom*){auto&s=*(Impl*)p;
        for(unsigned i=0;i<5;i++)if(!strcmp(name,names[i])){++s.counts.particle_requests;return uc_particle_lifecycle_append(s.lifetime,s.assets+i,m);}return 1;}
    static int tint(void *p,void *model,const float *c){return ((Impl*)p)->models.tint(model,c);}
    static int visible(void *p,void *model,unsigned enabled){return ((Impl*)p)->models.visible(model,enabled);}
    static int transform(void *p,void *model,const float *m){return ((Impl*)p)->models.transform(model,m);}
    static int parent(void*,int32_t,unsigned*){return 1;}
    static int remove(void*){return 1;}
    UCGunFlareServices services(){return {this,particles,tint,visible,transform,parent,remove};}
};
NativeIntegrationWeaponEffects::NativeIntegrationWeaponEffects(NativeRenderer &r,const std::filesystem::path &game,
    const std::filesystem::path &catalogs,const std::filesystem::path &shaders):impl(new Impl){
    auto &s=*impl;uint8_t hash[32];auto archive=read_archive(game/"1-01.ent",hash);
    s.models.load(r,archive.get(),hash,catalogs/"1-01.ent.ucmat",identity);
    const char *textures[5];
    for(unsigned i=0;i<5;i++){
        auto index=find_resource(archive.get(),names[i]);BytesOwner data;
        require(index!=SIZE_MAX&&uc_archive_read(archive.get(),index,UC_AR_LOCAL_PAYLOAD,&data.value)==UC_AR_OK&&
            !uc_particle_asset_load(data.value.data,data.value.size,s.assets+i),"Missing authored muzzle particles");
        require((s.assets[i].particle_type==1||s.assets[i].particle_type==3)&&s.assets[i].blend<7,"Unsupported muzzle particle family");textures[i]=s.assets[i].texture;
    }
    require(uc_texture_resources_create(archive.get(),1,textures,5,&s.textures)==UC_TEXTURE_RESOURCES_OK,"Muzzle textures failed");
    for(unsigned i=0;i<5;i++){
        auto *t=uc_texture_resources_at(s.textures,i);require(t&&t->image&&!t->sequence,"Muzzle image unsupported");
        unsigned other=0;for(;other<i;other++)if(uc_texture_resources_at(s.textures,other)==t)break;
        s.images[i]=other<i?s.images[other]:r.upload(*t->image);
    }
    require(!uc_particle_registry_create(96,&s.registry),"Muzzle registry allocation");
    UCParticleLifecycleHooks hooks{Impl::registered,Impl::unregistered};require(!uc_particle_lifecycle_create(96,&hooks,&s,&s.lifetime),"Muzzle lifetime allocation");
    s.flare.first_person=1;s.flare.parent=-1;s.flare.selected=UINT32_MAX;memcpy(s.flare.world,identity,64);memcpy(s.muzzle,identity,64);
    uint32_t seed=1;require(!uc_particle_sprite_random(&seed,s.table),"Muzzle sprite table");
    s.quads.open(r.device.Get(),r.context.Get(),source(shaders/"particle_vertex.hlsl"),source(shaders/"particle.hlsl"));
    s.sprites.open(r.device.Get(),r.context.Get(),source(shaders/"particle_billboard.hlsl"),source(shaders/"particle.hlsl"));
    D3D11_SAMPLER_DESC sampler{};sampler.Filter=D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;sampler.AddressU=sampler.AddressV=sampler.AddressW=D3D11_TEXTURE_ADDRESS_WRAP;
    require(SUCCEEDED(r.device->CreateSamplerState(&sampler,&s.sampler)),"Muzzle sampler");
    D3D11_DEPTH_STENCIL_DESC depth{};depth.DepthEnable=TRUE;depth.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ZERO;depth.DepthFunc=D3D11_COMPARISON_LESS_EQUAL;
    require(SUCCEEDED(r.device->CreateDepthStencilState(&depth,&s.depth)),"Muzzle depth state");
    const D3D11_BLEND factors[]={D3D11_BLEND_ZERO,D3D11_BLEND_ONE,D3D11_BLEND_SRC_COLOR,D3D11_BLEND_INV_SRC_COLOR,D3D11_BLEND_SRC_ALPHA,D3D11_BLEND_INV_SRC_ALPHA};
    for(unsigned i=0;i<7;i++){
        UCParticleRenderState state{};require(uc_particle_render_begin(&state,i)!=0,"Muzzle blend policy");
        D3D11_BLEND_DESC desc{};auto &b=desc.RenderTarget[0];b.BlendEnable=state.blend_enabled;b.SrcBlend=factors[state.source_factor];b.DestBlend=factors[state.destination_factor];
        b.BlendOp=state.operation?D3D11_BLEND_OP_REV_SUBTRACT:D3D11_BLEND_OP_ADD;b.SrcBlendAlpha=D3D11_BLEND_ZERO;b.DestBlendAlpha=D3D11_BLEND_ONE;b.BlendOpAlpha=D3D11_BLEND_OP_ADD;b.RenderTargetWriteMask=7;
        require(SUCCEEDED(r.device->CreateBlendState(&desc,&s.blends[i])),"Muzzle blend state");
    }
}
NativeIntegrationWeaponEffects::~NativeIntegrationWeaponEffects()=default;
const NativeWeaponEffectsCounts &NativeIntegrationWeaponEffects::counts()const{return impl->counts;}
UCGunFlareServices NativeIntegrationWeaponEffects::particle_service(){UCGunFlareServices h{};h.user=impl.get();h.particles=Impl::particles;return h;}
bool NativeIntegrationWeaponEffects::shot(uint32_t item){auto&s=*impl;if((item!=2&&item!=17)||s.queued)return false;s.queued=item;return true;}
void NativeIntegrationWeaponEffects::step(const NativeFirstperson &p,float delta,UCGameRandom &rng){
    auto &s=*impl;require(p.equipped&&delta>0&&delta<=1,"Muzzle update input");const auto &gun=p.equipped->gun;
    const bool attached=p.inventory.active_item==2||p.inventory.active_item==17;
    if(attached){
    if(s.bound!=&gun.source){require(uc_muzzle_bind(&gun.source,&s.bone)!=0&&s.bone>=0,"Missing authored DummyFlare");s.bound=&gun.source;}
    require(uc_muzzle_world(s.bone,gun.source.bone_count,gun.prepared.world,gun.prepared.world_valid,p.object,s.muzzle)!=0,"Invalid current muzzle pose");
    memcpy(s.flare.world,s.muzzle,64);
    }
    auto services=s.services();require(attached||!s.queued,"GunFlare request on secondary equipment");
    if(s.queued){uint32_t item=s.queued;s.queued=0;require(!uc_gunflare_shot(&s.flare,s.models.groups,2,item,&rng,&services),"GunFlare shot failed");++s.counts.shots;}
    require(!uc_gunflare_transform(&s.flare,s.models.groups,2,&rng,&services)&&!uc_gunflare_tick(&s.flare,s.models.groups,2,&services),"GunFlare tick failed");
    const float wind[3]{};require(!uc_particle_lifecycle_step(s.lifetime,delta,wind,&rng),"Muzzle particle step failed");
    s.counts.live_systems=uc_particle_lifecycle_count(s.lifetime);s.counts.live_particles=0;
    for(uint32_t i=0;i<s.counts.live_systems;i++)s.counts.live_particles+=uc_particle_lifecycle_at(s.lifetime,i)->system->count;
}
void NativeIntegrationWeaponEffects::draw(NativeRenderer &r,const NativeFirstperson &p,const float object_view[16],const float camera_world[16],
    float aspect,float inverse_exposure,float fraction,bool particles){
    auto &s=*impl;s.counts.model_draws=0;
    const bool attached=p.inventory.active_item==2||p.inventory.active_item==17;
    require(p.equipped&&(!attached||s.bound==&p.equipped->gun.source),"Muzzle draw before step");
    float view[16],projection[16],sliced[16];store(view,XMMatrixInverse(nullptr,matrix(camera_world)));
    require(!uc_firstperson_projection(p.resource.field_of_view,aspect,25,50000,0,1,projection)&&!uc_firstperson_depth_slice(projection,.1f,.15f,sliced),"Muzzle projection");
    float bone[1][16],render_muzzle[16];const uint8_t valid=1;
    if(attached){
    require(p.equipped->gun.presentation_bone(uint32_t(s.bone),fraction,bone[0])&&
        uc_muzzle_world(0,1,bone,&valid,object_view,render_muzzle),"Muzzle presentation bone");
    // The source random flare rotation stays fixed for this simulation tick;
    // only its attachment follows the same displayed bone as the weapon.
    const auto rebase=XMMatrixInverse(nullptr,matrix(s.muzzle))*matrix(render_muzzle);
    for(auto &m:s.models.instances)if(m.visible){
        float model_clip[16];store(model_clip,matrix(m.world)*rebase*matrix(sliced));
        m.model.draw(r,model_clip,m.tint,nullptr,inverse_exposure);++s.counts.model_draws;
    }
    }
    if(particles){
        require(!uc_firstperson_projection(p.camera.fov,aspect,25,50000,0,1,projection),"Muzzle particle projection");
        draw_world(r,view,projection,camera_world);
    }
}
void NativeIntegrationWeaponEffects::draw_world(NativeRenderer &r,const float view[16],const float projection[16],const float camera[16]){
    auto &s=*impl;s.counts.particle_draws=0;
    ComPtr<ID3D11DepthStencilState> saved_depth;ComPtr<ID3D11BlendState> saved_blend;ComPtr<ID3D11RasterizerState> saved_raster;
    UINT stencil=0,mask=0;float factor[4];r.context->OMGetDepthStencilState(&saved_depth,&stencil);
    r.context->OMGetBlendState(&saved_blend,factor,&mask);r.context->RSGetState(&saved_raster);
    float clip[16];store(clip,matrix(view)*matrix(projection));
    NativeParticleQuadView qview{};NativeParticleBillboardView bview{};
    for(unsigned i=0;i<4;i++)for(unsigned j=0;j<4;j++)qview.clip_rows[i][j]=bview.clip_rows[i][j]=clip[j*4+i];
    memcpy(bview.right,camera,16);memcpy(bview.up,camera+4,16);qview.fog[3]=1;
    r.context->OMSetDepthStencilState(s.depth.Get(),0);
    for(uint32_t i=0;i<s.counts.live_systems;i++){
        auto *v=uc_particle_lifecycle_at(s.lifetime,i);size_t index=size_t(v->asset-s.assets);require(index<5&&v->system->count<=128,"Muzzle draw capacity");
        UCParticleQuad quads[128]{};UCParticleSprite sprites[128]{};UCParticleSpriteAdmission admission{};
        require(!uc_particle_lifecycle_geometry(v,camera,s.table,uc_texture_resources_at(s.textures,index),0,1,0,0,quads,sprites,128,&admission),"Muzzle geometry failed");
        if(!admission.count)continue;UCParticleRenderState state{};require(uc_particle_render_begin(&state,admission.blend)!=0,"Muzzle render state");
        qview.alpha_test=bview.alpha_test=state.alpha_test;qview.texture_alpha_kill=bview.texture_alpha_kill=state.texture_alpha_kill;
        r.context->OMSetBlendState(s.blends[admission.blend].Get(),nullptr,~0u);
        if(v->asset->particle_type==1){UCParticleQuadVertex vertices[128*4];require(!uc_particle_quad_batch(quads,admission.count,vertices),"Muzzle quad batch");s.quads.draw(vertices,admission.count,qview,s.images[index].Get(),s.sampler.Get());}
        else for(uint32_t start=0;start<admission.count;start+=39){uint32_t count=std::min(39u,admission.count-start);UCParticleBillboard batch[39];require(!uc_particle_billboard_batch(sprites+start,count,batch),"Muzzle sprite batch");s.sprites.draw(batch,count,bview,s.images[index].Get(),s.sampler.Get());}
        ++s.counts.particle_draws;
    }
    r.context->VSSetShader(r.vertex_shader.Get(),nullptr,0);r.context->PSSetShader(r.pixel_shader.Get(),nullptr,0);r.context->IASetInputLayout(r.layout.Get());
    r.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);ID3D11Buffer *cb=r.constants.Get();r.context->VSSetConstantBuffers(0,1,&cb);r.context->PSSetConstantBuffers(0,1,&cb);
    r.context->OMSetDepthStencilState(saved_depth.Get(),stencil);r.context->OMSetBlendState(saved_blend.Get(),factor,mask);r.context->RSSetState(saved_raster.Get());
}
