#ifndef UC_CHARACTER_FACE_H
#define UC_CHARACTER_FACE_H
#include "game_random.h"
#ifdef __cplusplus
extern "C" {
#endif
/*2AD2E0/2AD5A0/2AFB60: retained noise profiles and damped head motion.
 * One shared game RNG, four constructor draws. No private random stream. */
typedef struct UCCharacterFaceNoise {
    int32_t axis,secondary;float magnitude,chance,minimum,maximum,remaining,value[3];
} UCCharacterFaceNoise;
typedef struct UCCharacterFace {
    UCCharacterFaceNoise noise[4];float head[3],rate[3],elapsed,phoneme_elapsed;
} UCCharacterFace;
/*2ADBC0 type6 record. bone initially names a source symbolic bone, resolved
 * against the actual skeleton before2B62E0 consumes it. -1 is absent. */
typedef struct UCCharacterFacePose {int32_t bone;float rotation[3],translation[3];} UCCharacterFacePose;
int uc_character_face_init(UCCharacterFace *,UCGameRandom *);
int uc_character_face_step(UCCharacterFace *,UCGameRandom *,const uint8_t *phonemes,
    uint32_t count,float delta,float weight,UCCharacterFacePose output[8],uint32_t *emitted);
int uc_character_face_apply(float (*local)[16],uint32_t bones,const UCCharacterFacePose *,uint32_t count);
#ifdef __cplusplus
}
#endif
#endif
