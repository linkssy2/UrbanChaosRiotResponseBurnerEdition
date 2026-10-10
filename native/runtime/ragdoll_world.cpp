#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemSingleThreaded.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayerInterfaceTable.h>
#include <Jolt/Physics/Collision/BroadPhase/ObjectVsBroadPhaseLayerFilterTable.h>
#include <Jolt/Physics/Collision/ObjectLayerPairFilterTable.h>
#include <Jolt/Physics/Collision/GroupFilterTable.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/OffsetCenterOfMassShape.h>
#include <Jolt/Physics/Constraints/HingeConstraint.h>
#include <Jolt/Physics/Constraints/SwingTwistConstraint.h>
#include "ragdoll_world.h"
#include "character_navigation.h"
#include <map>
#include <set>
#include <array>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace {
using namespace JPH;
void need(bool value,const char *message){if(!value)throw std::runtime_error(message);}
Vec3 vector(const float *p){need(p&&std::isfinite(p[0])&&std::isfinite(p[1])&&std::isfinite(p[2]),"Invalid ragdoll vector");return Vec3(p[0],p[1],p[2]);}
struct Library {
    Library(){RegisterDefaultAllocator();need(!Factory::sInstance,"Native physics factory already exists");Factory::sInstance=new Factory;RegisterTypes();}
    ~Library(){UnregisterTypes();delete Factory::sInstance;Factory::sInstance=nullptr;}
};
Library &library(){static Library value;return value;}
Mat44 matrix(const float *p){
    const Vec3 x=vector(p),y=vector(p+4),z=vector(p+8);
    need(std::abs(x.LengthSq()-1)<.02f&&std::abs(y.LengthSq()-1)<.02f&&std::abs(z.LengthSq()-1)<.02f&&
        std::abs(x.Dot(y))<.02f&&std::abs(x.Dot(z))<.02f&&std::abs(y.Dot(z))<.02f&&x.Cross(y).Dot(z)>.98f,"Invalid ragdoll body orientation");
    return Mat44(Vec4(x,0),Vec4(y,0),Vec4(z,0),Vec4(vector(p+12),1));
}
RefConst<Shape> capsule(const UCRagdollBody &b){
    const Vec3 a=vector(b.capsule_a),end=vector(b.capsule_b),middle=(a+end)*.5f,axis=end-a;
    need(std::isfinite(b.radius)&&b.radius>0&&axis.LengthSq()>0,"Invalid ragdoll capsule");
    RefConst<Shape> shape=new CapsuleShape(axis.Length()*.5f,b.radius);
    shape=new RotatedTranslatedShape(middle,Quat::sFromTo(Vec3::sAxisY(),axis.Normalized()),shape);
    const Vec3 offset=vector(b.center_of_mass)-shape->GetCenterOfMass();
    if(offset.LengthSq()>0)shape=new OffsetCenterOfMassShape(shape,offset);
    return shape;
}
RefConst<Shape> box_shape(const UCSolidBlockGeometry &g){
    const Vec3 half=vector(g.half)+Vec3::sReplicate(g.radius);
    need(g.radius>=0&&half.GetX()>0&&half.GetY()>0&&half.GetZ()>0,"Invalid ragdoll world box");
    return new BoxShape(half,g.radius);
}
Quat box_rotation(const UCSolidBlockGeometry &g){
    Quat q(g.rotation[0],g.rotation[1],g.rotation[2],g.rotation[3]);
    need(!q.IsNaN()&&std::abs(q.LengthSq()-1)<.02f,"Invalid ragdoll world box rotation");return q.Normalized();
}
}

