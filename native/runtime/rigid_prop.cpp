#include "rigid_prop.h"
#include "model_shadow_casters.h"
#include <DirectXMath.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>
using namespace DirectX;
NativeRigidProp::~NativeRigidProp(){uc_entity_subscriptions_close(subscriptions_);}
int native_prop_explosion_step(UCExplosionField&field,float delta,const std::vector<std::unique_ptr<NativeRigidProp>>&props){
    if(props.size()>4096)return 1;
    std::vector<UCExplosionBody> bodies;std::vector<NativeRigidProp*> active;
    bodies.reserve(props.size());active.reserve(props.size());
    for(const auto&owner:props){
        if(!owner||owner->failure())return 1;if(!owner->ready())continue;
        active.push_back(owner.get());bodies.emplace_back();const auto&p=*owner;auto&b=bodies.back();
        if(*p.entity.view.id<0)return 1;
        b.identity=uint32_t(*p.entity.view.id)+1;b.dynamic=!p.parameters().unmovable;
        uc_collision_to_physics(p.entity.transform+12,b.position);std::memcpy(b.force_position,b.position,12);
    }
    if(uc_explosion_field_step(&field,delta,bodies.data(),bodies.size()))return 1;
    for(size_t i=0;i<active.size();i++)if(bodies[i].impulsed&&active[i]->impulse(bodies[i].impulse))return 1;
    return 0;
}
static XMMATRIX matrix(const float*p){return XMLoadFloat4x4((const XMFLOAT4X4*)p);}
static std::string lower(const char*p){std::string s=p?p:"";for(char&c:s){if(c=='/')c='\\';else if(c>='A'&&c<='Z')c+=(char)('a'-'A');}return s;}
static bool rotation(const float*m){
    for(unsigned i=0;i<16;i++)if(!std::isfinite(m[i])||std::fabs(m[i])>1e6f)return false;
    if(m[3]!=0||m[7]!=0||m[11]!=0||m[15]!=1)return false;
    for(unsigned i=0;i<3;i++)for(unsigned j=0;j<3;j++){float d=0;for(unsigned k=0;k<3;k++)d+=m[4*i+k]*m[4*j+k];if(std::fabs(d-(i==j?1.f:0.f))>.0001f)return false;}
    return XMVectorGetX(XMMatrixDeterminant(matrix(m)))>0;
}
int NativeRigidProp::construct(NativeRenderer&r,NativeMissionWorld&w,uint32_t i,const UCArchive*archive,
    const uint8_t hash[32],const std::filesystem::path&catalog,const UCCollisionAsset&scenery){
    if(ready_||entity.aux||i>=w.source_count()||!archive||!hash)return 1;
    const auto&m=w.metadata(i);const auto*d=m.definition;
    if(!d||std::strcmp(d->class_name,"RigidBody")||!m.asset_present||*w.active_descriptor()!=&m||!rotation(d->transform))return 1;
    std::string name=lower(d->asset_name);
    const bool cardboard=name=="assets_scene_items\\01-01\\sih_carboard_box\\sih_cardboard_box.amf";
    const bool canister=name=="assets_scene_items\\generic_havok\\sih_gascannister\\sih_gascannister.mtxt";
    if(!cardboard&&!canister)return 1;
    const uint8_t*cursor=*w.property_cursor(i);UCRigidPropParameters p;
    if(uc_rigid_prop_parameters(d,&cursor,&p)||p.max_life!=0||p.respawn_time!=0||p.explode_on_contact||p.explode_contact!=-1)return 1;
    if((p.point_particle&&*p.point_particle)||(p.shot_particle&&*p.shot_particle&&std::strcmp(p.shot_particle,"redgasfire.simpleparticlesystem")))return 1;
    const float scalars[]={p.mass,p.linear_damping,p.angular_damping,p.friction,p.restitution,p.temperature};
    for(float f:scalars)if(!std::isfinite(f)||f<0||f>10000)return 1;
    if(p.mass<=0||p.friction>1||p.restitution>1)return 1;
    for(unsigned k=0;k<3;k++)if(p.velocity[k]!=0||p.angular_velocity[k]!=0)return 1;
    const auto dot=name.find_last_of('.');std::string collision=name.substr(0,dot)+(canister?".mtxthk":".mdlhk");
    BytesOwner bytes;auto at=find_resource(archive,collision.c_str());UCModelCollisionAsset asset{};
    if(at==SIZE_MAX||uc_archive_read(archive,at,UC_AR_LOCAL_PAYLOAD,&bytes.value)!=UC_AR_OK||uc_model_collision_load(bytes.value.data,bytes.value.size,canister,&asset))return 1;
    int unsupported=0;groups_=asset.group_count;for(uint32_t n=0;n<asset.shape_count;n++){UCModelCollisionShape converted;if(asset.shapes[n].object!=0||uc_rigid_prop_box(&asset.shapes[n],&converted)){unsupported=1;break;}all_shapes_.push_back(converted);if(converted.group==0)local_.push_back(converted);}
    uc_model_collision_free(&asset);if(unsupported||local_.empty()){local_.clear();return 1;}
    try{model_.load(r,archive,hash,catalog,d->asset_name,nullptr,nullptr,nullptr,0);}
    catch(...){local_.clear();return 1;}
    for(const auto&material:model_.materials)if(!material.supported()){local_.clear();return 1;}
    parameters_=p;scenery_=&scenery;delta_=w.script_services().delta;world_=&w;renderer_=&r;archive_=archive;std::memcpy(archive_hash_,hash,32);catalog_=catalog;asset_=d->asset_name;
    uc_rigid_prop_damage_begin(&damage_,&parameters_);
    if(!delta_||uc_entity_fresh_construct_authored(&w.context(),&entity.view,w.active_descriptor()))return failure_=2;
    *w.property_cursor(i)=cursor;
    entity.model=&model_;entity.flags|=0x2000; //1363A0 final class flag; PC model publication.
    if(entity.script.attach_actor(entity.view,m,w.script_services()))return failure_=2;
    if(uc_entity_subscriptions_create(this,&subscriptions_))return failure_=2;
    int inserted=0;if(uc_entity_listener_register(&w.registry(),entity.id,entity.id,uc_entity_subscriptions_listener(subscriptions_),w.listeners(),&inserted)||!inserted)return failure_=2;
    UCEntityEventSubscription hit{0xbf,[](void*p,const void*payload,uint32_t*){return payload?static_cast<NativeRigidProp*>(p)->event_hit(*(const int32_t*)payload):1;}};
    if(uc_entity_subscriptions_append(subscriptions_,&hit))return failure_=2;
    float identity[16]{};for(unsigned n=0;n<4;n++)identity[n*5]=1;
    try{model_.bind_pose(r,identity);}catch(...){return failure_=2;}
    boxes_.resize(local_.size());ready_=true;sleeping_=p.unmovable!=0;refresh_boxes();
    if(services_.physics){
        const XMMATRIX swap=XMMatrixSet(1,0,0,0,0,0,1,0,0,1,0,0,0,0,0,1);
        XMMATRIX world=swap*matrix(entity.transform)*swap;world.r[3]=XMVectorMultiply(world.r[3],XMVectorSet(.01f,.01f,.01f,1));
        XMFLOAT4X4 pose;XMStoreFloat4x4(&pose,world);physics_body_=services_.physics->create_prop(entity.id,local_.data(),local_.size(),&pose._11,parameters_);
    }
    return 0;
}
void NativeRigidProp::sync_physics(){
    if(!physics_body_)return;
    XMFLOAT4X4 pose;physics_body_->sample(&pose._11,velocity_,sleeping_);
    const XMMATRIX swap=XMMatrixSet(1,0,0,0,0,0,1,0,0,1,0,0,0,0,0,1);
    XMMATRIX world=swap*XMLoadFloat4x4(&pose)*swap;world.r[3]=XMVectorMultiply(world.r[3],XMVectorSet(100,100,100,1));
    XMStoreFloat4x4((XMFLOAT4X4*)entity.transform,world);refresh_boxes();
}
void NativeRigidProp::refresh_boxes(){
    const XMMATRIX swap=XMMatrixSet(1,0,0,0,0,0,1,0,0,1,0,0,0,0,0,1);
    XMMATRIX world=swap*matrix(entity.transform)*swap;world.r[3]=XMVectorMultiply(world.r[3],XMVectorSet(.01f,.01f,.01f,1));
    for(size_t i=0;i<local_.size();i++){
        auto&b=boxes_[i];const auto&s=local_[i];XMMATRIX composed=matrix(s.transform)*world;
        XMStoreFloat3((XMFLOAT3*)b.center,composed.r[3]);XMStoreFloat4((XMFLOAT4*)b.rotation,XMQuaternionNormalize(XMQuaternionRotationMatrix(composed)));
        std::memcpy(b.half,s.dimensions,12);b.radius=s.radius;
    }
}
int NativeRigidProp::base_update(){
    if(!ready_||failure_)return failure_?failure_:1;
    const float dt=*delta_;if(!std::isfinite(dt)||dt<0||dt>.1f)return failure_=2;
    if(physics_body_){sync_physics();return entity.script.update()?failure_=2:0;}
    if(!sleeping_&&dt>0){
        float previous[3];uc_collision_to_physics(entity.transform+12,previous);bool touched=false;
        //Real initial overlap against the authored mesh radius. Corner spheres
        //remain an explicit PC contact approximation, not an OBB manifold.
        for(unsigned iteration=0;iteration<4;iteration++){
            UCCollisionContact deepest{};
            for(const auto&b:boxes_){XMMATRIX basis=XMMatrixRotationQuaternion(XMLoadFloat4((const XMFLOAT4*)b.rotation));
                for(unsigned corner=0;corner<8;corner++){
                    XMVECTOR v=XMVectorSet((corner&1?1.f:-1.f)*b.half[0],(corner&2?1.f:-1.f)*b.half[1],(corner&4?1.f:-1.f)*b.half[2],0);
                    XMFLOAT3 offset;XMStoreFloat3(&offset,XMVector3TransformNormal(v,basis));float point[3]={b.center[0]+offset.x,b.center[1]+offset.y,b.center[2]+offset.z};UCCollisionContact contacts[64];
                    size_t count=uc_collision_capsule_contacts(scenery_,point,point,b.radius,contacts,64);if(count>64)return failure_=2;
                    for(size_t n=0;n<count;n++)if(contacts[n].penetration>deepest.penetration)deepest=contacts[n];
                }
            }
            if(deepest.penetration<=.0001f)break;
            touched|=deepest.normal[2]>.7f;
            float t[3];uc_collision_to_physics(entity.transform+12,t);for(unsigned k=0;k<3;k++)t[k]+=deepest.normal[k]*(deepest.penetration+.0001f);uc_collision_to_render(t,entity.transform+12);refresh_boxes();
        }
        velocity_[2]-=17.f*dt; //Explicit same-preview gravity policy, not Havok integration.
        float displacement[3];for(unsigned k=0;k<3;k++){velocity_[k]*=std::max(0.f,1.f-parameters_.linear_damping*dt);displacement[k]=velocity_[k]*dt;}
        float translation[3];uc_collision_to_physics(entity.transform+12,translation);
        //ponytail: translational corner sweeps, no invented angular/contact solver.
        for(unsigned plane=0;plane<4;plane++){
         float fraction=1;UCCollisionRayHit nearest{};bool hit=false;
         for(const auto&b:boxes_){XMMATRIX basis=XMMatrixRotationQuaternion(XMLoadFloat4((const XMFLOAT4*)b.rotation));
            for(unsigned corner=0;corner<8;corner++){
                const float padding=b.radius+scenery_->radius;
                XMVECTOR v=XMVectorSet((corner&1?1.f:-1.f)*(b.half[0]+padding),(corner&2?1.f:-1.f)*(b.half[1]+padding),(corner&4?1.f:-1.f)*(b.half[2]+padding),0);
                XMFLOAT3 offset;XMStoreFloat3(&offset,XMVector3TransformNormal(v,basis));float from[3]={b.center[0]+offset.x,b.center[1]+offset.y,b.center[2]+offset.z},to[3];for(unsigned k=0;k<3;k++)to[k]=from[k]+displacement[k];
                UCCollisionRayHit q;int result=uc_collision_raycast(scenery_,from,to,&q);if(result<0)return failure_=2;
                if(result&&q.fraction<fraction){nearest=q;fraction=q.fraction;hit=true;}
            }
         }
         for(unsigned k=0;k<3;k++)translation[k]+=displacement[k]*fraction;
         if(hit){touched|=nearest.normal[2]>.7f;float normal_speed=0,inward=0;for(unsigned k=0;k<3;k++){normal_speed+=velocity_[k]*nearest.normal[k];displacement[k]*=1-fraction;inward+=displacement[k]*nearest.normal[k];}
            for(unsigned k=0;k<3;k++){translation[k]+=nearest.normal[k]*.0001f;if(normal_speed<0)velocity_[k]-=(1+parameters_.restitution)*normal_speed*nearest.normal[k];if(inward<0)displacement[k]-=inward*nearest.normal[k];}
            if(nearest.normal[2]>.7f){velocity_[0]*=1-parameters_.friction;velocity_[1]*=1-parameters_.friction;float speed=0;for(float v:velocity_)speed+=v*v;if(speed<.04f){std::fill(std::begin(velocity_),std::end(velocity_),0.f);sleeping_=true;}}
         }
         uc_collision_to_render(translation,entity.transform+12);refresh_boxes();
         float remaining=0;for(float d:displacement)remaining+=d*d;if(!hit||remaining<1e-12f||sleeping_)break;
        }
        //PC contact stabilization: source assets can begin across a mesh edge.
        //Sleep only after a quarter-second of upward support with <1mm motion.
        float moved=0;for(unsigned k=0;k<3;k++){float d=translation[k]-previous[k];moved+=d*d;}
        resting_time_=touched&&moved<1e-6f?resting_time_+dt:0;
        if(resting_time_>=.25f){std::fill(std::begin(velocity_),std::end(velocity_),0.f);sleeping_=true;}
        uc_collision_to_render(translation,entity.transform+12);refresh_boxes();
    }
    if(entity.script.update())return failure_=2;return 0;
}
UCRigidPropDamageServices NativeRigidProp::damage_services(){UCRigidPropDamageServices h{};h.owner=this;h.id=entity.id;h.position=entity.transform+12;h.delta=delta_?*delta_:0;
    h.base_update=[](void*p){return static_cast<NativeRigidProp*>(p)->base_update();};
    h.body=[](void*p,uint8_t*out){*out=!static_cast<NativeRigidProp*>(p)->boxes_.empty();return 0;};
    //Explicit translation-only PC motion adapter. Sleeping is not source body
    //type6; only authored unmovable chooses that static-motion category.
    h.motion=[](void*p,int32_t*type,float*out){auto&s=*static_cast<NativeRigidProp*>(p);*type=s.parameters_.unmovable?6:0;std::memcpy(out,s.velocity_,12);return 0;};
    h.fixed=[](void*p){auto&s=*static_cast<NativeRigidProp*>(p);if(s.physics_body_)s.physics_body_->sleep();std::fill(std::begin(s.velocity_),std::end(s.velocity_),0.f);s.sleeping_=true;return 0;};
    h.wake=[](void*p){auto&s=*static_cast<NativeRigidProp*>(p);if(s.physics_body_)s.physics_body_->wake();if(!s.parameters_.unmovable)s.sleeping_=false;s.resting_time_=0;return 0;};
    //The PC body has no Havok property21/22 producer. Both are genuinely absent
    //until a concrete damage owner publishes them;1354A0 returns zero if absent.
    h.value=[](void*p,uint32_t key,uint32_t*out){auto&s=*static_cast<NativeRigidProp*>(p);if(key==0x15)*out=s.contacts_;else if(key==0x16)std::memcpy(out,&s.blast_,4);else return 1;return 0;};
    h.particle=[](void*p,const char*a,const char*b,uint8_t hidden){auto&s=*static_cast<NativeRigidProp*>(p);return s.services_.particle?s.services_.particle(s.services_.owner,s,a,b,hidden):1;};
    h.resolve_parent=[](void*p,int32_t id,int32_t*parent,uint8_t*found){auto&s=*static_cast<NativeRigidProp*>(p);auto*t=uc_entity_event_lookup(&s.world_->registry(),id);*found=t!=nullptr;if(!t)return 0;
        auto&c=s.world_->context();auto*v=c.resolve(c.owner,t);if(!v||!v->target.parent_id)return 1;*parent=*v->target.parent_id;return 0;};
    h.flags=[](void*p,uint32_t flags){auto&s=*static_cast<NativeRigidProp*>(p);s.entity.flags|=flags;return 0;};
    h.explosion=[](void*p,const UCRigidPropExplosion*x){auto&s=*static_cast<NativeRigidProp*>(p);return s.services_.explosion?s.services_.explosion(s.services_.owner,s,*x):1;};
    h.groups=[](void*p,int32_t*out){*out=(int32_t)static_cast<NativeRigidProp*>(p)->groups_;return 0;};
    h.group_one=[](void*p){return static_cast<NativeRigidProp*>(p)->group_one();};
    h.sound=[](void*p,const char*name){auto&s=*static_cast<NativeRigidProp*>(p);return s.services_.sound?s.services_.sound(s.services_.owner,s,name):1;};
    h.destroy=[](void*){return 1;}; //max_life/respawn not admitted by construct.
    return h;
}
int NativeRigidProp::group_one(){
    std::vector<UCModelCollisionShape> selected;for(const auto&s:all_shapes_)if(s.group==1)selected.push_back(s);if(selected.empty())return 1;
    try{destroyed_model_=std::make_unique<NativeAnimatedModel>();destroyed_model_->load(*renderer_,archive_,archive_hash_,catalog_,asset_.c_str(),nullptr,nullptr,nullptr,1);for(const auto&m:destroyed_model_->materials)if(!m.supported())return 1;
        float identity[16]{};for(unsigned i=0;i<4;i++)identity[i*5]=1;destroyed_model_->bind_pose(*renderer_,identity);entity.model=destroyed_model_.get();}catch(...){return 1;}
    if(physics_body_)physics_body_->shape(selected.data(),selected.size());
    local_=std::move(selected);boxes_.resize(local_.size());refresh_boxes();if(!parameters_.unmovable)sleeping_=false;resting_time_=0;return 0;
}
int NativeRigidProp::update(){if(!ready_||failure_)return failure_?failure_:1;auto h=damage_services();return uc_rigid_prop_damage_step(&damage_,&parameters_,&h)?failure_=2:0;}
int NativeRigidProp::event_hit(int32_t id){if(!ready_||failure_)return 1;auto h=damage_services();return uc_rigid_prop_damage_event(&damage_,&parameters_,&h,id)?failure_=2:0;}
int NativeRigidProp::bullet_contact(int32_t id){if(!ready_||failure_)return 1;auto h=damage_services();return uc_rigid_prop_damage_bullet(&damage_,&parameters_,&h,id)?failure_=2:0;}
int32_t NativeRigidProp::bone_index(const char*n)const{const auto&m=destroyed_model_?*destroyed_model_:model_;if(!n)return -1;for(uint32_t i=0;i<m.source.bone_count;i++)if(!_strnicmp((const char*)m.source.bones[i].source_metadata+4,n,16))return (int32_t)i;return -1;}
bool NativeRigidProp::posed_bone(int32_t i,float*out)const{const auto&m=destroyed_model_?*destroyed_model_:model_;if(!out||i<0||uint32_t(i)>=m.source.bone_count||!m.prepared.ready||!m.prepared.world_valid[i])return false;std::memcpy(out,m.prepared.world[i],64);return true;}
int NativeRigidProp::late(){return ready_&&!failure_?0:failure_?failure_:1;}
int NativeRigidProp::impulse(const float*p){if(!ready_||failure_||!p||parameters_.unmovable)return 1;for(unsigned k=0;k<3;k++)if(!std::isfinite(p[k])||std::fabs(p[k])>10000)return 1;if(physics_body_){physics_body_->impulse(p);sync_physics();}else for(unsigned k=0;k<3;k++)velocity_[k]+=p[k]/parameters_.mass;sleeping_=false;resting_time_=0;return 0;}
void NativeRigidProp::draw(NativeRenderer&r,const float*vp,float exposure,NativeModelLighting *lighting,const NativeLightingFrame *frame){if(!ready_||failure_)return;XMFLOAT4X4 m;XMStoreFloat4x4(&m,matrix(entity.transform)*matrix(vp));const float tint[4]={1,1,1,1};auto &model=destroyed_model_?*destroyed_model_:model_;if(lighting&&frame)model.draw_lit(r,*lighting,*frame,entity.transform,&m._11,tint,nullptr,exposure);else model.draw(r,&m._11,tint,nullptr,exposure);}
void NativeRigidProp::collect_shadows(NativeRenderer&r,NativeModelLighting&lighting,const NativeLightingFrame&frame,NativeModelShadowCasters&casters,float){
    if(!ready_||failure_)return;auto&model=destroyed_model_?*destroyed_model_:model_;if(!NativeModelShadowCasters::eligible(model,entity.secondary))return;model.present(r,1);
    casters.add(model,entity.transform,entity.secondary,lighting.prepare_membership(model,frame,entity.transform),frame);
}
int NativeRigidProp::retire(){
    if(!ready_||!world_||failure_||entity.helper||entity.actor.handleBC||!entity.aux)return 1;
    if((entity.flags&0x1000)&&(entity.source<0||uint32_t(entity.source)>=world_->source_count()))return 1;
    //4696F8+24 ->3D710 ->218B30. No source inter-body constraint list exists
    //in this adapter; the shared solver owns its real contact relationships.
    uc_mission_command_destroy(nullptr,entity.actor.audioC0);entity.actor.audioC0=nullptr;
    if(uc_entity_fresh_aux_clear(entity.aux))return failure_=2;
    entity.flags|=0x8100;uint8_t queued=0;
    if(uc_entity_retire(&world_->retirement(),entity.id,&queued)||!queued)return failure_=2;
    if(entity.flags&0x1000){auto*f=world_->slot(entity.source).flags;*f=(*f&~15u)|16u;}
    entity.flags|=0x80000;return 0;
}
int NativeRigidProp::close_detached(){if(entity.script.close_detached())return 1;if(world_&&entity.identity.listeners[0]){if(uc_entity_listener_list_remove_first(world_->listeners(),entity.id))return 1;for(auto&l:entity.identity.listeners)l=nullptr;}
    physics_body_.reset();uc_entity_subscriptions_close(subscriptions_);subscriptions_=nullptr;entity.model=nullptr;destroyed_model_.reset();boxes_.clear();local_.clear();all_shapes_.clear();ready_=false;scenery_=nullptr;delta_=nullptr;world_=nullptr;
    if(entity.aux&&uc_entity_fresh_aux_destroy(entity.aux))return 1;entity.aux=nullptr;return 0;}

