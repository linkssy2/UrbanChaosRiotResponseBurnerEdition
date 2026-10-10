#ifndef UC_DIALOGUE_EMOTE_H
#define UC_DIALOGUE_EMOTE_H
#include <stdint.h>
#include "game_random.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCDialogueEmoteSession {
 uintptr_t speakers[3];int32_t lines[3],counts[2],total;float cooldowns[230];
} UCDialogueEmoteSession;
typedef struct UCDialogueEmoteState {
 uint32_t mode;int32_t priority;float cooldown;uint8_t locked,active,has_bank;
} UCDialogueEmoteState;
typedef struct UCDialogueEmoteServices {
 void*owner;
 double (*distance)(void*); /* 21A380 */
 int (*actor_flags)(void*,uint32_t*); /* registry actor; 1found,0absent */
 uint8_t (*busy)(void*); /* 21A400 */
 void (*stop)(void*,uint8_t emote_bank); /* 21A000 */
 void (*play)(void*,int32_t line); /* 21A450, original result ignored */
 uint8_t (*text)(void*,int32_t line,uint8_t actor_found); /* format,subtitle,17CC0 */
} UCDialogueEmoteServices;
/* Exact21A730 decision and219FA0/219DF0 shared ownership. Numeric code0
 * is silence; signed packed codes choose lowbyte for modes1..3, highbyte0. */
int uc_dialogue_emote_select(UCDialogueEmoteSession*,UCDialogueEmoteState*,uintptr_t identity,const char*prefix,int32_t code,int32_t priority,const UCDialogueEmoteServices*);
int uc_dialogue_emote_release(UCDialogueEmoteSession*,UCDialogueEmoteState*,uintptr_t identity);
int uc_dialogue_emote_advance(UCDialogueEmoteSession*,float delta);
int uc_dialogue_emote_pair(UCGameRandom*,int32_t ordinal,int32_t first,int32_t second,float chance,int32_t*selected);
int32_t uc_dialogue_mission_ordinal(const char*);
double uc_dialogue_emote_distance(const float matrix[16],const float position[3]);
#ifdef __cplusplus
}
#endif
#endif
