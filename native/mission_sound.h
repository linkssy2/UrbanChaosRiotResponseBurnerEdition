#ifndef UC_MISSION_SOUND_H
#define UC_MISSION_SOUND_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCMissionSound {
 uint8_t started,wait_for_voice,stopping;float remaining;
 uint8_t has_bank,has_start;
} UCMissionSound;
typedef struct UCMissionSoundServices {
 void*owner;int(*play)(void*,unsigned stop);int(*playing)(void*,unsigned*);
 double(*delta)(void*);int(*remove)(void*);
} UCMissionSoundServices;
/* Whole110F70: first start is its own tick. The Stop request starts a .5s
 * trailing interval; crossing zero removes on the following tick. */
int uc_mission_sound_step(UCMissionSound*,const UCMissionSoundServices*);
#ifdef __cplusplus
}
#endif
#endif