int NativeRigidProp::player_contact(const UCPlayer&player,const UCPlayerInput&input){
    if(!ready_||failure_)return 1;
    if(parameters_.unmovable||(!input.forward&&!input.right))return 0;
    float length=std::max(1.f,std::hypot(input.forward,input.right));
    float s=std::sin(input.yaw),c=std::cos(input.yaw);
    const float desired[2]={(-s*input.forward+c*input.right)*input.speed/length,
        (c*input.forward+s*input.right)*input.speed/length};
    float low[3]={player.position[0],player.position[1],player.position[2]+UC_PLAYER_CAPSULE_BOTTOM};
    float high[3]={low[0],low[1],player.position[2]+(player.crouched?UC_PLAYER_CAPSULE_CROUCH_TOP:UC_PLAYER_CAPSULE_STANDING_TOP)};
    float best=0,momentum[3]{};
    for(const auto&box:boxes_){UCCollisionContact hit{};
        int contact=uc_collision_block_contact(&box,low,high,UC_PLAYER_CAPSULE_RADIUS+.002f,&hit);
        if(contact<0)return 1;if(!contact||std::fabs(hit.normal[2])>.5f)continue;
        float n=std::hypot(hit.normal[0],hit.normal[1]);if(n<.001f)continue;
        float x=hit.normal[0]/n,y=hit.normal[1]/n;
        float closing=-((desired[0]-velocity_[0])*x+(desired[1]-velocity_[1])*y);
        if(closing<=best)continue;best=closing;
        // ponytail: finite 80kg PC player coupling, linear-only. Original Havok
        // contact mass/torque requires a separate parity recovery.
        float j=closing*(80.f*parameters_.mass/(80.f+parameters_.mass));
        momentum[0]=-x*j;momentum[1]=-y*j;
    }
    return best>0?impulse(momentum):0;
}
int NativeRigidProp::bullet_impulse(const float momentum[3]){
    if(!ready_||failure_)return 1;
    if(parameters_.unmovable)return 0;
    //2A6FF0 speed gate; the existing PC body owns linear motion, not torque.
    float square=(velocity_[2]*velocity_[2]+velocity_[1]*velocity_[1])+velocity_[0]*velocity_[0];
    return square<83.33333587646484f?impulse(momentum):0;
}
