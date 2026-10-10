#ifndef UC_EXPLOSION_H
#define UC_EXPLOSION_H
#include "level_asset.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCExplosionParameters {
 uint32_t sound;float scale;int32_t damage_type;float radius,power;
 int32_t owner_id;uint32_t affects_doors,ignore_collision;int32_t particle_type;
 uint32_t force_kill;int32_t damage_team;uint32_t show_particles,effects_only;
} UCExplosionParameters;
/* Authored E8E00 property values, not a complete Explosion constructor.
 * Loaded descriptor remains immutable. Output unchanged on invalid input. */
int uc_explosion_parameters(const UCLevelEntity *,UCExplosionParameters *);
typedef union UCExplosionProperty {int32_t integer;float number;const char *string;} UCExplosionProperty;
typedef struct UCExplosionConstructor {
 UCExplosionParameters parameters;
 const char *sound;void *audio,*field;int32_t child_b4,child_b8;
} UCExplosionConstructor;
typedef struct UCExplosionConstructorServices {
 void *owner;
 int(*base)(void*); /*complete20BDC0, including actual registration/placement*/
 int(*coroutine_initialize)(void*); /*actual1FB780, retained B0 owner*/
 int(*property)(void*,unsigned,const char*,UCExplosionProperty*,unsigned *found);
 int(*sound)(void*,const char*,void **); /*216020 with real actor and flag0*/
 int(*coroutine_bind)(void*); /*1FB9D0(actual entity,0), after final fields*/
} UCExplosionConstructorServices;
/* WholeE8E00 suffix and callback ordering. Services complete original owners;
 * no success placeholders. Fields publish at their actual source frontiers.
 * Missing properties use source defaults; Boolean properties test only low8.
 * parameters.sound is the unrelated authored link-index projection and remains
 * untouched; actual type6 string/audio identity is retained in sound/audio.
 * Constructor power is multiplied by1.5f exactly once. Source-unwritten fields
 * remain untouched.0 completed,1 invalid,2 committed failure: never retry2. */
int uc_explosion_construct(UCExplosionConstructor*,const UCExplosionConstructorServices*);
typedef struct UCExplosionScorchServices {
 void *owner;
 int(*query)(void*,const float from[3],const float to[3],uint32_t type,
  uint32_t group,uint32_t mask,float impulse,float point[3],float normal[3],uint8_t *hit);
 int(*decal)(void*,const char *shader,const float point[3],const float normal[3],
  const float size[2],uint32_t mask);
} UCExplosionScorchServices;
/* Whole E9130. No hit emits no mark and consumes no geometry RNG. The decal
 * callback completes the NULL-tangent2088D0 path, flags0/final0. */
int uc_explosion_scorch(const float position[3],const UCExplosionScorchServices*);
/* EA690 amount only, after the separate E9610 eligibility/occlusion query.
 * Target origin/render cm; horizontal radius and vertical extent are explicit
 * recipient arguments (Mainplayer/Character use80,140). No health publication.
 * Matches bounded lifted x87 double arithmetic, not all extended80 FP states. */
int uc_explosion_damage_amount(const float center[3],float radius,float damage,
 int32_t damage_type,uint8_t blast,const float target[3],float width,float height,float *out);
/* E9610/E9430 eligibility before damage. E90C0 remains the caller's real
 * obstruction query: 0 clear,1 blocked,negative error. Target identity and
 * source-selected collision group are supplied by the same actor/session.
 * Coordinates are render centimetres; either clear body ray admits damage.
 * Damage type2 overrides ignore_collision. No health/team/response mutation.
 * Output is unchanged on invalid input or query error. */
typedef int (*UCExplosionObstruction)(void *,const float from[3],const float to[3],
 const void *target,int32_t collision_group);
/*WholeE90C0: collision214230 then same-target exception in offline sessions.
 *query returns -1 error,0 miss,1 hit and optional owning actor identity.*/
typedef struct UCExplosionObstructionServices {
 void*owner;
 int(*query)(void*,const float*,const float*,uint32_t,uint32_t,uint32_t,float,int32_t*,uint8_t*);
 int(*multiplayer)(void*);
} UCExplosionObstructionServices;
int uc_explosion_obstructed(const float from[3],const float to[3],int32_t target,
 int32_t collision_group,const UCExplosionObstructionServices*);
int uc_explosion_damage_eligible(const float center[3],float radius,int32_t damage_type,
 uint8_t ignore_collision,const float position[3],float width,float height,
 const void *target,int32_t collision_group,UCExplosionObstruction,void *,uint8_t *out);
/* EA580 admitted positive-distance interior, physics coordinates. The outer
 *2A6C70 owner must filter bodies, enforce80 unique bodies/30 updates, apply its
 * .03 scaling/type policy and actually publish impulses. This is its field
 * vector, not a complete Havok replacement. Zero-distance is rejected rather
 * than sending the original nonfinite result to a body. */
int uc_explosion_force(const float center[3],float radius,float power,float minimum,
 float maximum,float remaining,const float target[3],float out[3]);
/* E8D80 duration: -1 is infinite,0 is stopped; finite nonnegative delta. */
int uc_explosion_force_tick(float *remaining,float delta);
/*2A6C70 ordered body projection. Caller owns broadphase enumeration and body
 * identities for the entire field lifetime. It must publish returned impulses
 * and type1 property16 changes before advancing physics. No Havok ownership.
 * disabled is original body+92, NOT a guessed sleeping flag. candidate.dynamic
 * is original broadphase type1; excluded is property2B. Positions are physics
 * metres; force_position is the source motion point, not necessarily origin.
 * Zero initialization starts a field; center/power are supplied explicitly.
 * Invalid input rejects before writes. Coincident force points reject to avoid
 * original NaNs. State remains usable only while identities cannot be reused. */
typedef struct UCExplosionField {
 float center[3],power,remaining;
 uint32_t ticks,done,count,visited[80];
} UCExplosionField;
typedef struct UCExplosionBody {
 uint32_t identity,dynamic,excluded,disabled,type;
 float position[3],force_position[3],property16;
 float impulse[3];uint32_t affected,impulsed;
} UCExplosionBody;
int uc_explosion_field_step(UCExplosionField *,float delta,UCExplosionBody *,size_t count);
#ifdef __cplusplus
}
#endif
#endif
