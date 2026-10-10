#ifndef UC_PLAYER_SHIELD_HIT_H
#define UC_PLAYER_SHIELD_HIT_H
#include "player_damage.h"
#include "model_asset.h"
#include "particle_draw.h"
#include "game_random.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCPlayerShieldRecoil {float horizontal,vertical,impact;} UCPlayerShieldRecoil;
typedef struct UCPlayerShieldMark {float point[4][3],alpha;} UCPlayerShieldMark;
enum {UC_PLAYER_SHIELD_MARKS=300};
typedef struct UCPlayerShieldMarks {uint32_t count;UCPlayerShieldMark marks[UC_PLAYER_SHIELD_MARKS];} UCPlayerShieldMarks;
/* Whole C6A80/C6820 with the real selector0 matrix and shared game RNG.
 * damage is ignored by the original helper. type0..3 follows its exact arrays.
 * Retains actor244/248/24C and shares the actual658/65C kick fields. */
/* BCCB0 selector11 uses the authored ShieldMain bone. Same C0222 matrix
 * consumer as qualified uc_muzzle_world; absent tag returns -1. */
int uc_player_shield_bind(const UCModel*,int32_t*);
/* Whole BC0A0 geometry/color publication. matrix is the last published260;
 * shade is actual2A0, dim is world2149B0, and first/count select live marks. */
int uc_player_shield_geometry(const UCPlayerShieldMarks*,unsigned first,unsigned count,
 const float matrix[16],float shade,int32_t item,int32_t action,uint8_t dim,UCParticleQuad*);
int uc_player_shield_recoil(UCPlayerShieldRecoil*,UCPlayerImpactState*,const float frame[16],unsigned type,const float direction[3],UCGameRandom*);
/* C28F0 selector0, BC350 and BBF60: generates the authored bullet mark and
 * surface sample using real selector11 ShieldMain matrix. Marks retain source
 * capacity300 and alpha; full buffers still consume all five random draws.
 * Output is the transformed local surface sample (separate from quad corners). */
int uc_player_shield_surface(UCPlayerShieldMarks*,const float shield_frame[16],UCGameRandom*,float point[3]);
/* Whole BC930 fields, including source +25C feedback intensity publication.
 * Per-update decay is deliberately not multiplied by elapsed time. */
int uc_player_shield_recover(UCPlayerShieldRecoil*,float *feedback_amount);
/* Whole C0C80: same actor64C/650 intensity and 654 rate; adds to actual player
 * 694/698 pitch/yaw before its base/look matrices are rebuilt. */
int uc_player_hit_angles(UCPlayerImpactState*,float *rate,const UCPlayerShieldRecoil*,float delta,float angles[3]);
typedef struct UCShieldSparkServices {
 void *owner;const float *world;
 int(*feedback)(void*,const float[3],float,float,float,unsigned);
 int(*particles)(void*,const char*,const float[16]);
 int(*yield)(void*);double(*delta)(void*);int(*remove)(void*);
}UCShieldSparkServices;
/*18ECF0 ->18EB40. Completing yield suspends the actual owning continuation. */
int uc_player_shield_spark_sequence(const UCShieldSparkServices*);
#ifdef __cplusplus
}
#endif
#endif
