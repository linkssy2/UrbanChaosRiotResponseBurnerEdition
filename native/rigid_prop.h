#ifndef UC_RIGID_PROP_H
#define UC_RIGID_PROP_H
#include "entity_stream.h"
#include "collision_asset.h"
typedef struct UCRigidPropParameters {
    float mass,linear_damping,angular_damping,friction,restitution;
    float velocity[3],angular_velocity[3];
    const char *sound;
    uint32_t unmovable,max_bullet_hits;
    float max_life,explode_delay;
    uint32_t hurts_on_collision,explodable;
    float explosion_offset[3];
    uint32_t explode_on_contact;
    float contact_speed;
    uint32_t influenced_by_explosions;
    int32_t explode_contact;
    float respawn_time;
    const char *point_particle,*shot_particle;
    float temperature;
} UCRigidPropParameters;
#ifdef __cplusplus
extern "C" {
#endif
/* Immutable authored-stream projection of1363A0's23 ordered property reads.
 * Borrows the SAME descriptor/link strings and advances its supplied cursor.
 * Source booleans consume a u32 and test only its low byte. All23 values must
 * exist; defaults/name lookup and intervening constructor effects are outside
 * this projection. Failure preserves output/cursor. This does not construct a
 * RigidBody, resolve sound/particles or complete contact registration. */
int uc_rigid_prop_parameters(const UCLevelEntity *,const uint8_t **cursor,UCRigidPropParameters *);
/* Full2A3E50 family2 box conversion. Finite affine inputs with zero basis W,
 * translation W1, positive dimensions, RN/FTZ-off; disjoint input/output.
 * Preserves imported metadata/radius and changes actual halfXYZ/W, transform
 * and quaternion. No other family or runtime body/group selection implied. */
int uc_rigid_prop_box(const UCModelCollisionShape *,UCModelCollisionShape *);
#ifdef __cplusplus
}
#endif
#endif
