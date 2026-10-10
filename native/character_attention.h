#ifndef UC_CHARACTER_ATTENTION_H
#define UC_CHARACTER_ATTENTION_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCCharacterAttention {float elapsed;int32_t target,actor;float point[3];} UCCharacterAttention;
typedef struct UCCharacterAttentionServices {
 void*owner;double delta;
 int (*world)(void*,int32_t,float[16]); /*1 found,0 missing,-1 error*/
 int (*head)(void*,int32_t,float[3]); /*original property fallback belongs caller*/
} UCCharacterAttentionServices;
int uc_character_attention_init(UCCharacterAttention*,int32_t,const UCCharacterAttentionServices*);
int uc_character_attention_step(UCCharacterAttention*,const UCCharacterAttentionServices*);
int uc_character_attention_near(const UCCharacterAttention*,const UCCharacterAttentionServices*);
int uc_character_attention_select(UCCharacterAttention*,int32_t,float,const UCCharacterAttentionServices*);
void uc_character_attention_reset(UCCharacterAttention*);
/*34270: HOT ZONE/event107 has priority over COLD ZONE/event28. The query
 * callback owns original1F90E0 virtual+4 semantics, not ordinary event send.
 * Actor ID/lifetime is stable throughout; named supplies the live ordered view.
 * Callbacks return0 on success; query publishes handled and one-byte reply.
 * Return-1 on invalid/service failure, otherwise0none/1hot/2cold. */
typedef struct UCCharacterZoneServices {
 void *owner;
 int (*named)(void*,const char*,const int32_t**,uint32_t*);
 int (*query)(void*,int32_t target,uint32_t event,int32_t actor,uint8_t *handled,uint8_t *reply);
} UCCharacterZoneServices;
int uc_character_dialogue_zone(int32_t actor,const UCCharacterZoneServices*);
#ifdef __cplusplus
}
#endif
#endif
