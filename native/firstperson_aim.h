#ifndef UC_NATIVE_FIRSTPERSON_AIM_H
#define UC_NATIVE_FIRSTPERSON_AIM_H
#include "firstperson_motion.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct UCFirstpersonAimRay {
    float origin[3], direction[3], end[3], miss_distance;
} UCFirstpersonAimRay;
typedef struct UCFirstpersonAimCollision {
    int32_t entity;
    float distance, point[3];
    uint32_t detail;
} UCFirstpersonAimCollision;
typedef struct UCFirstpersonAimHit {
    int32_t entity;
    uint32_t classification;
    float distance;
    uint32_t detail;
    float point[3];
} UCFirstpersonAimHit;
typedef struct UCFirstpersonAimHistory {
    int32_t selected_entity;
    float point[3], previous_point[3];
} UCFirstpersonAimHistory;

/* Prepare the original forward query from the resolved tag0 look frame.
 * ray_length is already resolved by the current-item/range owner; no default
 * weapon range or collision result is invented. miss_distance preserves the
 * original float(ray_length + .01f), for use only after a real query miss.
 * No raycast is performed. The original query's category9/mask0x4000 and player
 * identifier remain the scene-query caller's responsibility. */
UCMotionStatus uc_firstperson_aim_ray_prepare(const float look[16], float ray_length,
    UCFirstpersonAimRay *out);

/* Initial hit publication BEFORE property/team classification and the special
 * held-selector1/15 transform-tag override. Initial classification is0 when
 * entity==-1, otherwise5. The supplied collision is an actual owner's result.
 * This output is not a completed classification or a complete aim update. */
UCMotionStatus uc_firstperson_aim_hit_begin(const UCFirstpersonAimCollision *collision,
    UCFirstpersonAimHit *out);

/* After the completed hit/classification owner, prepare the next aim history.
 * held_transform is the resolved held-object virtual+24 transform, or NULL only
 * when the actual held-object owner is absent. Copies old point to previous,
 * clears selected_entity to-1, and writes the original15000-unit endpoint.
 * search_needed is1 except for final classification1 or4. Stops BEFORE the
 * AUTOAIM world query; a miss/class0 still requests that query. Candidate
 * selection/publication and final selected-point replacement remain external.
 * Neither input matrix is modified; no controller/frame owner is inferred. */
UCMotionStatus uc_firstperson_aim_history_begin(UCFirstpersonAimHistory *state,
    const float look[16], const float *held_transform, uint32_t classification,
    unsigned *search_needed);

/* All calls require disjoint input/state/output storage and finite supplied
 * numeric inputs. Ray length must be nonnegative; classification must be0..5.
 * Preparation requires a nonzero direction and finite intermediate results,
 * nearest-even rounding with denormals enabled. Failures publish nothing.
 * History reads old point only; old previous-point bytes may be arbitrary.
 * Original x87 normalization is compared through the existing double-backed
 * lifted reference, not certified for all Xbox80-bit rounding/FP-status cases.
 * No allocation, device, world query, callback or initialization occurs. */
#ifdef __cplusplus
}
#endif
#endif
