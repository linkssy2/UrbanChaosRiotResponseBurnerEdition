#ifndef UC_NATIVE_GAME_RANDOM_H
#define UC_NATIVE_GAME_RANDOM_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCGameRandom { uint32_t words[250],first,second; } UCGameRandom;
/* Original2D36C0 fixed table and reset indices0/147, followed by warmup draws.
 * warmup is0 for deterministic reset, otherwise the supplied original timer's
 * low byte. Clock selection and reset timing belong to the session owner.
 * This is the SHARED game stream: never reseed or clone it for every weapon.
 * Returns0 on success,1 on invalid arguments; invalid calls preserve outputs. */
int uc_game_random_init(UCGameRandom *,uint8_t warmup);
/* Original2D3670: advance both indices, XOR, store and return the signed word.
 * State and result storage must be disjoint. No guest globals or host RNG. */
int uc_game_random_next(UCGameRandom *,int32_t *);
/* Original2D37B0 signed remainder, including wrapped last-first and result.
 * A zero divisor or INT_MIN/-1 would trap on Xbox: reject before consuming.
 * Original2D3800 uses signed float conversion, not a random-word bit test.
 * Same state/output separation and invalid-call preservation as next. */
int uc_game_random_range(UCGameRandom *,int32_t first,int32_t last,int32_t *);
int uc_game_random_boolean(UCGameRandom *,int32_t *);
#ifdef __cplusplus
}
#endif
#endif
