#pragma once
#include "mission_world.h"
#include "model.h"
#include "ragdoll_world.h"
#include <vector>
#include <string>
extern "C" {
#include "../rigid_prop.h"
#include "../rigid_prop_damage.h"
#include "../explosion.h"
#include "../collision_asset_query.h"
#include "../player_controller.h"
}
class NativeRigidProp;
struct NativeRigidPropServices {
    void *owner=nullptr;
    int (*particle)(void *,NativeRigidProp &,const char *asset,const char *bone,uint8_t hidden)=nullptr;
    int (*explosion)(void *,NativeRigidProp &,const UCRigidPropExplosion &)=nullptr;
    int (*sound)(void *,NativeRigidProp &,const char *event)=nullptr;
    NativeRagdollWorld *physics=nullptr;
};
/* Explicit PC prop adapter, not complete1363A0/3DAA0 or a Havok replacement.
 * SAME authored actor/slot/mission owner, real initial-pose group0 model and
 * imported family2 boxes. Production uses shared Jolt bodies for scenery,
 * prop/prop and ragdoll contacts, rotation and sleeping, with authored mass,
 * damping/friction/restitution. Legacy isolated fixtures without physics services
 * retain translation-only corner sweeps. Contact sound/damage and respawn remain
 * unsupported; this is not bitwise Havok solver parity. Actual135840/135AE0/
 *135BA0 hit/timer policy dispatches required attached-particle and deferred
 * Explosion services; unresolved services fail when reached. Box/canister
 * resources require zero initial velocities, no max-life/contact/respawn
 * triggers, supported shot recipe and orthonormal authored transforms.
 * Original collision-body property21/22 producers remain unsupported; the
 * present PC body has no such properties. No bulk/all-level spawn. */
class NativeRigidProp {
    NativeAnimatedModel model_;
    std::unique_ptr<NativeAnimatedModel> destroyed_model_;
    UCRigidPropParameters parameters_{};
    std::vector<UCModelCollisionShape> local_;
    std::vector<UCSolidBlockGeometry> boxes_;
    const UCCollisionAsset *scenery_=nullptr;
    const float *delta_=nullptr;
    float velocity_[3]{};
    float resting_time_=0;
    bool ready_=false,sleeping_=false;
    int failure_=0;
    NativeMissionWorld *world_=nullptr;
    NativeRenderer *renderer_=nullptr;
    const UCArchive *archive_=nullptr;
    uint8_t archive_hash_[32]{};
    std::filesystem::path catalog_;
    std::string asset_;
    std::vector<UCModelCollisionShape> all_shapes_;
    uint32_t groups_=0;
    UCEntitySubscriptions *subscriptions_=nullptr;
    UCRigidPropDamage damage_{};
    NativeRigidPropServices services_{};
    std::unique_ptr<NativePropBody> physics_body_;
    uint32_t contacts_=0;float blast_=0;
    int base_update();
    int group_one();
    UCRigidPropDamageServices damage_services();
    void refresh_boxes();
public:
    NativeMissionEntity entity;
    NativeRigidProp()=default;
    ~NativeRigidProp();
    NativeRigidProp(const NativeRigidProp&)=delete;
    NativeRigidProp&operator=(const NativeRigidProp&)=delete;
    /* Caller binds entity.view before registration; world.ensure owns late
     * slot publication. Borrowed archive/scenery/world outlive close. Returns
     *1 on unsupported before registration,2 after committed constructor effects. */
    int construct(NativeRenderer &,NativeMissionWorld &,uint32_t source_index,
        const UCArchive *,const uint8_t archive_hash[32],const std::filesystem::path &catalog,
        const UCCollisionAsset &scenery);
    int update();
    int late();
    int close_detached();
    int retire();
    void draw(NativeRenderer &,const float view_projection[16],float inverse_exposure=1,NativeModelLighting *lighting=nullptr,const NativeLightingFrame *frame=nullptr);
    void collect_shadows(NativeRenderer &,NativeModelLighting &,const NativeLightingFrame &,class NativeModelShadowCasters &,float fraction=1);
    /* PC impulse in physics coordinates; changes the real retained body velocity.
     * Does not claim original bullet damage or contact impulse distribution. */
    int impulse(const float momentum[3]);
    int bullet_impulse(const float momentum[3]);
    int player_contact(const UCPlayer&,const UCPlayerInput&);
    void sync_physics();
    void set_services(const NativeRigidPropServices &s){services_=s;}
    int bullet_contact(int32_t projectile);
    int event_hit(int32_t sender);
    int32_t bone_index(const char *)const;
    bool posed_bone(int32_t,float out[16])const;
    const UCRigidPropDamage&damage()const{return damage_;}
    const std::vector<UCSolidBlockGeometry>&boxes()const{return boxes_;}
    const std::vector<UCModelCollisionShape>&collision_shapes()const{return local_;}
    const UCRigidPropParameters&parameters()const{return parameters_;}
    bool sleeping()const{return sleeping_;}
    bool ready()const{return ready_;}
    int failure()const{return failure_;}
};
/* PC body adapter for the qualified explosion scanner. Actual retained props
 * provide stable entity IDs and physics origins; this PC adapter
 * has no original Havok body classifications, so ordinary movable bodies use
 * type0/disabled0. Sleeping PC props can be woken by the real impulse endpoint.
 * No character damage, torque or explosive-canister chain reaction is implied. */
int native_prop_explosion_step(UCExplosionField &,float delta,
    const std::vector<std::unique_ptr<NativeRigidProp>> &);
