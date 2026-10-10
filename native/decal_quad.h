#ifndef UC_DECAL_QUAD_H
#define UC_DECAL_QUAD_H
#include "vector_random.h"
/* Actual223450 NULL-tangent impact-decal branch, including a single shared
 * four-lane random draw and2D3C70 axis-angle rotation (host double sin/cos).
 * Finite point magnitude<=1e6, normal<=16 with nonzero cross length, size<=1e4,
 * RN/DAZ-FTZ off and disjoint inputs/output. Source normal need not be unit.
 * Return1 rejects before random/output mutation;0 publishes all four corners.
 * This constructs geometry only; validation rays/material/ring/draw are separate. */
int uc_decal_quad(const float point[3],const float normal[3],const float size[2],
    UCVectorRandom *,float corners[4][3]);
#endif
