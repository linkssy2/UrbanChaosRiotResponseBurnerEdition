#ifndef UC_CHARACTER_HEARTBEAT_H
#define UC_CHARACTER_HEARTBEAT_H
#include <stdint.h>
typedef struct UCCharacterHeartbeat {
 int32_t next,count;const int32_t*flags;const char*text;float percent;
} UCCharacterHeartbeat;
typedef struct UCCharacterHeartbeatServices {
 void*owner;
 int(*enter)(void*);int(*root_exists)(void*);int(*regeneration_gate)(void*,int32_t);
 double(*health)(void*);double(*maximum)(void*);int(*set_health)(void*,float);
 int(*monitor_ready)(void*);int(*resource)(void*,const char**);
 int(*show)(void*,const char*);int(*transition)(void*,int32_t);int(*exit)(void*);int(*base)(void*,uint8_t);
} UCCharacterHeartbeatServices;
/*Whole116B00 entry and116C00 update. Transition returns1 when changed,0 when
 *no next behavior; other services return0success except named predicates.*/
int uc_character_heartbeat_enter(const UCCharacterHeartbeat*,const UCCharacterHeartbeatServices*);
int uc_character_heartbeat_update(const UCCharacterHeartbeat*,uint8_t,const UCCharacterHeartbeatServices*);
/*1720E0's retained state transform after real root lookup. The caller performs
 *the final transition and applies this actor168 suppression counter.*/
void uc_character_invulnerable(uint32_t*root_flags,int32_t*suppression,
 uint8_t enemy,uint8_t player,uint8_t reactions,uint8_t tazer);
#endif
