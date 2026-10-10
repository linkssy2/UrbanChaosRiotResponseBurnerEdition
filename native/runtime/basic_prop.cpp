#include "basic_prop.h"
#include "model_shadow_casters.h"
#include <DirectXMath.h>
#include <cmath>
#include <cstring>
#include <string>
using namespace DirectX;
static XMMATRIX matrix(const float*p){return XMLoadFloat4x4((const XMFLOAT4X4*)p);}
static bool rotation(const float*m){
    for(unsigned i=0;i<16;i++)if(!std::isfinite(m[i])||std::fabs(m[i])>1e6f)return false;
    if(m[3]!=0||m[7]!=0||m[11]!=0||std::fabs(m[15]-1)>.000001f)return false;
    for(unsigned i=0;i<3;i++)for(unsigned j=0;j<3;j++){float d=0;for(unsigned k=0;k<3;k++)d+=m[4*i+k]*m[4*j+k];if(std::fabs(d-(i==j?1.f:0.f))>.0001f)return false;}
    return XMVectorGetX(XMMatrixDeterminant(matrix(m)))>0;
}
int NativeBasicProp::construct(NativeRenderer&r,NativeMissionWorld&w,uint32_t i,const UCArchive*archive,const uint8_t hash[32],const std::filesystem::path&catalog){
    if(ready_||entity.aux||i>=w.source_count()||!archive||!hash)return 1;
    const auto&m=w.metadata(i);const auto*d=m.definition;
    if(!d||std::strcmp(d->class_name,"BasicProp")||!m.asset_present||*w.active_descriptor()!=&m||!rotation(d->transform))return 1;
    std::string name;for(const char*c=d->asset_name;*c;c++){
        if(*c=='/'||*c=='\\'){if(name.empty()||name.back()!='\\')name+='\\';}
        else name+=(*c>='A'&&*c<='Z')?(char)(*c+'a'-'A'):*c;
    }
    if(name!="assets_scene_items\\03-01\\sis_policebarrier\\sis_policebarrier_skin.amf"&&name!="assets_scene_items\\12-01\\sis_policecar\\sis_policecar_skin.amf")return 1;
    const uint8_t*cursor=*w.property_cursor(i);UCBasicPropParameters p;
    if(uc_basic_prop_parameters(d,&cursor,&p)||!p.body||p.model_value!=0||!p.visible_normal||!p.visible_thermal||p.rotation_x!=0||p.rotation_y!=0||p.rotation_z!=0||p.difficulty_variant)return 1;
    std::string collision=name.substr(0,name.find_last_of('.'))+".mdlhk";BytesOwner bytes;auto at=find_resource(archive,collision.c_str());UCModelCollisionAsset asset{};
    if(at==SIZE_MAX||uc_archive_read(archive,at,UC_AR_LOCAL_PAYLOAD,&bytes.value)!=UC_AR_OK||uc_model_collision_load(bytes.value.data,bytes.value.size,0,&asset))return 1;
    std::vector<UCModelCollisionShape> shapes;bool invalid=asset.group_count!=1;
    for(uint32_t n=0;n<asset.shape_count&&!invalid;n++){UCModelCollisionShape v;const auto&s=asset.shapes[n];if(s.group||s.object||uc_rigid_prop_box(&s,&v)){invalid=true;break;}shapes.push_back(v);}
    uc_model_collision_free(&asset);if(invalid||shapes.empty())return 1;
    try{model_.load(r,archive,hash,catalog,d->asset_name,nullptr);}catch(...){return 1;}
    for(const auto&material:model_.materials)if(!material.supported())return 1;
    parameters_=p;
    if(uc_entity_fresh_construct_authored(&w.context(),&entity.view,w.active_descriptor()))return failure_=2;
    *w.property_cursor(i)=cursor;entity.model=&model_;entity.flags|=0x2000;
    if(entity.flags&0x100||entity.script.attach_actor(entity.view,m,w.script_services()))return failure_=2;
    float identity[16]{};for(unsigned n=0;n<4;n++)identity[n*5]=1;
    try{model_.bind_pose(r,identity);}catch(...){return failure_=2;}
    const XMMATRIX swap=XMMatrixSet(1,0,0,0,0,0,1,0,0,1,0,0,0,0,0,1);
    XMMATRIX world=swap*matrix(entity.transform)*swap;world.r[3]=XMVectorMultiply(world.r[3],XMVectorSet(.01f,.01f,.01f,1));
    for(const auto&s:shapes){UCSolidBlockGeometry b{};XMMATRIX composed=matrix(s.transform)*world;XMStoreFloat3((XMFLOAT3*)b.center,composed.r[3]);XMStoreFloat4((XMFLOAT4*)b.rotation,XMQuaternionNormalize(XMQuaternionRotationMatrix(composed)));std::memcpy(b.half,s.dimensions,12);b.radius=s.radius;boxes_.push_back(b);}
    shapes_=std::move(shapes);ready_=true;return 0;
}
int NativeBasicProp::update(){if(!ready_||failure_)return failure_?failure_:1;if(entity.flags&0x100)return failure_=2;return entity.script.update()?failure_=2:0;}
void NativeBasicProp::draw(NativeRenderer&r,const float*vp,float exposure,NativeModelLighting *lighting,const NativeLightingFrame *frame){if(!ready_||failure_)return;XMFLOAT4X4 m;XMStoreFloat4x4(&m,matrix(entity.transform)*matrix(vp));const float tint[4]={1,1,1,1};if(lighting&&frame)model_.draw_lit(r,*lighting,*frame,entity.transform,&m._11,tint,nullptr,exposure);else model_.draw(r,&m._11,tint,nullptr,exposure);}
void NativeBasicProp::collect_shadows(NativeRenderer&r,NativeModelLighting&lighting,const NativeLightingFrame&frame,NativeModelShadowCasters&casters,float){
    if(!ready_||failure_||!NativeModelShadowCasters::eligible(model_,entity.secondary))return;model_.present(r,1);
    casters.add(model_,entity.transform,entity.secondary,lighting.prepare_membership(model_,frame,entity.transform),frame);
}
int NativeBasicProp::close_detached(){if(entity.script.close_detached())return 1;entity.model=nullptr;boxes_.clear();shapes_.clear();ready_=false;if(entity.aux&&uc_entity_fresh_aux_destroy(entity.aux))return 1;entity.aux=nullptr;return 0;}
