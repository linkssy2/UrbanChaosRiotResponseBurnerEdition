#ifndef UC_GAME_DIFFICULTY_H
#define UC_GAME_DIFFICULTY_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* The actual profile+48 and adaptive+28/+2C read by167C0. This projection
 * does not replace the remaining profile or adaptive-history update owner. */
typedef struct UCGameDifficulty {uint32_t mode;float adaptive,scale;} UCGameDifficulty;
void uc_game_difficulty_fresh(UCGameDifficulty *);
/* Keep the source x87 result in double until its actual consumer stores it. */
int uc_game_difficulty_value(const UCGameDifficulty *,double *);
/*16A60 checkpoint reward. Same adaptive+28, profile+48 and global5096DC. */
int uc_game_difficulty_checkpoint(UCGameDifficulty *,uint8_t disabled);
#ifdef __cplusplus
}
#endif
#endif
