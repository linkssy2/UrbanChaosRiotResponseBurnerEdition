#ifndef UC_CHARACTER_STEP_ASIDE_H
#define UC_CHARACTER_STEP_ASIDE_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCCharacterStepAside {float distance;int32_t target;uint8_t first;} UCCharacterStepAside;
typedef struct UCCharacterStepAsideServices {
 void *owner;
 const float*(*target)(void*,int32_t); /*live target world, null if retired*/
 uint8_t(*busy)(void*); /*innerA8+84*/
 const float*(*actor)(void*);
 uint8_t(*clear)(void*,const float[3]); /*32D60(point,0)*/
 void(*side)(void*,uint8_t left); /*inner98/9C and retained completion*/
 void(*move)(void*,const float[3]); /*inner80(point,0,0,completion)*/
 void(*notify)(void*,uint8_t moved); /*root source event4F9E00*/
 int32_t(*completion)(void*); /*-1 absent,0 pending,1 done,2 cancelled*/
 void(*exit)(void*);
} UCCharacterStepAsideServices;
/* Exact10AA70/10AF30 decision. Entry10A970 sets first=1. The source
 * radial fallback produces NaN for coincident points; the real clearance
 * owner must reject that domain without submitting invalid physics. */
int uc_character_step_aside_update(UCCharacterStepAside*,const UCCharacterStepAsideServices*);
#ifdef __cplusplus
}
#endif
#endif
