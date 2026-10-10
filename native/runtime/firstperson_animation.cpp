#include "firstperson_animation.h"
#include <array>
#include <stdexcept>

static void require(bool ok,const char *what){if(!ok)throw std::runtime_error(what);}
static void read(const UCArchive *archive,const char *name,BytesOwner &bytes){
    size_t i=find_resource(archive,name);
    require(i!=SIZE_MAX&&uc_archive_read(archive,i,UC_AR_LOCAL_PAYLOAD,&bytes.value)==UC_AR_OK,
        "Missing first-person animation resource");
}
NativeFirstpersonClips::~NativeFirstpersonClips(){
    uc_animation_template_free(&catalog);uc_animation_template_free(&skeleton);
}
void NativeFirstpersonClips::load(const UCArchive *archive,const char *catalog_name,const char *skeleton_name){
    require(clips.empty()&&!catalog.clip_names&&!skeleton.bone_names,"Animation bank already loaded");
    BytesOwner c,s;read(archive,catalog_name,c);read(archive,skeleton_name,s);
    require(uc_animation_template_load(c.value.data,c.value.size,&catalog)==UC_ANIMATION_OK&&
        uc_animation_template_load(s.value.data,s.value.size,&skeleton)==UC_ANIMATION_OK,
        "Unsupported first-person animation catalog or skeleton");
    for(uint32_t i=0;i<catalog.clip_count;i++){
        BytesOwner bytes;read(archive,catalog.clip_names[i],bytes);
        auto clip=std::make_unique<NativeFirstpersonClip>();
        clip->bytes.assign(bytes.value.data,bytes.value.data+bytes.value.size);
        UCAnimationClip decoded{};
        std::unique_ptr<UCAnimationClip,decltype(&uc_animation_clip_free)> cleanup(&decoded,uc_animation_clip_free);
        require(uc_animation_clip_load(clip->bytes.data(),clip->bytes.size(),&decoded)==UC_ANIMATION_OK&&
            uc_animation_annotations_load(&decoded,&clip->annotations)==UC_ANIMATION_OK,
            "Unsupported first-person animation records");
        clips.push_back(std::move(clip));
    }
}
const NativeFirstpersonClip &NativeFirstpersonClips::find(const char *alias) const{
    uint32_t index=UINT32_MAX;
    require(uc_animation_template_find_clip(&catalog,alias,&index)==UC_ANIMATION_OK&&index<clips.size(),
        "First-person action has no authored animation");
    return *clips[index];
}
static std::array<UCAnimationControllerEntry,32> entries(NativeFirstpersonAnimations &owner){
    require(owner.list.size()<=32,"First-person controller count");
    std::array<UCAnimationControllerEntry,32> result{};
    for(size_t i=0;i<owner.list.size();i++){
        auto &c=owner.list[i];result[i]={&c->timing,&c->sample.clip,&c->resource->annotations};
    }
    return result;
}
void NativeFirstpersonAnimations::apply(const NativeFirstpersonClips &bank,const UCMainplayerPresentationAction &action){
    const auto &clip=bank.find(action.alias);
    auto next=std::make_unique<NativeFirstpersonController>();next->resource=&clip;
    require(uc_animation_sample_load(clip.bytes.data(),clip.bytes.size(),&next->sample)==UC_ANIMATION_OK&&
        uc_mainplayer_presentation_controller(&next->sample.clip,&action,&next->timing)==UC_ANIMATION_OK,
        "Unsupported first-person action controller");
    auto current_entries=entries(*this);uint32_t index=0;
    require(uc_animation_controller_list_insert_index(current_entries.data(),(uint32_t)list.size(),
        &next->timing,&index)==UC_ANIMATION_OK,"First-person controller insertion");
    // Reserve before clearing the previous current controller, so allocation
    // failure cannot change an already published action.
    list.reserve(list.size()+1);
    if(current)current->timing.flags&=~9u;
    current=next.get();list.insert(list.begin()+index,std::move(next));
}
void NativeFirstpersonAnimations::update(NativeRenderer &renderer,NativeAnimatedModel &model,
    const NativeFirstpersonClips &bank,const UCAnimationBlendMath &math,float delta,UCAnimationEventQueue &events,NativeAnimatedModel *attachment){
    auto input=entries(*this);UCAnimationControllerListUpdate result{};
    require(uc_animation_controller_list_advance(input.data(),(uint32_t)list.size(),&bank.skeleton,delta,&result)==UC_ANIMATION_OK,
        "First-person controller list update");
    UCAnimationBindingSample samples[32]{};size_t count=0;
    for(uint32_t i=0;i<result.survivor_count;i++){
        auto &c=*list[result.survivors[i]];
        if(c.timing.flags&0x80000000u)samples[count++]={&c.sample,c.timing.phase,c.timing.weight,
            uc_animation_controller_root_factor(&c.timing)};
    }
    const float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    model.sample(renderer,math,samples,count,identity);
    if(attachment)attachment->sample(renderer,math,samples,count,identity);
    events=result.events; // References point into the bank, not removed instances.
    for(uint32_t i=0;i<result.removed_count;i++)if(list[result.removed[i]].get()==current)current=nullptr;
    size_t retained=0;
    for(uint32_t i=0;i<result.survivor_count;i++){
        size_t index=result.survivors[i];
        if(retained!=index)list[retained]=std::move(list[index]);
        ++retained;
    }
    list.resize(retained);
}