struct NativeRagdollWorld::State {
    Library &runtime=library();
    BroadPhaseLayerInterfaceTable broad{2,2};
    ObjectLayerPairFilterTable pairs{2};
    std::unique_ptr<ObjectVsBroadPhaseLayerFilterTable> filter;
    PhysicsSystem physics;
    TempAllocatorImpl temporary{16*1024*1024};
    JobSystemSingleThreaded jobs{1024};
    struct Box {BodyID id;UCSolidBlockGeometry geometry;uint64_t seen;};
    std::map<std::pair<int32_t,const void*>,Box> boxes;
    std::set<int32_t> prop_entities;
    uint32_t dynamic_props=0;
    uint64_t generation=0;uint32_t group=0;
    State(){
        broad.MapObjectToBroadPhaseLayer(0,BroadPhaseLayer(0));broad.MapObjectToBroadPhaseLayer(1,BroadPhaseLayer(1));
        pairs.EnableCollision(0,1);pairs.EnableCollision(1,1);
        filter=std::make_unique<ObjectVsBroadPhaseLayerFilterTable>(broad,2,pairs,2);
        physics.Init(8192,0,32768,8192,broad,*filter,pairs);
    }
    BodyID add(const Shape *shape,Vec3 position,Quat rotation){
        BodyCreationSettings settings(shape,position,rotation,EMotionType::Static,0);
        auto id=physics.GetBodyInterface().CreateAndAddBody(settings,EActivation::DontActivate);
        need(!id.IsInvalid(),"Native ragdoll scenery capacity exceeded");return id;
    }
    void remove(BodyID id){auto &b=physics.GetBodyInterface();if(b.IsAdded(id))b.RemoveBody(id);b.DestroyBody(id);}
    ~State(){BodyIDVector ids;physics.GetBodies(ids);for(auto id:ids)remove(id);}
};
struct NativeRagdoll::State {
    std::shared_ptr<NativeRagdollWorld::State> world;
    std::vector<BodyID> bodies;
    struct Joint {Ref<TwoBodyConstraint> constraint;unsigned a,b;Vec3 pivot_a,pivot_b;};
    std::vector<Joint> joints;
    ~State(){for(auto &j:joints)world->physics.RemoveConstraint(j.constraint);joints.clear();for(auto id:bodies)world->remove(id);}
};
struct NativePropBody::State {
    std::shared_ptr<NativeRagdollWorld::State> world;
    BodyID body;int32_t entity=-1;float mass=0;bool movable=false,registered=false;
    ~State(){if(!body.IsInvalid())world->remove(body);if(registered){world->prop_entities.erase(entity);if(movable)--world->dynamic_props;}}
};
static RefConst<Shape> prop_shape(const UCModelCollisionShape*shapes,size_t count){
    need(shapes&&count&&count<=128,"Invalid native prop shape count");StaticCompoundShapeSettings compound;
    for(size_t i=0;i<count;i++){
        const auto&s=shapes[i];need(s.kind==1&&s.family==2,"Unsupported native prop shape");
        UCSolidBlockGeometry box{};std::memcpy(box.half,s.dimensions,12);box.radius=s.radius;
        const auto local=matrix(s.transform);auto shape=box_shape(box);
        compound.AddShape(local.GetTranslation(),local.GetQuaternion().Normalized(),shape);
    }
    auto result=compound.Create();need(!result.HasError(),"Native prop compound construction failed");return result.Get();
}
std::unique_ptr<NativePropBody> NativeRagdollWorld::create_prop(int32_t entity,const UCModelCollisionShape*shapes,size_t count,const float transform[16],const UCRigidPropParameters&p){
    need(entity>=0&&!state_->prop_entities.count(entity),"Duplicate native prop body");
    need(std::isfinite(p.mass)&&p.mass>0&&std::isfinite(p.friction)&&p.friction>=0&&p.friction<=1&&std::isfinite(p.restitution)&&p.restitution>=0&&p.restitution<=1&&std::isfinite(p.linear_damping)&&p.linear_damping>=0&&std::isfinite(p.angular_damping)&&p.angular_damping>=0,"Invalid native prop parameters");
    auto shape=prop_shape(shapes,count);auto pose=matrix(transform);auto s=std::make_unique<NativePropBody::State>();s->world=state_;s->entity=entity;s->mass=p.mass;s->movable=!p.unmovable;
    BodyCreationSettings settings(shape,pose.GetTranslation(),pose.GetQuaternion().Normalized(),s->movable?EMotionType::Dynamic:EMotionType::Static,s->movable?1:0);
    settings.mFriction=p.friction;settings.mRestitution=p.restitution;settings.mLinearDamping=p.linear_damping;settings.mAngularDamping=p.angular_damping;
    settings.mMotionQuality=EMotionQuality::LinearCast;
    settings.mOverrideMassProperties=EOverrideMassProperties::CalculateInertia;settings.mMassPropertiesOverride.mMass=p.mass;
    s->body=state_->physics.GetBodyInterface().CreateAndAddBody(settings,s->movable?EActivation::Activate:EActivation::DontActivate);
    need(!s->body.IsInvalid(),"Native prop body capacity exceeded");
    state_->prop_entities.insert(entity);s->registered=true;if(s->movable)++state_->dynamic_props;
    return std::unique_ptr<NativePropBody>(new NativePropBody(std::move(s)));
}
NativePropBody::NativePropBody(std::unique_ptr<State>s):state_(std::move(s)){}
NativePropBody::~NativePropBody()=default;
void NativePropBody::sample(float out[16],float speed[3],bool&sleeping)const{
    auto&bi=state_->world->physics.GetBodyInterface();auto m=bi.GetWorldTransform(state_->body);
    for(unsigned i=0;i<4;i++)m.GetColumn4(i).StoreFloat4(reinterpret_cast<Float4*>(out+4*i));
    bi.GetLinearVelocity(state_->body).StoreFloat3(reinterpret_cast<Float3*>(speed));sleeping=!bi.IsActive(state_->body);
}
void NativePropBody::impulse(const float*p){need(state_->movable,"Cannot impulse a static prop");state_->world->physics.GetBodyInterface().AddImpulse(state_->body,vector(p));}
void NativePropBody::sleep(){if(state_->movable){auto&bi=state_->world->physics.GetBodyInterface();bi.SetLinearAndAngularVelocity(state_->body,Vec3::sZero(),Vec3::sZero());bi.DeactivateBody(state_->body);}}
void NativePropBody::wake(){if(state_->movable)state_->world->physics.GetBodyInterface().ActivateBody(state_->body);}
void NativePropBody::shape(const UCModelCollisionShape*p,size_t n){
    auto shape=prop_shape(p,n);auto&world=state_->world->physics;
    world.GetBodyInterface().SetShape(state_->body,shape,true,state_->movable?EActivation::Activate:EActivation::DontActivate);
    if(state_->movable){BodyLockWrite lock(world.GetBodyLockInterface(),state_->body);need(lock.Succeeded(),"Native prop shape mass update failed");lock.GetBody().GetMotionProperties()->ScaleToMass(state_->mass);}
}

