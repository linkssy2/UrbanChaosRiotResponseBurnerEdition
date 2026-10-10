#ifndef UC_VECTOR_RANDOM_H
#define UC_VECTOR_RANDOM_H
#include <stdint.h>
/* Original shared51E190 stream, independent of the250-word integer game RNG.
 * Native indices replace the two pointers into28 groups of four floats. */
typedef struct UCVectorRandom {float values[112];uint32_t first,second;} UCVectorRandom;
/* Actual3AAED0 startup: seed12345, modulus4000000, including the original
 * initialization's zeroed last slot and its in-place four mixing passes. */
int uc_vector_random_init(UCVectorRandom *);
/* One original signed four-lane draw. RN, DAZ/FTZ off, initialized shared state,
 * finite scale of magnitude<=65536, disjoint result. Returns1 before mutation
 * for invalid arguments/state; otherwise publishes all four lanes. */
int uc_vector_random_signed(UCVectorRandom *,float scale,float result[4]);
#endif
