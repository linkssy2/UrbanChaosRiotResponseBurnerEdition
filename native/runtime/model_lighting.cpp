#include "model_lighting.h"
#include "model.h"
#include "control_model_bridge.h"
extern "C" {
#include "../point_light_receiver.h"
#include "../point_light_pixel.h"
#include "../model_visibility.h"
#include "../model_material.h"
#include "../texture_asset/texture_asset.h"
}
#include <d3dcompiler.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <xmmintrin.h>
namespace {
void require(bool x,const char*m){if(!x)throw std::runtime_error(m);}void check(HRESULT h){require(SUCCEEDED(h),"Native model lighting GPU failure");}
constexpr float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
ComPtr<ID3D11Buffer> buffer(NativeRenderer&r,UINT bytes,UINT bind,bool dynamic=false,const void*data=nullptr){D3D11_BUFFER_DESC d{};d.ByteWidth=bytes;d.Usage=dynamic?D3D11_USAGE_DYNAMIC:D3D11_USAGE_DEFAULT;d.BindFlags=bind;d.CPUAccessFlags=dynamic?D3D11_CPU_ACCESS_WRITE:0;D3D11_SUBRESOURCE_DATA initial{data};ComPtr<ID3D11Buffer>b;check(r.device->CreateBuffer(&d,data?&initial:nullptr,&b));return b;}
void update_vectors(NativeRenderer&r,ID3D11Buffer*b,const UCModelRenderVectors*v,size_t count){D3D11_MAPPED_SUBRESOURCE map{};check(r.context->Map(b,0,D3D11_MAP_WRITE_DISCARD,0,&map));memcpy(map.pData,v,count*sizeof(*v));r.context->Unmap(b,0);}
void transform(const float v[4],const float m[16],float o[4]){for(unsigned i=0;i<4;i++)o[i]=(m[i]*v[0]+m[4+i]*v[1])+(m[8+i]*v[2]+m[12+i]*v[3]);}
void packed(const float in[4],float out[4]){uint32_t x;require(uc_point_light_constant(in,&x)!=0,"Nonfinite native model light color");out[0]=float((x>>16)&255)/255.f;out[1]=float((x>>8)&255)/255.f;out[2]=float(x&255)/255.f;out[3]=float(x>>24)/255.f;}
bool two_sided(UCMaterialSpan s){if(s.size!=69||!s.data||s.data[0]!=0xC1||s.data[17]!=0x82||s.data[34]!=2||s.data[35]!=0xC0||s.data[52]!=0x82)return false;for(unsigned i=0;i<4;i++){uint32_t x[4];const unsigned offsets[]={1,18,36,53};for(unsigned j=0;j<4;j++)memcpy(x+j,s.data+offsets[j]+4*i,4);if(x[0]||x[1]!=0x93||x[2]!=0x900||x[3]!=0x93)return false;}return true;}
void upload(void*p,unsigned first,const float*v,unsigned n){auto&o=*static_cast<UCNativeControlModelView*>(p);if(first==113&&n==16)memcpy(o.attenuation_rows,v,64);else if(first==112&&n==4)memcpy(o.center,v,16);else throw std::runtime_error("Unknown model receiver constant");}
void receiver(void*,const void*,const void*){}
void directional(const float ray[3],const float world[16],float out[4],float raw[4]){
 float norm=(world[8]*world[8]+world[4]*world[4])+world[0]*world[0];require(std::isfinite(norm)&&norm!=0,"Singular directional model world");
 float inverse[16]={0},scale=1.f/norm;for(unsigned i=0;i<3;i++)for(unsigned j=0;j<3;j++)inverse[i*4+j]=world[j*4+i]*scale;inverse[15]=1;
 float v[4]={ray[0],ray[1],ray[2],0},local[4];transform(v,inverse,local);
 __m128 q=_mm_loadu_ps(local),n=_mm_mul_ps(q,q);n=_mm_add_ps(n,_mm_movehl_ps(n,n));n=_mm_add_ss(n,_mm_shuffle_ps(n,n,1));
 require(_mm_cvtss_f32(n)>0,"Zero directional light");__m128 r=_mm_rsqrt_ss(n),correction=_mm_sub_ss(_mm_set_ss(3),_mm_mul_ss(_mm_mul_ss(r,n),r));r=_mm_mul_ss(_mm_mul_ss(_mm_set_ss(.5f),r),correction);r=_mm_shuffle_ps(r,r,0);
 _mm_storeu_ps(local,_mm_xor_ps(_mm_mul_ps(q,r),_mm_set1_ps(-0.f)));memcpy(raw,local,16);float neg[4],a[4],b[4];for(unsigned i=0;i<4;i++)neg[i]=-local[i];packed(local,a);packed(neg,b);for(unsigned i=0;i<4;i++)out[i]=a[i]-b[i];
}
}
NativeModelLightData::~NativeModelLightData(){uc_model_light_membership_free(membership);uc_model_render_free(&prepared);}
void NativeModelLightData::load(NativeRenderer&r,const UCArchive*a,const UCModel&m,const std::vector<NativeMaterial>&base){
 materials.resize(m.material_count);std::vector<uint8_t> draw(m.batch_count);
 for(uint32_t i=0;i<m.material_count;i++){
  auto&out=materials[i];if(!base[i].supported()||base[i].state.depth_disabled)continue;
  size_t index=find_resource(a,m.material_names[i]);if(index==SIZE_MAX)continue;UCMaterialAsset asset{};
  if(uc_material_load(a,index,UC_AR_LOCAL_PAYLOAD,&asset)!=UC_MATERIAL_OK)continue;
  // Skin4 chooses the skinned VS; flag8 only changes ordinary base write mask.
  out.two_sided=two_sided(asset.setup_program[1]);
  if(base[i].state.alpha_prepass&&base[i].state.blend_mode==1&&base[i].sequence&&out.two_sided&&uc_model_material_masked(&asset)){
   out.masked=out.supported=out.receives=true;uc_material_free(&asset);continue;
  }
  if(base[i].state.blend_mode||base[i].state.alpha_prepass){uc_material_free(&asset);continue;}
  UCMaterialAsset ordinary=asset;ordinary.header[9]=255;
  out.supported=!(asset.effective_flags&~uint16_t(4|8|2))&&asset.header[8]==255&&
   (!asset.setup_program[1].size||out.two_sided)&&uc_model_material_prepass(&ordinary,base[i].sequence!=nullptr,&out.prepass);
  out.write_alpha=(asset.effective_flags&8)!=0;
  if(out.supported&&asset.header[9]!=255){
   unsigned dep=asset.header[9];out.supported=false;
   if(dep<asset.dependency_count&&asset.dependencies[dep].kind){
    size_t image=find_resource(a,asset.dependencies[dep].name);UCTextureAsset texture{};
    if(image!=SIZE_MAX&&uc_texture_load(a,image,UC_AR_LOCAL_PAYLOAD,&texture)==UC_TEXTURE_OK){
     if(!(texture.source_flags&0xFF00)){out.auxiliary=r.upload(texture);out.supported=true;}
     uc_texture_free(&texture);
    }
   }
  }
  out.receives=out.supported&&!(asset.effective_flags&2);
  out.specular=float(asset.header[6])*(1.f/255.f);uc_material_free(&asset);
 }
 bool any=false;for(uint32_t i=0;i<m.batch_count;i++){draw[i]=materials[m.batches[i].material_index].supported;any|=draw[i]!=0;}
 if(any)require(uc_model_render_init_lighting(&m,draw.data(),&prepared)==UC_MODEL_RENDER_OK,"Native model lighting geometry");
}
void NativeModelLightData::prepare(NativeRenderer&r,const UCModelRender&base){
 if(!prepared.model)return;
 ++pose_revision;
 if(!reset){previous_vertices.assign(prepared.vertices,prepared.vertices+prepared.vertex_count);previous_vectors.assign(prepared.vectors,prepared.vectors+prepared.vertex_count);}
 require(uc_model_render_update_lighting(&prepared,&base,nullptr)==UC_MODEL_RENDER_OK,"Native model light pose");
 if(mesh.vertices)r.update(mesh,prepared.vertices,prepared.vertex_count);
 else{std::vector<NativeVertex> v(prepared.vertex_count);memcpy(v.data(),prepared.vertices,v.size()*sizeof(NativeVertex));mesh=r.upload(v.data(),v.size(),prepared.indices,prepared.index_count,true);vectors=buffer(r,UINT(prepared.vertex_count*sizeof(UCModelRenderVectors)),D3D11_BIND_VERTEX_BUFFER,true);}
 update_vectors(r,vectors.Get(),prepared.vectors,prepared.vertex_count);
 if(reset){previous_vertices.assign(prepared.vertices,prepared.vertices+prepared.vertex_count);previous_vectors.assign(prepared.vectors,prepared.vectors+prepared.vertex_count);reset=false;}
}
void NativeModelLightData::present(NativeRenderer&r,float f){
 if(!prepared.model)return;require(prepared.ready&&std::isfinite(f)&&f>=0&&f<=1,"Native model light presentation");
 if(f==1){r.update(mesh,prepared.vertices,prepared.vertex_count);update_vectors(r,vectors.Get(),prepared.vectors,prepared.vertex_count);return;}
 present_vertices.assign(prepared.vertices,prepared.vertices+prepared.vertex_count);present_vectors.assign(prepared.vectors,prepared.vectors+prepared.vertex_count);
 for(size_t i=0;i<present_vertices.size();i++){
  for(unsigned j=0;j<3;j++){float a=previous_vertices[i].position[j],b=prepared.vertices[i].position[j];present_vertices[i].position[j]=f==0?a:a+(b-a)*f;}
  float a[9],b[9],v[9];memcpy(a,&previous_vectors[i],36);memcpy(b,&prepared.vectors[i],36);for(unsigned j=0;j<9;j++)v[j]=f==0?a[j]:a[j]+(b[j]-a[j])*f;memcpy(&present_vectors[i],v,36);
 }
 r.update(mesh,present_vertices.data(),present_vertices.size());update_vectors(r,vectors.Get(),present_vectors.data(),present_vectors.size());
}
struct NativeModelLighting::Impl {
 struct Ordered{uint32_t source,kind;size_t index;};std::vector<Ordered> ordered;
 NativeIntegrationLighting&lights;ComPtr<ID3D11VertexShader>vs;ComPtr<ID3D11PixelShader>ps,ambient;
 ComPtr<ID3D11InputLayout>layout;ComPtr<ID3D11Buffer>constants;ComPtr<ID3D11BlendState>blend;ComPtr<ID3D11DepthStencilState>depth,predepth;ComPtr<ID3D11RasterizerState>raster;
 Impl(NativeIntegrationLighting&l):lights(l){}
 void bind(NativeRenderer&r,NativeModelLightData&m,const UCNativeControlModelView&v,bool prepass,uint32_t first,uint32_t count){
  auto*c=r.context.Get();ID3D11ShaderResourceView *cube,*point,*spot;ID3D11SamplerState*sampler;lights.model_resources(cube,point,spot,sampler);
  ID3D11Buffer*vb[]={m.mesh.vertices.Get(),m.vectors.Get()};UINT strides[]={24,sizeof(UCModelRenderVectors)},offsets[]={0,0};
  c->IASetVertexBuffers(0,2,vb,strides,offsets);c->IASetIndexBuffer(m.mesh.indices.Get(),DXGI_FORMAT_R32_UINT,0);c->IASetInputLayout(layout.Get());c->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  c->VSSetShader(vs.Get(),nullptr,0);c->PSSetShader(prepass?ambient.Get():ps.Get(),nullptr,0);c->UpdateSubresource(constants.Get(),0,nullptr,&v,0,0);ID3D11Buffer*cb=constants.Get();c->VSSetConstantBuffers(1,1,&cb);c->PSSetConstantBuffers(1,1,&cb);
  ID3D11ShaderResourceView*images[]={cube,v.kind%3==2?spot:point};c->PSSetShaderResources(0,2,images);ID3D11SamplerState*ss[]={sampler,sampler};c->PSSetSamplers(0,2,ss);
  c->RSSetState(prepass?r.rasterizer.Get():raster.Get());c->OMSetDepthStencilState(prepass?predepth.Get():depth.Get(),0);c->OMSetBlendState(prepass?r.blends[0][0].Get():blend.Get(),nullptr,UINT32_MAX);c->DrawIndexed(count,first,0);
 }
};
NativeModelLighting::NativeModelLighting(NativeRenderer&r,NativeIntegrationLighting&l,const std::filesystem::path&dir):p_(new Impl(l)){
 auto&s=*p_;auto path=dir/L"model_lighting.hlsl";ComPtr<ID3DBlob>code,error;
 for(unsigned i=0;i<3;i++){
  UINT flags=D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_OPTIMIZATION_LEVEL3;if(i)flags|=D3DCOMPILE_IEEE_STRICTNESS;
  HRESULT h=D3DCompileFromFile(path.c_str(),nullptr,D3D_COMPILE_STANDARD_FILE_INCLUDE,i==0?"VSNative":i==1?"PSNative":"Ambient",i==0?"vs_5_0":"ps_5_0",flags,0,&code,&error);
  if(FAILED(h)&&error)throw std::runtime_error((const char*)error->GetBufferPointer());check(h);
  if(!i){check(r.device->CreateVertexShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,&s.vs));D3D11_INPUT_ELEMENT_DESC input[]={{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0},{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,1,0,D3D11_INPUT_PER_VERTEX_DATA,0},{"BASIS",5,DXGI_FORMAT_R32G32B32_FLOAT,1,12,D3D11_INPUT_PER_VERTEX_DATA,0},{"BASIS",6,DXGI_FORMAT_R32G32B32_FLOAT,1,24,D3D11_INPUT_PER_VERTEX_DATA,0}};check(r.device->CreateInputLayout(input,5,code->GetBufferPointer(),code->GetBufferSize(),&s.layout));}
  else check(r.device->CreatePixelShader(code->GetBufferPointer(),code->GetBufferSize(),nullptr,i==1?s.ps.GetAddressOf():s.ambient.GetAddressOf()));code.Reset();error.Reset();
 }
 s.constants=buffer(r,sizeof(UCNativeControlModelView),D3D11_BIND_CONSTANT_BUFFER);
 D3D11_BLEND_DESC b{};auto&t=b.RenderTarget[0];t.BlendEnable=TRUE;t.SrcBlend=t.DestBlend=t.SrcBlendAlpha=t.DestBlendAlpha=D3D11_BLEND_ONE;t.BlendOp=t.BlendOpAlpha=D3D11_BLEND_OP_ADD;t.RenderTargetWriteMask=15;check(r.device->CreateBlendState(&b,&s.blend));
 D3D11_DEPTH_STENCIL_DESC d{};d.DepthEnable=TRUE;d.DepthFunc=D3D11_COMPARISON_LESS_EQUAL;d.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ZERO;check(r.device->CreateDepthStencilState(&d,&s.depth));d.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ALL;check(r.device->CreateDepthStencilState(&d,&s.predepth));
 D3D11_RASTERIZER_DESC rs{};rs.FillMode=D3D11_FILL_SOLID;rs.CullMode=D3D11_CULL_NONE;rs.DepthClipEnable=TRUE;rs.DepthBias=-100;rs.SlopeScaledDepthBias=-.5f;check(r.device->CreateRasterizerState(&rs,&s.raster));
}
NativeModelLighting::~NativeModelLighting()=default;
const UCModelLightMembership* NativeModelLighting::prepare_membership(NativeAnimatedModel&model,const NativeLightingFrame&frame,const float world[16],NativeModelLightQuery query){
 auto*data=model.lighting.get();if(!data||!data->prepared.model)return nullptr;auto&m=*data;
 if(frame.generation&&frame.owner&&m.member_owner==frame.owner&&m.member_frame==frame.generation&&m.member_pose==m.pose_revision&&!memcmp(m.member_world,world,64))return m.membership;
 UCModelBounds bounds{};require(uc_model_bounds(&model.source,model.prepared.world,model.source.bone_count,model.prepared.world_valid,&bounds)==UC_MODEL_OK,"Native light model bounds");
 if(!m.membership){float center[3]={99999,99999,99999};m.membership=uc_model_light_membership_create(std::min(65536u,p_->lights.counts().authored+1024u),0,center);require(m.membership!=nullptr,"Native model membership allocation");}
 m.points.clear();for(size_t i=0;i<frame.point_count;i++){require(frame.points[i].membership!=nullptr,"Missing source point membership");m.points.push_back(frame.points[i].membership);}
 struct Query{NativeModelLightQuery q;}q{query};UCModelLightMembershipView members{};members.generation=++m.generation;members.static_budget=3;members.world=world;members.bounds=&bounds;members.model=&model.source;members.lights=m.points.data();members.light_count=uint32_t(m.points.size());members.context=&q;
 members.world_query=[](void*p,const UCModelMembershipLight*l,const float center[3]){auto&q=static_cast<Query*>(p)->q;return q.occluded?q.occluded(q.owner,center,l->geometry->world_center):0;};
 require(uc_model_light_membership_update(m.membership,&members)!=0,"Native actor point membership needs original query service");
 m.member_owner=frame.owner;m.member_frame=frame.generation;m.member_pose=m.pose_revision;memcpy(m.member_world,world,64);return m.membership;
}
void NativeModelLighting::draw(NativeAnimatedModel&model,NativeRenderer&r,const NativeLightingFrame&frame,const float world[16],const float clip[16],const float tint[4],const NativeTextureContext*textures,float inverse_exposure,bool alpha_exempt,NativeModelLightQuery query){
 auto*data=model.lighting.get();if(!data||!data->prepared.model){model.draw(r,clip,tint,textures,inverse_exposure,alpha_exempt);return;}
 auto&m=*data;auto&s=*p_;require(m.prepared.ready&&std::isfinite(frame.exposure)&&frame.exposure>0,"Native model light draw state");UCNativeControlModelView common{};
 for(unsigned i=0;i<4;i++)for(unsigned j=0;j<4;j++)common.clip_rows[i][j]=clip[j*4+i];
 // Original opaque977A0 ambient first. Reuse the actual base position stream
 // and exact NativeRenderer VS for its depth; separately compiled contractions
 // (or the auxiliary light palette frontier) must not define base-pass depth.
 auto*c=r.context.Get();c->VSSetShader(r.vertex_shader.Get(),nullptr,0);c->PSSetShader(s.ambient.Get(),nullptr,0);c->IASetInputLayout(r.layout.Get());
 ID3D11Buffer*base_constants=r.constants.Get();c->VSSetConstantBuffers(0,1,&base_constants);c->PSSetConstantBuffers(0,1,&base_constants);
 ID3D11Buffer*light_constants=s.constants.Get();c->PSSetConstantBuffers(1,1,&light_constants);
 for(uint32_t i=0;i<model.prepared.range_count;i++){
  const auto&range=model.prepared.ranges[i];const auto&recipe=m.materials[range.material_index];if(!recipe.supported)continue;
  if(recipe.masked){
   const auto&material=model.materials[range.material_index];auto state=material.state;state.prepass_two_sided=true;state.prepass_alpha_exempt=alpha_exempt;
   memcpy(state.constant,frame.ambient,12);state.inverse_exposure=1;
   c->PSSetShader(r.pixel_shader.Get(),nullptr,0);r.draw(model.mesh,clip,tint,material.texture_for(textures,range.material_index),range.first_index,range.index_count,state);continue;
  }
  c->PSSetShader(s.ambient.Get(),nullptr,0);
  float color[4]={frame.ambient[0],frame.ambient[1],frame.ambient[2],1};
  if(recipe.prepass.emission){float add=(float(recipe.prepass.emission)*(1.f/255.f))/frame.exposure;for(unsigned k=0;k<4;k++)color[k]=color[k]*tint[k]+add;}
  packed(color,common.color);common.material_kind=recipe.auxiliary?2u:0u;c->UpdateSubresource(s.constants.Get(),0,nullptr,&common,0,0);
  ID3D11ShaderResourceView*image=recipe.auxiliary.Get();c->PSSetShaderResources(2,1,&image);ID3D11SamplerState*sampler=r.samplers[0].Get();c->PSSetSamplers(2,1,&sampler);
  NativeDrawState state;state.write_alpha=false;
  r.draw(model.mesh,clip,tint,nullptr,range.first_index,range.index_count,state);
 }
 common.material_kind=0;
 UCModelBounds bounds{};require(uc_model_bounds(&model.source,model.prepared.world,model.source.bone_count,model.prepared.world_valid,&bounds)==UC_MODEL_OK,"Native light model bounds");
 UCModelBounds world_bounds{};for(unsigned i=0;i<3;i++){float middle=(world[i]*bounds.center[0]+world[12+i])+(world[4+i]*bounds.center[1]+world[8+i]*bounds.center[2]);float extent=fabsf(world[i]*bounds.extent[0])+(fabsf(world[4+i]*bounds.extent[1])+fabsf(world[8+i]*bounds.extent[2]));world_bounds.minimum[i]=middle-extent;world_bounds.maximum[i]=middle+extent;}
 float inv[16];require(uc_point_light_inverse(world,inv)!=0,"Native model light inverse");float eye[4]={frame.eye[0],frame.eye[1],frame.eye[2],1};transform(eye,inv,common.eye);
 prepare_membership(model,frame,world,query);
 auto light=[&](UCNativeControlModelView v,float intensity,bool specular){
  const unsigned base=v.kind;
  for(uint32_t j=0;j<m.prepared.range_count;j++){const auto&range=m.prepared.ranges[j];const auto&material=m.materials[range.material_index];if(!material.receives)continue;v.two_sided=material.two_sided;v.material_kind=material.masked?1u:0u;
   if(material.masked){const auto&source=model.materials[range.material_index];ID3D11ShaderResourceView*image=source.texture_for(textures,range.material_index);c->PSSetShaderResources(2,1,&image);ID3D11SamplerState*sampler=source.state.point_filter?r.point_sampler.Get():r.samplers[(source.state.clamp_u?1:0)|(source.state.clamp_v?2:0)].Get();c->PSSetSamplers(2,1,&sampler);}
   if((base==0||specular)&&material.specular>=.05f){v.kind=base+3;float spec[4];for(float&x:spec)x=intensity*material.specular;packed(spec,v.specular);}else v.kind=base;
   s.bind(r,m,v,false,range.first_index,range.index_count);
  }
 };
 s.ordered.clear();for(size_t i=0;i<frame.point_count;i++)s.ordered.push_back({frame.points[i].source_index,1,i});
 for(size_t i=0;i<frame.directional_count;i++)s.ordered.push_back({frame.directionals[i].source_index,0,i});
 for(size_t i=0;i<frame.spot_count;i++)s.ordered.push_back({frame.spot_indices?frame.spot_indices[i]:UINT32_MAX,2,i});
 std::stable_sort(s.ordered.begin(),s.ordered.end(),[](const Impl::Ordered&a,const Impl::Ordered&b){return a.source<b.source;});
 for(const auto&entry:s.ordered){size_t i=entry.index;
  if(entry.kind==1){const auto&p=frame.points[i];if(!uc_model_light_membership_contains(m.membership,p.membership))continue;auto v=common;v.kind=1;float provider[16],center[4];uint8_t dirty=0;UCPointLightReceiverTarget target{provider,&dirty,center,&v,upload,receiver};
   if(uc_point_light_receiver(p.membership->geometry,world,&bounds,&model,nullptr,&target)!=2)continue;float color[4];for(unsigned k=0;k<4;k++)color[k]=(p.color[k]*p.intensity)/frame.exposure;packed(color,v.color);light(v,p.intensity,p.specular);
  }else if(entry.kind==0){const auto&p=frame.directionals[i];auto v=common;v.kind=0;directional(p.direction,world,v.direction,v.center);float color[4];for(unsigned k=0;k<4;k++)color[k]=(p.color[k]*p.intensity)/frame.exposure;packed(color,v.color);light(v,p.intensity,true);
  }else{const auto&p=frame.spots[i];require(frame.spot_planes&&frame.spot_specular,"Native model spotlight source metadata");uint32_t visible=0;
   require(uc_model_visibility_plane(p.projection,&world_bounds,frame.spot_planes[i],&visible)==UC_MODEL_OK,"Native model spotlight bounds");if(!visible)continue;
   auto v=common;v.kind=2;UCNativeControlSpot local=p;for(unsigned k=0;k<4;k++)transform(world+4*k,p.projection,local.projection+4*k);
   require(uc_spot_light_rows(&local,v.attenuation_rows)!=0,"Native model spot projection");transform(p.center,inv,v.center);float color[4];for(unsigned k=0;k<4;k++)color[k]=(p.color[k]*p.intensity)/frame.exposure;packed(color,v.color);light(v,p.intensity,frame.spot_specular[i]!=0);
  }
 }
 // Every base draw installs its own material state after these shared resources.
 c->VSSetShader(r.vertex_shader.Get(),nullptr,0);c->PSSetShader(r.pixel_shader.Get(),nullptr,0);c->IASetInputLayout(r.layout.Get());ID3D11Buffer*cb=r.constants.Get();c->VSSetConstantBuffers(0,1,&cb);c->PSSetConstantBuffers(0,1,&cb);
 for(uint32_t i=0;i<model.prepared.range_count;i++){const auto&range=model.prepared.ranges[i];const auto&material=model.materials[range.material_index];if(m.materials[range.material_index].masked)continue;auto state=material.state;state.inverse_exposure*=inverse_exposure;if(m.materials[range.material_index].supported){state.blend_mode=3;state.inverse_exposure=1;state.vertex_exposure=false;state.write_alpha=m.materials[range.material_index].write_alpha;}if(state.alpha_prepass){state.prepass_two_sided=true;state.prepass_alpha_exempt=alpha_exempt;}r.draw(model.mesh,clip,tint,material.texture_for(textures,range.material_index),range.first_index,range.index_count,state);}
}
