#include "explosion_debris.h"
#include "../texture_asset/texture_asset.h"
#include <DirectXMath.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
using namespace DirectX;
namespace {
void need(bool yes,const char*message){if(!yes)throw std::runtime_error(message);}
std::string shader(const std::filesystem::path&p){auto b=read_file(p,1024*1024);return {b.begin(),b.end()};}
XMMATRIX matrix(const float*p){XMFLOAT4X4 m;std::memcpy(&m,p,64);return XMLoadFloat4x4(&m);}
void store(float*p,FXMMATRIX m){XMFLOAT4X4 f;XMStoreFloat4x4(&f,m);std::memcpy(p,&f,64);}
}
void NativeExplosionDebris::open(NativeRenderer&r,NativeCharacterWorld&world,UCArchiveLookup&lookup,UCGameRandom&random,const std::filesystem::path&shaders){
 need(!renderer_&&lookup.archive&&world.scenery,"Debris owner services");
 sprites_.open(r.device.Get(),r.context.Get(),shader(shaders/"particle_billboard.hlsl"),shader(shaders/"particle.hlsl"));
 D3D11_SAMPLER_DESC sampler{};sampler.Filter=D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;sampler.AddressU=sampler.AddressV=sampler.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;sampler.MaxLOD=D3D11_FLOAT32_MAX;
 need(SUCCEEDED(r.device->CreateSamplerState(&sampler,&sampler_)),"Debris sampler");
 D3D11_BLEND_DESC blend{};auto&target=blend.RenderTarget[0];target.BlendEnable=TRUE;target.SrcBlend=D3D11_BLEND_SRC_ALPHA;target.DestBlend=D3D11_BLEND_INV_SRC_ALPHA;target.BlendOp=D3D11_BLEND_OP_ADD;
 target.SrcBlendAlpha=D3D11_BLEND_ZERO;target.DestBlendAlpha=D3D11_BLEND_ONE;target.BlendOpAlpha=D3D11_BLEND_OP_ADD;target.RenderTargetWriteMask=7;
 need(SUCCEEDED(r.device->CreateBlendState(&blend,&blend_)),"Debris alpha mode2");renderer_=&r;world_=&world;lookup_=&lookup;random_=&random;
}
int NativeExplosionDebris::append(uint32_t count,const float*position,float size,float lifetime,float power,const char*texture){
 if(!renderer_||!position||!texture)return 1;
 if(count_==8)return 0; //48470 admission, before allocation/RNG/resource lookup.
 if(count>65536)return 1; //bounded host allocation; actual callers32/128.
 auto batch=std::make_unique<Batch>();batch->particles.resize(count);
 if(uc_explosion_debris_initialize(&batch->state,batch->particles.data(),count,position,size,lifetime,power,random_))return 1;
 size_t index=SIZE_MAX;need(!uc_archive_lookup(lookup_,texture,&index)&&index!=SIZE_MAX,"Debris texture lookup");
 struct Texture{UCTextureAsset value{};~Texture(){uc_texture_free(&value);}}image;
 need(!uc_texture_load(lookup_->archive,index,UC_AR_LOCAL_PAYLOAD,&image.value),"Debris texture asset");batch->texture=renderer_->upload(image.value);
 batch->order=serial_++;batches_[count_++]=std::move(batch);return 0;
}
int NativeExplosionDebris::explosion(const UCExplosionParameters&parameters,const float*position,uint32_t count,float scale,float power,const char*texture){
 if(parameters.particle_type==1)return 0;if(!position)return 1;
 const float raised[3]={position[0],position[1]+20.f,position[2]};return append(count,raised,(parameters.scale*scale)*8.f,4.f,power,texture);
}
void NativeExplosionDebris::step(float delta){
 need(renderer_&&std::isfinite(delta)&&delta>=0,"Debris step");
 for(unsigned i=0;i<count_;){
  need(!uc_explosion_debris_step(&batches_[i]->state,delta,
   [](void*q,const float*from,const float*to,uint32_t type,uint32_t group,uint32_t mask,float impulse,float*point,float*normal,uint8_t*found){
    auto&w=*static_cast<NativeCharacterWorld*>(q);UCCollisionResult result{};int32_t entity=0;int hit=w.query(from,to,type,group,mask,impulse,result,entity);
    if(hit<0)return 1;*found=(uint8_t)hit;if(hit){std::memcpy(point,result.point,12);std::memcpy(normal,result.normal,12);}return 0;},world_),"Debris collision update481B0");
  if(batches_[i]->state.remaining<=0){batches_[i].reset();--count_;if(i<count_)batches_[i]=std::move(batches_[count_]);}
  else ++i;
 }
}
unsigned NativeExplosionDebris::particles()const{unsigned n=0;for(unsigned i=0;i<count_;i++)n+=batches_[i]->state.count;return n;}
void NativeExplosionDebris::draw(NativeRenderer&r,const float*view,const float*projection){
 need(renderer_==&r&&view&&projection,"Debris draw owner");if(!count_)return;
 float camera[16],clip[16];store(camera,XMMatrixInverse(nullptr,matrix(view)));store(clip,matrix(view)*matrix(projection));NativeParticleBillboardView frame{};
 for(unsigned i=0;i<4;i++)for(unsigned j=0;j<4;j++)frame.clip_rows[i][j]=clip[j*4+i];std::memcpy(frame.up,camera+4,16);std::memcpy(frame.right,camera,16);frame.alpha_test=frame.texture_alpha_kill=1;
 std::array<const Batch*,8> ordered{};for(unsigned i=0;i<count_;i++)ordered[i]=batches_[i].get();std::sort(ordered.begin(),ordered.begin()+count_,[](const Batch*a,const Batch*b){return a->order<b->order;});
 ComPtr<ID3D11BlendState>blend;ComPtr<ID3D11DepthStencilState>depth;ComPtr<ID3D11RasterizerState>raster;float factor[4];UINT mask=0,reference=0;
 r.context->OMGetBlendState(&blend,factor,&mask);r.context->OMGetDepthStencilState(&depth,&reference);r.context->RSGetState(&raster);r.context->OMSetBlendState(blend_.Get(),nullptr,~0u);r.context->OMSetDepthStencilState(r.transparent_depth.Get(),0);
 for(unsigned i=0;i<count_;i++)for(uint32_t first=0;first<ordered[i]->state.count;first+=39){
  unsigned count=std::min(39u,ordered[i]->state.count-first);float positions[39][4],colors[39][4];UCParticleSprite source[39]{};UCParticleBillboard output[39];
  need(!uc_explosion_debris_vertices(&ordered[i]->state,first,count,positions,colors),"Debris billboard geometry48700");
  for(unsigned j=0;j<count;j++){std::memcpy(source[j].center,positions[j],12);source[j].radius=positions[j][3];std::memcpy(source[j].color,colors[j],16);}
  need(!uc_particle_billboard_batch(source,count,output),"Debris billboard packing");sprites_.draw(output,count,frame,ordered[i]->texture.Get(),sampler_.Get());
 }
 r.context->OMSetBlendState(blend.Get(),factor,mask);r.context->OMSetDepthStencilState(depth.Get(),reference);r.context->RSSetState(raster.Get());
 r.context->VSSetShader(r.vertex_shader.Get(),nullptr,0);r.context->PSSetShader(r.pixel_shader.Get(),nullptr,0);r.context->IASetInputLayout(r.layout.Get());r.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
 ID3D11Buffer*constant=r.constants.Get();r.context->VSSetConstantBuffers(0,1,&constant);r.context->PSSetConstantBuffers(0,1,&constant);
}
