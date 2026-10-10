#ifndef UC_CHARACTER_COLLISION_H
#define UC_CHARACTER_COLLISION_H
#include "collision_triangle.h"
#ifdef __cplusplus
extern "C" {
#endif
/*21C310 family1: actual bind correction times sampled bone world, normalized
 * basis rows, then XZY row order. Translation remains render centimetres.
 * Finite nonzero basis, double-backed x87 normalization; disjoint output.*/
int uc_character_collision_pose(const float bind[16],const float posed[16],float out[16]);
typedef struct UCCharacterCapsule {float a[3],b[3],radius;} UCCharacterCapsule;
/*PC geometric adapter for the retained authored capsule. Physics metres, no
 * contact dynamics/keyframe solver. Like301520, an interior start is excluded;
 * strict closer selection preserves existing owners at equal fractions.
 *1 hit,0 miss,-1 invalid; preserves output on miss/error.*/
int uc_character_capsule_cast(const UCCharacterCapsule*,const float from[3],
 const float to[3],float limit,UCCollisionTriangleHit*);
#ifdef __cplusplus
}
#endif
#endif
