#include "model_shadow_casters.h"
#include <cmath>
#include <cstring>
#include <stdexcept>
namespace {void require(bool x){if(!x)throw std::runtime_error("Native model caster domain");}}
bool NativeModelShadowCasters::eligible(const NativeAnimatedModel&model,uint32_t flags){
 // UCModel owns this inline112-byte array; it is not a nullable guest pointer.
 static_assert(sizeof model.source.source_node>=0x4c);
 uint32_t stencil;memcpy(&stencil,model.source.source_node+0x48,4);
 return (flags&0x10)&&stencil&&model.prepared.ready&&model.lighting;
}
void NativeModelShadowCasters::add(const NativeAnimatedModel&model,const float world[16],uint32_t flags,
 const UCModelLightMembership*membership,const NativeLightingFrame&frame){
 //20BD65 entity+60 bit10 ->21ADC0/B5CE0 model+60 bit2. BB300 requires
 //the actual retained node+48 stencil-batch count, independently of surfaces.
 if(!eligible(model,flags)||!membership)return;
 size_t first_pair=pairs_.size();for(size_t i=0;i<frame.point_count;i++){
  const auto&p=frame.points[i];if(p.shadows&&p.source_index!=UINT32_MAX&&p.membership&&uc_model_light_membership_contains(membership,p.membership))
   pairs_.push_back({0,0,0,0,p.source_index});
 }
 if(pairs_.size()==first_pair)return;
 require(world&&world[3]==0&&world[7]==0&&world[11]==0&&world[15]==1);
 for(unsigned i=0;i<16;i++)require(std::isfinite(world[i]));
 const auto&mesh=model.prepared;size_t first_index=indices_.size(),first_vertex=positions_.size();
 for(uint32_t i=0;i<mesh.range_count;i++){
  const auto&range=mesh.ranges[i];require(range.material_index<model.lighting->materials.size());
  //Reuse qualified ordinary opaque material recipes; masked/alpha/alternate
  //families are explicit omissions rather than solid rectangular shadows.
  const auto&material=model.lighting->materials[range.material_index];
  if(!material.supported||material.masked)continue;
  require(range.first_index<=mesh.index_count&&range.index_count<=mesh.index_count-range.first_index);
  require(indices_.size()+range.index_count<=8000000);
  for(uint32_t j=0;j<range.index_count;j++){uint32_t index=mesh.indices[range.first_index+j];require(index<mesh.vertex_count);indices_.push_back(index);}
 }
 if(indices_.size()==first_index){pairs_.resize(first_pair);return;}
 require(first_vertex+mesh.vertex_count<=1000000&&pairs_.size()<=65536);positions_.resize(first_vertex+mesh.vertex_count);
 const auto*vertices=model.presented_vertices();require(vertices!=nullptr);
 for(uint32_t i=0;i<mesh.vertex_count;i++)for(unsigned k=0;k<3;k++){
  const float*p=vertices[i].position;float v=((p[0]*world[k]+p[1]*world[4+k])+p[2]*world[8+k])+world[12+k];require(std::isfinite(v));positions_[first_vertex+i][k]=v;
 }
 for(size_t i=first_pair;i<pairs_.size();i++){auto&p=pairs_[i];p.first_vertex=uint32_t(first_vertex);p.vertex_count=mesh.vertex_count;p.first_index=uint32_t(first_index);p.index_count=uint32_t(indices_.size()-first_index);}
}
void NativeModelShadowCasters::submit(NativeIntegrationShadows&shadows){
 views_.clear();for(const auto&p:pairs_)views_.push_back({reinterpret_cast<const float(*)[3]>(positions_.data()+p.first_vertex),indices_.data()+p.first_index,p.vertex_count,p.index_count,p.light});
 shadows.set_dynamic_casters(views_.data(),uint32_t(views_.size()));
}