NativeRagdollWorld::NativeRagdollWorld(const UCCollisionAsset &asset,const float gravity[3]):state_(std::make_shared<State>()){
    state_->physics.SetGravity(vector(gravity));
    need(!asset.triangle_count||(asset.triangles&&asset.vertices),"Missing native ragdoll scenery");
    if(asset.triangle_count){
        VertexList vertices;vertices.reserve(asset.vertex_count);
        const Vec3 scale=vector(asset.scaling);
        for(unsigned i=0;i<asset.vertex_count;++i){Float3 value;(vector(asset.vertices[i])*scale).StoreFloat3(&value);vertices.push_back(value);}
        IndexedTriangleList triangles;triangles.reserve(size_t(asset.triangle_count)*2);
        for(unsigned i=0;i<asset.triangle_count;++i){auto &t=asset.triangles[i];
            need(t.vertex[0]<asset.vertex_count&&t.vertex[1]<asset.vertex_count&&t.vertex[2]<asset.vertex_count,"Invalid ragdoll scenery index");
            triangles.emplace_back(t.vertex[0],t.vertex[1],t.vertex[2]);
            // Original scenery queries admit both sides; Jolt simulation meshes
            // are single sided. Preserve that coverage in this shared mesh.
            triangles.emplace_back(t.vertex[2],t.vertex[1],t.vertex[0]);
        }
        MeshShapeSettings settings(std::move(vertices),std::move(triangles));auto result=settings.Create();
        need(result.IsValid(),"Native ragdoll scenery construction failed");
        state_->add(result.Get(),Vec3::sZero(),Quat::sIdentity());state_->physics.OptimizeBroadPhase();
    }
}
NativeRagdollWorld::~NativeRagdollWorld()=default;
void NativeRagdollWorld::sync_collision(const NativeCharacterWorld &world){
    auto &s=*state_;auto &bi=s.physics.GetBodyInterface();++s.generation;
    for(const auto &box:world.boxes){
        if(s.prop_entities.count(box.entity))continue; //Already a real body: never mirror it as a static obstacle.
        need(box.geometry!=nullptr,"Missing ragdoll world box geometry");const auto &g=*box.geometry;
        auto key=std::make_pair(box.entity,static_cast<const void*>(box.geometry));auto found=s.boxes.find(key);
        if(found==s.boxes.end()){
            auto shape=box_shape(g);auto id=s.add(shape,vector(g.center),box_rotation(g));
            try{s.boxes.emplace(key,State::Box{id,g,s.generation});}catch(...){s.remove(id);throw;}
        }else{
            auto &b=found->second;b.seen=s.generation;
            if(std::memcmp(b.geometry.half,g.half,sizeof(g.half))||b.geometry.radius!=g.radius){auto shape=box_shape(g);bi.SetShape(b.id,shape,false,EActivation::Activate);}
            if(std::memcmp(b.geometry.center,g.center,sizeof(g.center))||std::memcmp(b.geometry.rotation,g.rotation,sizeof(g.rotation)))bi.SetPositionAndRotation(b.id,vector(g.center),box_rotation(g),EActivation::Activate);
            b.geometry=g;
        }
    }
    for(auto i=s.boxes.begin();i!=s.boxes.end();)if(i->second.seen!=s.generation){s.remove(i->second.id);i=s.boxes.erase(i);}else ++i;
}
void NativeRagdollWorld::step(float dt){
    need(std::isfinite(dt)&&dt>=0&&dt<=.1f,"Invalid native ragdoll simulation step");if(dt==0)return;
    // Two substeps at the game's30Hz tick keep the authored small capsules
    // stable without duplicating the world's update for every corpse.
    const auto error=state_->physics.Update(dt,std::max(1,int(std::ceil(dt*60.f))),&state_->temporary,&state_->jobs);
    need(error==EPhysicsUpdateError::None,"Native ragdoll simulation capacity exceeded");
}
uint32_t NativeRagdollWorld::body_count()const{return state_->physics.GetNumBodies();}
uint32_t NativeRagdollWorld::ragdoll_body_count()const{
    // Props share the solver but are not corpse bodies.
    return state_->physics.GetBodyStats().mNumBodiesDynamic-static_cast<uint32_t>(state_->dynamic_props);
}
std::unique_ptr<NativeRagdoll> NativeRagdollWorld::create(const UCRagdollAsset &asset,const float (*poses)[16]){
    need(asset.bodies&&asset.joints&&poses&&asset.body_count>1&&asset.body_count<=64&&asset.joint_count==asset.body_count-1,"Invalid native ragdoll asset");
    auto state=std::make_unique<NativeRagdoll::State>();state->world=state_;state->bodies.reserve(asset.body_count);state->joints.reserve(asset.joint_count);
    Ref<GroupFilterTable> group=new GroupFilterTable(asset.body_count);
    for(unsigned i=0;i<asset.joint_count;++i){const auto &j=asset.joints[i];need(j.body[0]<asset.body_count&&j.body[1]<asset.body_count&&j.body[0]!=j.body[1],"Invalid native ragdoll joint bodies");group->DisableCollision(j.body[0],j.body[1]);}
    auto &bi=state_->physics.GetBodyInterface();const auto group_id=++state_->group;
    std::vector<Body*> bodies;bodies.reserve(asset.body_count);
    for(unsigned i=0;i<asset.body_count;++i){const auto &b=asset.bodies[i];auto pose=matrix(poses[i]);auto shape=capsule(b);
        BodyCreationSettings settings(shape,pose.GetTranslation(),pose.GetQuaternion().Normalized(),EMotionType::Dynamic,1);
        settings.mCollisionGroup=CollisionGroup(group,group_id,i);settings.mMotionQuality=EMotionQuality::LinearCast;
        need(std::isfinite(b.mass_inv)&&b.mass_inv>0,"Invalid native ragdoll mass");
        settings.mOverrideMassProperties=EOverrideMassProperties::MassAndInertiaProvided;settings.mMassPropertiesOverride.mMass=1/b.mass_inv;
        const Vec3 inverse=vector(b.inertia_and_mass_inv);need(inverse.GetX()>0&&inverse.GetY()>0&&inverse.GetZ()>0,"Invalid native ragdoll inertia");
        settings.mMassPropertiesOverride.mInertia=Mat44::sScale(Vec3::sReplicate(1)/inverse);
        settings.mLinearDamping=b.linear_damping;settings.mAngularDamping=b.angular_damping;
        settings.mMaxLinearVelocity=b.max_linear_velocity;settings.mMaxAngularVelocity=b.max_angular_velocity;
        settings.mFriction=b.friction;settings.mRestitution=b.restitution;
        settings.mLinearVelocity=vector(b.linear_velocity);settings.mAngularVelocity=vector(b.angular_velocity);
        Body *body=bi.CreateBody(settings);need(body!=nullptr,"Native ragdoll body capacity exceeded");
        state->bodies.push_back(body->GetID());bodies.push_back(body);
    }
    for(unsigned i=0;i<asset.joint_count;++i){const auto &j=asset.joints[i];
        const Vec3 a=vector(j.basis[0])-vector(asset.bodies[j.body[0]].center_of_mass),b=vector(j.basis[1])-vector(asset.bodies[j.body[1]].center_of_mass);
        Ref<TwoBodyConstraint> constraint;
        if(j.kind==2){
            HingeConstraintSettings c;c.mSpace=EConstraintSpace::LocalToBodyCOM;c.mPoint1=a;c.mPoint2=b;
            //2F4AF0/389510: source zero is A.perp2, about the NEGATIVE axle.
            c.mHingeAxis1=-vector(j.basis[0]+4).Normalized();c.mHingeAxis2=-vector(j.basis[1]+4).Normalized();
            const float mid=(j.limits[0]+j.limits[1])*.5f;
            c.mNormalAxis1=Quat::sRotation(c.mHingeAxis1,mid)*vector(j.basis[0]+12).Normalized();c.mNormalAxis2=vector(j.basis[1]+8).Normalized();
            c.mLimitsMin=j.limits[0]-mid;c.mLimitsMax=j.limits[1]-mid;c.mMaxFrictionTorque=j.max_friction_torque;
            constraint=c.Create(*bodies[j.body[0]],*bodies[j.body[1]]);
        }else{
            need(j.kind==1,"Unsupported native ragdoll joint");SwingTwistConstraintSettings c;c.mSpace=EConstraintSpace::LocalToBodyCOM;c.mPosition1=a;c.mPosition2=b;
            c.mPlaneAxis1=vector(j.basis[0]+4).Normalized();c.mPlaneAxis2=vector(j.basis[1]+4).Normalized();
            c.mTwistAxis1=vector(j.basis[0]+8).Normalized();c.mTwistAxis2=vector(j.basis[1]+8).Normalized();
            const float mid=(j.limits[1]+j.limits[2])*.5f;const Vec3 normal=c.mPlaneAxis1.Cross(c.mTwistAxis1).Normalized();
            const Quat bias=Quat::sRotation(normal,mid);c.mPlaneAxis1=bias*c.mPlaneAxis1;c.mTwistAxis1=bias*c.mTwistAxis1;
            // ponytail: Jolt's elliptical swing is a native solver adaptation,
            // not Havok's exact cone/plane envelope. Preserve authored extents
            // and bias; a source solver is needed for bitwise dynamics parity.
            c.mPlaneHalfConeAngle=(j.limits[2]-j.limits[1])*.5f;c.mNormalHalfConeAngle=j.limits[0];
            c.mTwistMinAngle=j.limits[3];c.mTwistMaxAngle=j.limits[4];c.mMaxFrictionTorque=j.max_friction_torque;
            constraint=c.Create(*bodies[j.body[0]],*bodies[j.body[1]]);
        }
        state->joints.push_back({constraint,j.body[0],j.body[1],vector(j.basis[0]),vector(j.basis[1])});state_->physics.AddConstraint(constraint);
    }
    for(auto id:state->bodies)bi.AddBody(id,EActivation::Activate);
    return std::unique_ptr<NativeRagdoll>(new NativeRagdoll(std::move(state)));
}
NativeRagdoll::NativeRagdoll(std::unique_ptr<State> state):state_(std::move(state)){}
NativeRagdoll::~NativeRagdoll()=default;
unsigned NativeRagdoll::body_count()const{return static_cast<unsigned>(state_->bodies.size());}
void NativeRagdoll::transform(unsigned i,float out[16])const{
    need(out&&i<body_count(),"Invalid native ragdoll transform request");const auto m=state_->world->physics.GetBodyInterface().GetWorldTransform(state_->bodies[i]);
    for(unsigned column=0;column<4;++column)m.GetColumn4(column).StoreFloat4(reinterpret_cast<Float4*>(out+4*column));
}
void NativeRagdoll::velocity(unsigned i,const float linear[3],const float angular[3]){
    need(i<body_count(),"Invalid native ragdoll velocity request");auto &bi=state_->world->physics.GetBodyInterface();bi.SetLinearAndAngularVelocity(state_->bodies[i],vector(linear),vector(angular));
}
void NativeRagdoll::add_impulse(unsigned i,const float impulse[3],const float *point){
    need(i<body_count(),"Invalid native ragdoll impulse request");auto &bi=state_->world->physics.GetBodyInterface();if(point)bi.AddImpulse(state_->bodies[i],vector(impulse),vector(point));else bi.AddImpulse(state_->bodies[i],vector(impulse));
}
void NativeRagdoll::add_force(unsigned i,const float force[3],const float *point){
    need(i<body_count(),"Invalid native ragdoll force request");auto &bi=state_->world->physics.GetBodyInterface();if(point)bi.AddForce(state_->bodies[i],vector(force),vector(point));else bi.AddForce(state_->bodies[i],vector(force));
}
float NativeRagdoll::joint_error()const{
    float error=0;auto &bi=state_->world->physics.GetBodyInterface();
    for(auto &j:state_->joints){const auto a=bi.GetWorldTransform(state_->bodies[j.a])*j.pivot_a,b=bi.GetWorldTransform(state_->bodies[j.b])*j.pivot_b;error=std::max(error,(a-b).Length());}return error;
}
