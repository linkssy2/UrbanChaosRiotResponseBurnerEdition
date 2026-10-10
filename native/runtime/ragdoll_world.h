#pragma once
#include "../ragdoll_asset.h"
#include "../rigid_prop.h"
#include <memory>
#include <cstdint>
struct NativeCharacterWorld;
class NativeRagdoll;
class NativePropBody;

// Shared native solver, in the original physics axes/metres. A level's mesh
// is built once; characters own only their articulated bodies and constraints.
// Solver ownership survives a late corpse release during level teardown.
class NativeRagdollWorld {
    struct State;
    std::shared_ptr<State> state_;
    friend class NativeRagdoll;
    friend class NativePropBody;
public:
    NativeRagdollWorld(const UCCollisionAsset &,const float gravity[3]);
    ~NativeRagdollWorld();
    NativeRagdollWorld(const NativeRagdollWorld &)=delete;
    NativeRagdollWorld &operator=(const NativeRagdollWorld &)=delete;
    void sync_collision(const NativeCharacterWorld &);
    void step(float delta);
    std::unique_ptr<NativeRagdoll> create(const UCRagdollAsset &,const float (*body_world)[16]);
    uint32_t body_count()const;
    uint32_t ragdoll_body_count()const;
    std::unique_ptr<NativePropBody> create_prop(int32_t entity,const UCModelCollisionShape *,size_t count,const float world[16],const UCRigidPropParameters &);
};
class NativePropBody {
    struct State;std::unique_ptr<State> state_;
    explicit NativePropBody(std::unique_ptr<State>);
    friend class NativeRagdollWorld;
public:
    ~NativePropBody();
    void sample(float world[16],float velocity[3],bool &sleeping)const;
    void impulse(const float momentum[3]);
    void sleep();
    void wake();
    void shape(const UCModelCollisionShape *,size_t count);
};
class NativeRagdoll {
    struct State;
    std::unique_ptr<State> state_;
    explicit NativeRagdoll(std::unique_ptr<State>);
    friend class NativeRagdollWorld;
public:
    ~NativeRagdoll();
    NativeRagdoll(const NativeRagdoll &)=delete;
    NativeRagdoll &operator=(const NativeRagdoll &)=delete;
    void transform(unsigned body,float out[16])const;
    void velocity(unsigned body,const float linear[3],const float angular[3]);
    void add_impulse(unsigned body,const float impulse[3],const float *point=nullptr);
    void add_force(unsigned body,const float force[3],const float *point=nullptr);
    unsigned body_count()const;
    float joint_error()const;
};
