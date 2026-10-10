#include "integration_actors.h"
#include "model_shadow_casters.h"
#include "audio.h"
extern "C" {
#include "../animation_controller.h"
#include "../character_loop.h"
#include "../collision_asset_query.h"
}
#include <DirectXMath.h>
#include <cmath>
#include <algorithm>
#include <cfloat>
#include <cstring>
#include <stdexcept>

using namespace DirectX;
namespace {
void require(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
uint32_t word(const UCLevelBytes &p,size_t index){
    require(index<p.size/4,"Actor property is truncated");uint32_t v;memcpy(&v,p.data+index*4,4);return v;
}
bool empty_bank(const UCArchiveBytes &b){
    // Actual Default.amp has no ampv/amps/trkl/amrf chunks.24FE60 skips
    // event-hash construction when ampv is absent;24BCA0 then returns-1.
    // Keep this exact empty metadata domain separate from resident PCM loading.
    const uint8_t header[16]={0x20,1,0,0,0,0,0,0,0,0,0,0,0,0,12,0};
    if(b.size<12||memcmp(b.data,"RIFF",4)||memcmp(b.data+8,"AMPC",4))return false;
    uint32_t extent;memcpy(&extent,b.data+4,4);if(size_t(extent)+8!=b.size)return false;
    unsigned seen=0;
    for(size_t p=12;p<b.size;){
        if(b.size-p<8)return false;uint32_t bytes;memcpy(&bytes,b.data+p+4,4);
        if(bytes>b.size-p-8)return false;
        unsigned kind=!memcmp(b.data+p,"name",4)?1:!memcmp(b.data+p,"ampz",4)?2:
            !memcmp(b.data+p,"amph",4)?4:!memcmp(b.data+p,"guid",4)?8:!memcmp(b.data+p,"date",4)?16:0;
        if(!kind||(seen&kind))return false;seen|=kind;
        if(kind==4&&(bytes!=16||memcmp(b.data+p+8,header,16)))return false;
        if(kind==8&&bytes!=16)return false;if(kind==16&&bytes!=8)return false;
        p+=8+bytes;if(p&1)++p;if(p>b.size)return false;
    }
    return seen==31;
}
XMMATRIX matrix(const float *p){XMFLOAT4X4 v;memcpy(&v,p,64);return XMLoadFloat4x4(&v);}
struct Entry {
    NativeActorView view;
    uint32_t cast_flags=0;
    NativeAnimatedModel model;
    UCAnimationController timing{};
    UCAnimationAnnotations annotations{};
    UCCharacterLoop behaviour{};
    UCCharacterLoopController controller{};
    std::vector<int32_t> interrupts;
    float behaviour_world[16]{},delta=0;
    const uint8_t *flags=nullptr;size_t flag_count=0;
    NativeAudioBank bank;
    std::unique_ptr<float[][3]> vertices;
    std::vector<UCCollisionTriangle> triangles;
    ~Entry(){uc_animation_annotations_free(&annotations);}
    void geometry(){
        const auto &p=model.prepared;
        for(unsigned j=0;j<3;j++){view.low[j]=FLT_MAX;view.high[j]=-FLT_MAX;}
        for(uint32_t i=0;i<p.vertex_count;i++){
            const float *point=p.vertices[i].position;
            XMFLOAT3 v;XMStoreFloat3(&v,XMVector3TransformCoord(XMVectorSet(point[0],point[1],point[2],1),matrix(view.world)));
            const float render[3]={v.x,v.y,v.z};uc_collision_to_physics(render,vertices[i]);
            for(unsigned j=0;j<3;j++){view.low[j]=std::min(view.low[j],render[j]);view.high[j]=std::max(view.high[j],render[j]);}
        }
    }
    static UCCharacterLoopServices services(Entry &e){
        UCCharacterLoopServices h{};h.owner=&e;
        h.sound=[](void*v,void*a,void*b,int start){auto&e=*static_cast<Entry*>(v);require(a==&e&&b==&e.bank,"Actor sound identity");
            require(e.bank.find(start?"Start":"Stop")==-1,"Actor sound event needs real playback owner");++e.view.missing_sound_events;};
        // These operations are unreachable in the update-only PC activation
        // domain. Fail loudly if a future caller tries to extend it silently.
        h.attach_condition=[](void*,UCCharacterLoop*,int32_t){throw std::runtime_error("Actor child behaviour entry is unconnected");};
        h.collision=[](void*,void*,int){throw std::runtime_error("Actor body collision setter is unconnected");};
        h.animation_owner=[](void*,void*)->void*{throw std::runtime_error("Actor body animation getter is unconnected");};
        h.animation_begin=[](void*,void*,void*,void**){throw std::runtime_error("Actor body animation request is unconnected");};
        h.animation_cycle=[](void*v,void*a){auto&e=*static_cast<Entry*>(v);require(a==&e.timing,"Actor timing identity");return int(e.timing.phase<e.timing.previous);};
        h.condition=[](void*,void*)->int32_t{throw std::runtime_error("Actor condition object is unconnected");};
        h.interrupt=[](void*v,UCCharacterLoopController*c,int32_t count,const int32_t*indices){auto&e=*static_cast<Entry*>(v);require(c==&e.controller,"Actor controller identity");
            int value=uc_character_loop_flags(e.flags,e.flag_count,count,indices);require(value>=0,"Actor mission flag domain");return value;};
        h.transition=[](void*,UCCharacterLoop*,int32_t)->int{throw std::runtime_error("Actor next behaviour is unconnected");};
        h.base_exit=[](void*,UCCharacterLoop*){throw std::runtime_error("Actor behaviour graph exit is unconnected");};
        h.delta=[](void*v){return static_cast<Entry*>(v)->delta;};
        h.actor_world=[](void*v,void*a)->const float*{auto&e=*static_cast<Entry*>(v);require(a==&e,"Actor world identity");return e.view.world;};
        h.transition_ready=[](void*,const UCCharacterLoop*,int32_t){return 0;};
        return h;
    }
};
}
struct NativeIntegrationActors::Impl {
    std::vector<std::unique_ptr<Entry>> actors;
    UCAnimationBlendMath math{};
    std::vector<uint8_t> standalone_flags;
};
NativeIntegrationActors::NativeIntegrationActors(NativeRenderer &renderer,const std::filesystem::path &game,
    const std::filesystem::path &assets,const UCLevel &level,const uint32_t *ids,size_t count,
    const uint8_t *mission_flags,size_t flag_count):impl(std::make_unique<Impl>()){
    require(level.entities&&ids&&count&&count<=16,"Actor preview needs one to sixteen explicit authored entities");
    uint8_t hash[32];auto archive=read_archive(game/"1-01.ent",hash);
    require((mission_flags&&flag_count==level.field)||(!mission_flags&&!flag_count),"Actor shared mission flags");
    if(!mission_flags){impl->standalone_flags.assign(level.field,0);mission_flags=impl->standalone_flags.data();flag_count=impl->standalone_flags.size();}
    UCArchiveLookup texture_lookup{};require(!uc_archive_lookup_init(&texture_lookup,archive.get(),0),"Actor texture archive lookup");
    auto coefficients=read_file(assets/"animation-coefficients.bin",16384);
    require(uc_animation_blend_math_load(coefficients.data(),coefficients.size(),&impl->math)==UC_ANIMATION_OK,"Actor blend coefficients");
    const float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    for(size_t i=0;i<count;i++){
        require(ids[i]<level.entity_count,"Actor descriptor index");
        for(size_t j=0;j<i;j++)require(ids[j]!=ids[i],"Duplicate actor preview activation");
        const auto &entity=level.entities[ids[i]];
        const bool generic=!strcmp(entity.class_name,"Character"),cop=!strcmp(entity.class_name,"CharacterCop");
        require((generic&&entity.properties.size==40)||(cop&&entity.properties.size==28),"Unsupported authored character recipe");
        const uint32_t behaviour=word(entity.properties,1),spawn=word(entity.properties,3);
        require(behaviour<level.entity_count&&spawn<level.entity_count,"Actor authored references");
        const auto &b=level.entities[behaviour];const auto &s=level.entities[spawn];
        require(!strcmp(b.class_name,"PlayLoopedAnimBehaviour")&&!strcmp(s.class_name,"CharacterSpawnPoint"),"Actor needs authored looping animation and spawn");
        const uint32_t clip=word(b.properties,0);
        float duration;const uint32_t duration_bits=word(b.properties,1);memcpy(&duration,&duration_bits,4);
        require(clip<b.link_count&&duration==-1,"Actor preview only admits authored indefinite loops");
        auto e=std::make_unique<Entry>();e->cast_flags=(entity.flags&4)?0x10:0;auto &v=e->view;
        v.entity_index=ids[i];v.spawn_index=spawn;v.behaviour_index=behaviour;v.faction=word(entity.properties,0);
        v.name=entity.name;v.model=entity.asset_name;v.clip=b.links[clip];memcpy(v.world,s.transform,64);
        const uint32_t interrupt_count=word(b.properties,3);require(interrupt_count<=64&&b.properties.size==(8+interrupt_count)*4,"Actor loop property layout");
        for(uint32_t j=0;j<interrupt_count;j++){uint32_t flag=word(b.properties,4+j);require(flag<flag_count,"Actor interrupt flag");e->interrupts.push_back((int32_t)flag);}
        const uint32_t exit=word(b.properties,4+interrupt_count),bank=word(b.properties,5+interrupt_count);
        require(exit<level.entity_count&&!strcmp(level.entities[exit].class_name,"NoCondition")&&!level.entities[exit].properties.size&&
            bank<b.link_count&&!word(b.properties,7+interrupt_count),"Actor only admits authored NoCondition and collision-enabled body");
        const size_t bank_index=find_resource(archive.get(),b.links[bank]);BytesOwner bank_bytes;
        require(bank_index!=SIZE_MAX&&uc_archive_read(archive.get(),bank_index,UC_AR_LOCAL_PAYLOAD,&bank_bytes.value)==UC_AR_OK,"Actor authored sound bank");
        if(!empty_bank(bank_bytes.value))e->bank.load(bank_bytes.value.data,bank_bytes.value.size);
        require(e->bank.find("Start")==-1&&e->bank.find("Stop")==-1,"Actor bank requires event playback");
        e->flags=mission_flags;e->flag_count=flag_count;e->controller.actor=e.get();
        auto &loop=e->behaviour;loop.controller=&e->controller;loop.actor=e.get();loop.animation=&e->timing;loop.bank=&e->bank;loop.clip=&e->model.sampler.clip;
        loop.world=e->behaviour_world;memcpy(loop.world,b.transform,64);loop.duration=duration;loop.next=(int32_t)word(b.properties,2);loop.exit_condition=(int32_t)exit;
        require(loop.next>=0&&(uint32_t)loop.next<level.entity_count,"Actor next behaviour descriptor");
        loop.interrupt_count=(int32_t)interrupt_count;loop.interrupts=e->interrupts.data();loop.active=1;v.next_behaviour=loop.next;
        try{e->model.load(renderer,archive.get(),hash,assets/"1-01.ent.ucmat",v.model,v.clip,nullptr,&texture_lookup);}
        catch(const std::exception &error){throw std::runtime_error(std::string(v.model)+": "+error.what());}
        for(const auto &material:e->model.materials)
            require(material.supported()&&!material.state.blend_mode&&!material.state.alpha_prepass,"Actor needs unsupported material/pass");
        require(uc_animation_controller_init(&e->model.sampler.clip,0x80000009u,&e->timing)==UC_ANIMATION_OK&&
            uc_animation_annotations_load(&e->model.sampler.clip,&e->annotations)==UC_ANIMATION_OK,"Actor animation controller");
        // Body+A8 animation creation remains a PC adapter; its real clip timing
        // feeds the source behaviour update, with unavailable transitions explicit.
        e->timing.repeats=-1;e->timing.weight=1;
        e->model.sample(renderer,impl->math,0,identity);e->model.reset_presentation();
        const auto &p=e->model.prepared;e->vertices=std::make_unique<float[][3]>(p.vertex_count);
        e->triangles.resize(p.index_count/3);
        for(size_t t=0;t<e->triangles.size();t++)for(unsigned j=0;j<3;j++)e->triangles[t].vertex[j]=p.indices[t*3+j];
        e->geometry();impl->actors.push_back(std::move(e));
    }
}
NativeIntegrationActors::~NativeIntegrationActors()=default;
void NativeIntegrationActors::step(NativeRenderer &renderer,float delta){
    require(std::isfinite(delta)&&delta>=0&&delta<=1,"Actor delta outside preview domain");if(!delta)return;
    const float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    for(auto &p:impl->actors){auto &e=*p;UCAnimationAdvance advance{};UCAnimationEventQueue events{};
        if(e.view.pending_behaviour!=-1)continue;
        e.delta=delta;auto services=Entry::services(e);int result=uc_character_loop_update(&e.behaviour,&services);
        require(result==0||result==2,"Actor behaviour update");++e.view.behaviour_updates;e.view.behaviour_elapsed=e.behaviour.elapsed;
        if(result==2){e.view.pending_behaviour=e.behaviour.next;continue;}
        require(uc_animation_controller_advance(&e.timing,delta,&advance)==UC_ANIMATION_OK&&
            uc_animation_collect_events(&e.annotations,&advance,&events)==UC_ANIMATION_OK,"Actor loop advance");
        e.view.unhandled_animation_events+=events.count;e.view.phase=e.timing.phase;
        e.model.sample(renderer,impl->math,e.timing.phase,identity);e.geometry();
    }
}
void NativeIntegrationActors::draw(NativeRenderer &renderer,const float vp[16],float inverse_exposure,float fraction,NativeModelLighting *lighting,const NativeLightingFrame *frame){
    require(vp&&std::isfinite(inverse_exposure)&&inverse_exposure>0&&std::isfinite(fraction)&&fraction>=0&&fraction<=1,"Actor draw inputs");
    const float white[4]={1,1,1,1};NativeTextureContext texture{renderer.material_phase};
    for(auto &e:impl->actors){
        XMFLOAT4X4 m;XMStoreFloat4x4(&m,matrix(e->view.world)*matrix(vp));e->model.present(renderer,fraction);
        if(lighting&&frame)e->model.draw_lit(renderer,*lighting,*frame,e->view.world,&m._11,white,&texture,inverse_exposure);else e->model.draw(renderer,&m._11,white,&texture,inverse_exposure);
    }
}
void NativeIntegrationActors::collect_shadows(NativeRenderer&r,NativeModelLighting&lighting,const NativeLightingFrame&frame,NativeModelShadowCasters&casters,float fraction){
    for(auto&e:impl->actors)if(NativeModelShadowCasters::eligible(e->model,e->cast_flags)){e->model.present(r,fraction);casters.add(e->model,e->view.world,e->cast_flags,lighting.prepare_membership(e->model,frame,e->view.world),frame);}
}
bool NativeIntegrationActors::raycast(const float from[3],const float to[3],NativeActorHit &out)const{
    require(from&&to,"Actor ray endpoints");for(unsigned i=0;i<3;i++)require(std::isfinite(from[i])&&std::isfinite(to[i]),"Actor ray finite endpoints");
    float a[3],b[3];uc_collision_to_physics(from,a);uc_collision_to_physics(to,b);bool found=false;NativeActorHit result{};
    for(size_t i=0;i<impl->actors.size();i++){
        const auto &e=*impl->actors[i];UCCollisionAsset shape{};shape.vertices=e.vertices.get();shape.vertex_count=e.model.prepared.vertex_count;
        shape.triangles=const_cast<UCCollisionTriangle*>(e.triangles.data());shape.triangle_count=(uint32_t)e.triangles.size();
        shape.scaling[0]=shape.scaling[1]=shape.scaling[2]=1;UCCollisionRayHit h{};const int hit=uc_collision_raycast(&shape,a,b,&h);
        require(hit>=0,"Actor posed triangle query");
        if(hit&&(!found||h.fraction<result.fraction)){
            found=true;result.actor=(uint32_t)i;result.entity_index=e.view.entity_index;result.faction=e.view.faction;
            result.triangle=h.triangle;result.fraction=h.fraction;uc_collision_to_render(h.position,result.position);
            result.normal[0]=h.normal[0];result.normal[1]=h.normal[2];result.normal[2]=h.normal[1];
        }
    }
    if(found)out=result;return found;
}
bool NativeIntegrationActors::damage(uint32_t actor,float amount){
    if(actor>=impl->actors.size()||!std::isfinite(amount)||amount<0)return false;
    auto &v=impl->actors[actor]->view;v.pending_damage=amount;++v.hits;return true;
}
size_t NativeIntegrationActors::size()const{return impl->actors.size();}
const NativeActorView &NativeIntegrationActors::actor(size_t index)const{return impl->actors.at(index)->view;}
