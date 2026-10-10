#ifndef UC_MUZZLE_PLACEMENT_H
#define UC_MUZZLE_PLACEMENT_H
#include "model_asset.h"
#ifdef __cplusplus
extern "C" {
#endif
/* BCCB0 tag2 -> B5630, default ASCII locale: first case-insensitive
 * 16-byte bone name match for DummyFlare. Call once after equipment changes.
 * Missing is successful with index=-1. Invalid inputs preserve the output. */
int uc_muzzle_bind(const UCModel *,int32_t *index);
/* C01C0 tag10 uses the same lookup and axis conversion on the arms model. */
int uc_flashlight_bind(const UCModel *,int32_t *index);
/* Complete C01C0 tag2 path with a present model wrapper and final Mainplayer
 * C0180 weapon placement. pose contains model-space composed bone worlds;
 * validity must mark the selected bone. index=-1 uses the original identity
 * fallback. No fabricated pose for an unvisited bone; no palette correction.
 * The source21B0D0 Y/Z-axis swap precedes weapon-world composition.
 * Normal FP rounding, FTZ/DAZ off, finite inputs/results, disjoint storage.
 * Failure preserves out. Scheduling/D0080 copy to GunFlare remain caller owned. */
int uc_muzzle_world(int32_t index,uint32_t count,const float (*pose)[16],
                    const uint8_t *valid,const float weapon[16],float out[16]);
#ifdef __cplusplus
}
#endif
#endif
