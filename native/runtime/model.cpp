#include <chrono>
#include "model.h"
#include <DirectXMath.h>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <string>
#include <type_traits>

static void resource(const UCArchive *archive,const char *name,BytesOwner &bytes) {
    size_t selected=find_resource(archive,name);
    if(selected==SIZE_MAX||uc_archive_read(archive,selected,UC_AR_LOCAL_PAYLOAD,&bytes.value)!=UC_AR_OK)
        throw std::runtime_error(std::string("Missing native model resource: ")+name);
}
NativeAnimatedModel::~NativeAnimatedModel() {
    lighting.reset();uc_model_render_free(&prepared);uc_animation_sample_free(&sampler);
    uc_animation_binding_free(&binding);uc_model_free(&source);
}
void NativeAnimatedModel::load_action(NativeRenderer &renderer,const UCArchive *archive,const uint8_t hash[32],
    const std::filesystem::path &catalog,const char *model,const char *animation_catalog,const char *alias,const UCArchive *clip_archive) {
    if(source.vertices||!archive||!animation_catalog)throw std::invalid_argument("Invalid animation catalog request");
    BytesOwner bytes;resource(clip_archive?clip_archive:archive,animation_catalog,bytes);
    UCAnimationTemplate t{};
    std::unique_ptr<UCAnimationTemplate,decltype(&uc_animation_template_free)> cleanup(&t,uc_animation_template_free);
    if(uc_animation_template_load(bytes.value.data,bytes.value.size,&t)!=UC_ANIMATION_OK)
        throw std::runtime_error("Unsupported animation action catalog");
    uint32_t index=UINT32_MAX;auto status=uc_animation_template_find_clip(&t,alias,&index);
    std::string clip;
    if(status==UC_ANIMATION_OK&&index!=UINT32_MAX)clip=t.clip_names[index];
    if(clip.empty())throw std::runtime_error("Animation action has no supported authored clip");
    load(renderer,archive,hash,catalog,model,clip.c_str(),clip_archive);
}
void NativeAnimatedModel::load(NativeRenderer &renderer,const UCArchive *archive,const uint8_t hash[32],
    const std::filesystem::path &catalog,const char *model,const char *clip,const UCArchive *clip_archive,UCArchiveLookup *texture_lookup,uint32_t selected_group,size_t *consumed_prefix) {
    if(source.vertices||!archive||!model||(consumed_prefix&&selected_group!=UINT32_MAX))throw std::invalid_argument("Invalid native model load");
    BytesOwner model_bytes,template_bytes,clip_bytes;resource(archive,model,model_bytes);
    size_t consumed=0;
    auto status=consumed_prefix?uc_model_load_prefix(model_bytes.value.data,model_bytes.value.size,&source,&consumed):
        selected_group==UINT32_MAX?uc_model_load(model_bytes.value.data,model_bytes.value.size,&source):
        uc_model_load_group(model_bytes.value.data,model_bytes.value.size,selected_group,&source);
    if(status!=UC_MODEL_OK)
        throw std::runtime_error("Unsupported native model asset");
    if(!clip) {
        if(clip_archive)throw std::invalid_argument("Initial pose has no separate animation archive");
    } else {
    resource(archive,source.animation_template,template_bytes);
    if(clip_archive&&clip_archive!=archive) {
        BytesOwner other;resource(clip_archive,source.animation_template,other);
        if(other.value.size!=template_bytes.value.size||memcmp(other.value.data,template_bytes.value.data,other.value.size))
            throw std::runtime_error("Cross-archive animation skeleton differs from model template");
    }
    resource(clip_archive?clip_archive:archive,clip,clip_bytes);
    UCAnimationTemplate animation_template{};
    if(uc_animation_template_load(template_bytes.value.data,template_bytes.value.size,&animation_template)!=UC_ANIMATION_OK)
        throw std::runtime_error("Unsupported native animation template");
    auto result=uc_animation_binding_init(&source,&animation_template,&binding);
    uc_animation_template_free(&animation_template);
    if(result!=UC_ANIMATION_OK||uc_animation_sample_load(clip_bytes.value.data,clip_bytes.value.size,&sampler)!=UC_ANIMATION_OK)
        throw std::runtime_error("Unsupported native animation binding");
    }
    /* Original base pass parent is RGB; the material can explicitly enable
     * RGBA. The draw helper always installs the resulting mask for each draw. */
    materials=load_materials(catalog,hash,archive,source.material_names,source.material_count,
        UC_NATIVE_MATERIAL_GEOMETRY_SKIN3,false,renderer,texture_lookup);
    std::vector<uint8_t> draw(source.batch_count);
    for(uint32_t i=0;i<source.batch_count;++i)draw[i]=materials[source.batches[i].material_index].supported();
    if(uc_model_render_init(&source,draw.data(),&prepared)!=UC_MODEL_RENDER_OK||!prepared.index_count)
        throw std::runtime_error("Model has no supported native base draws");
#if UC_NATIVE_MODEL_LIGHTING
    lighting=std::make_shared<NativeModelLightData>();lighting->load(renderer,archive,source,materials);
#endif
    if(consumed_prefix)*consumed_prefix=consumed;
}
void NativeAnimatedModel::sample(NativeRenderer &renderer,const UCAnimationBlendMath &math,float phase,const float root[16]) {
    UCAnimationBindingSample entry{&sampler,phase,1,1};sample(renderer,math,&entry,1,root);
}
void NativeAnimatedModel::sample(NativeRenderer &renderer,const UCAnimationBlendMath &math,
    const UCAnimationBindingSample *entries,size_t count,const float root[16],bool graph) {
    prepared.ready=0;
    auto sample_start=renderer.profile_cpu?std::chrono::steady_clock::now():std::chrono::steady_clock::time_point{};
    auto status=graph?uc_animation_binding_sample_graph(&binding,entries,count,&math):
        uc_animation_binding_sample_many(&binding,entries,count,&math);
    if(status!=UC_ANIMATION_OK)throw std::runtime_error(std::string("Native animation binding: ")+uc_animation_status(status));
    if(renderer.profile_cpu)renderer.profile_model_ms[0]+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-sample_start).count();
    prepare(renderer,binding.local,root);
}
void NativeAnimatedModel::bind_pose(NativeRenderer &renderer,const float root[16]) {
    if(!source.vertices)throw std::invalid_argument("Initial pose requires a loaded model");
    prepare(renderer,nullptr,root);
}
void NativeAnimatedModel::prepare(NativeRenderer &renderer,const float (*local)[16],const float root[16]) {
    if(equipment){
        if(!equipment_local_)equipment_local_.reset(new float[source.bone_count][16]);
        for(uint32_t i=0;i<source.bone_count;i++)
            memcpy(equipment_local_[i],local?local[i]:source.bones[i].initial_local,64);
        if(uc_model_pose_equipment(equipment,equipment_local_.get(),source.bone_count))
            throw std::runtime_error("Invalid character equipment bone mapping");
        local=equipment_local_.get();
    }
    if(!presentation_reset_) {
        previous_vertices_.assign(prepared.vertices,prepared.vertices+prepared.vertex_count);
        if(source.bone_count){previous_world_.assign(&prepared.world[0][0],&prepared.world[0][0]+source.bone_count*16);
            previous_valid_.assign(prepared.world_valid,prepared.world_valid+source.bone_count);}
    }
    auto profile_at=renderer.profile_cpu?std::chrono::steady_clock::now():std::chrono::steady_clock::time_point{};
    auto rendered=uc_model_render_update(&prepared,local,root,nullptr,nullptr);
    if(rendered!=UC_MODEL_RENDER_OK)throw std::runtime_error("Native model pose update: "+std::to_string(rendered));
    if(renderer.profile_cpu){auto now=std::chrono::steady_clock::now();renderer.profile_model_ms[1]+=std::chrono::duration<double,std::milli>(now-profile_at).count();profile_at=now;}
    prepared.ready=0; // Publish only after the matching GPU vertices are resident.
    /* Both are plain owned {float3,float2,ARGB} records. Copy into the GPU
     * mapping; no C++ type-punning reads and no per-frame buffer recreation.
     * ponytail: CPU skinning for this first integration; move this exact math
     * to the GPU when matched gameplay identifies it as a material cost. */
    static_assert(std::is_trivially_copyable_v<NativeVertex> && sizeof(NativeVertex)==sizeof(UCModelRenderVertex));
    static_assert(offsetof(NativeVertex,color)==offsetof(UCModelRenderVertex,color));
    if(mesh.vertices)renderer.update(mesh,prepared.vertices,prepared.vertex_count);
    else {
        std::vector<NativeVertex> vertices(prepared.vertex_count);
        memcpy(vertices.data(),prepared.vertices,vertices.size()*sizeof(NativeVertex));
        mesh=renderer.upload(vertices.data(),vertices.size(),prepared.indices,prepared.index_count,true);
    }
    if(renderer.profile_cpu){auto now=std::chrono::steady_clock::now();renderer.profile_model_ms[2]+=std::chrono::duration<double,std::milli>(now-profile_at).count();profile_at=now;}
    prepared.ready=1;presented_fraction_=1;
#if UC_NATIVE_MODEL_LIGHTING
    if(lighting){lighting->reset=presentation_reset_;lighting->prepare(renderer,prepared);}
#endif
    if(renderer.profile_cpu){auto now=std::chrono::steady_clock::now();renderer.profile_model_ms[3]+=std::chrono::duration<double,std::milli>(now-profile_at).count();}
    if(presentation_reset_) {
        previous_vertices_.assign(prepared.vertices,prepared.vertices+prepared.vertex_count);
        if(source.bone_count){previous_world_.assign(&prepared.world[0][0],&prepared.world[0][0]+source.bone_count*16);
            previous_valid_.assign(prepared.world_valid,prepared.world_valid+source.bone_count);}
        presentation_reset_=false;
    }
}
void NativeAnimatedModel::reset_presentation(){presentation_reset_=true;if(lighting)lighting->reset=true;}
void NativeAnimatedModel::present(NativeRenderer &r,float fraction){
    if(!std::isfinite(fraction)||fraction<0||fraction>1||!prepared.ready)
        throw std::invalid_argument("Invalid model presentation fraction");
    if(fraction==presented_fraction_)return;
    presented_fraction_=fraction;
#if UC_NATIVE_MODEL_LIGHTING
    if(lighting)lighting->present(r,fraction);
#endif
    if(fraction==1||presentation_reset_||previous_vertices_.size()!=prepared.vertex_count){
        r.update(mesh,prepared.vertices,prepared.vertex_count);return;
    }
    presentation_vertices_.assign(prepared.vertices,prepared.vertices+prepared.vertex_count);
    for(size_t v=0;v<presentation_vertices_.size();++v)for(unsigned k=0;k<3;++k){
        const float a=previous_vertices_[v].position[k],b=prepared.vertices[v].position[k];
        presentation_vertices_[v].position[k]=fraction==0?a:a+(b-a)*fraction;
    }
    r.update(mesh,presentation_vertices_.data(),presentation_vertices_.size());
}
void native_interpolate_transform(const float before[16],const float after[16],float fraction,float out[16]){
    using namespace DirectX;
    if(!before||!after||!out||!std::isfinite(fraction)||fraction<0||fraction>1)
        throw std::invalid_argument("Invalid transform presentation fraction");
    if(fraction==0||fraction==1){memcpy(out,fraction==0?before:after,64);return;}
    XMFLOAT4X4 a,b;memcpy(&a,before,64);memcpy(&b,after,64);
    XMVECTOR sa,qa,ta,sb,qb,tb;
    // Mirrored authored bones need one consistent reflection axis. DirectX's
    // largest-scale choice can switch axes between almost-unit poses, making
    // interpolated signed scales collapse to zero halfway through the frame.
    auto decompose=[](XMMATRIX m,XMVECTOR &s,XMVECTOR &q,XMVECTOR &t){
        const bool mirrored=XMVectorGetX(XMMatrixDeterminant(m))<0;
        if(mirrored)m.r[0]=XMVectorNegate(m.r[0]);
        if(!XMMatrixDecompose(&s,&q,&t,m))return false;
        if(mirrored)s=XMVectorSetX(s,-XMVectorGetX(s));
        return true;
    };
    if(!decompose(XMLoadFloat4x4(&a),sa,qa,ta)||!decompose(XMLoadFloat4x4(&b),sb,qb,tb))
        throw std::runtime_error("Non-affine presentation transform");
    const auto m=XMMatrixAffineTransformation(XMVectorLerp(sa,sb,fraction),XMVectorZero(),
        XMQuaternionSlerp(qa,qb,fraction),XMVectorLerp(ta,tb,fraction));
    XMStoreFloat4x4(&a,m);memcpy(out,&a,64);
}
bool NativeAnimatedModel::presentation_bone(uint32_t bone,float fraction,float out[16])const{
    if(!out||!std::isfinite(fraction)||fraction<0||fraction>1||bone>=source.bone_count||!prepared.ready||!prepared.world_valid[bone])return false;
    const float *current=prepared.world[bone];
    if(presentation_reset_||previous_valid_.size()!=source.bone_count||!previous_valid_[bone]){
        memcpy(out,current,64);return true;
    }
    native_interpolate_transform(previous_world_.data()+bone*16,current,fraction,out);return true;
}
void NativeAnimatedModel::select_skin(int32_t selection){
    if(!source.vertices||materials.size()!=source.material_count)throw std::runtime_error("Skin selection requires a loaded model");
    std::vector<UCTextureSequenceState> states(materials.size(),UCTextureSequenceState{0,1});
    for(size_t i=0;i<materials.size();i++)if(materials[i].sequence&&
        uc_texture_sequence_skin_state(selection,materials[i].sequence->names.count,&states[i]))
        throw std::runtime_error("Skin selection outside texture sequence");
    skin_states_.swap(states);
}
void NativeAnimatedModel::draw(NativeRenderer &renderer,const float matrix[16],const float tint[4],const NativeTextureContext *context,float inverse_exposure,bool alpha_exempt) {
    if(!prepared.ready||!mesh.vertices)throw std::runtime_error("Native model draw before a valid pose");
    if(context&&((context->states&&context->state_count!=materials.size())||(!context->states&&context->state_count)))
        throw std::runtime_error("Texture state count must match model material order");
    NativeTextureContext selected{context?context->renderer_phase:renderer.material_phase};
    if(!skin_states_.empty()&&(!context||!context->states)){
        selected.states=skin_states_.data();selected.state_count=skin_states_.size();context=&selected;
    }
    // Validate every sequence before submitting any draw. No cached index changes.
    for(uint32_t i=0;i<prepared.range_count;++i) {
        auto index=prepared.ranges[i].material_index;(void)materials[index].texture_for(context,index);
    }
    for(uint32_t i=0;i<prepared.range_count;++i) {
        const auto &range=prepared.ranges[i];const auto &material=materials[range.material_index];
        auto state=material.state;state.inverse_exposure*=inverse_exposure;
        // This PC skinned-model parent uses CULL_NONE. A material with no cull
        // override inherits it; the static-scene prepass's BACK default is not
        // the model parent (and removes the shield's inward-facing window).
        if(state.alpha_prepass){state.prepass_two_sided=true;state.prepass_alpha_exempt=alpha_exempt;}
        renderer.draw(mesh,matrix,tint,material.texture_for(context,range.material_index),range.first_index,range.index_count,state);
    }
}

void NativeAnimatedModel::draw_lit(NativeRenderer&r,NativeModelLighting&lights,const NativeLightingFrame&frame,
 const float world[16],const float clip[16],const float tint[4],const NativeTextureContext*context,float inverse_exposure,bool alpha_exempt,NativeModelLightQuery query){
 if(!prepared.ready||!mesh.vertices)throw std::runtime_error("Native lit draw before pose");
 if(context&&((context->states&&context->state_count!=materials.size())||(!context->states&&context->state_count)))throw std::runtime_error("Texture state count must match model material order");
 NativeTextureContext selected{context?context->renderer_phase:r.material_phase};
 if(!skin_states_.empty()&&(!context||!context->states)){selected.states=skin_states_.data();selected.state_count=skin_states_.size();context=&selected;}
 for(uint32_t i=0;i<prepared.range_count;i++){auto index=prepared.ranges[i].material_index;(void)materials[index].texture_for(context,index);}
#if UC_NATIVE_MODEL_LIGHTING
 lights.draw(*this,r,frame,world,clip,tint,context,inverse_exposure,alpha_exempt,query);
#else
 (void)lights;(void)frame;(void)world;(void)query;draw(r,clip,tint,context,inverse_exposure,alpha_exempt);
#endif
}
