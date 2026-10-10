#include "game_difficulty.h"
#include <math.h>
#include <fenv.h>
#include <xmmintrin.h>
void uc_game_difficulty_fresh(UCGameDifficulty*s){if(s)*s=(UCGameDifficulty){1,.5f,1};}
int uc_game_difficulty_value(const UCGameDifficulty*s,double*out){
    static const float low[]={0,.2f,.5f,.8f},high[]={.5f,.8f,1,1};
    if(!s||!out||s->mode>3||!isfinite(s->adaptive)||!isfinite(s->scale)||
        fabsf(s->adaptive)>1e6f||fabsf(s->scale)>1e6f||fegetround()!=FE_TONEAREST||(_mm_getcsr()&0xE040u))return 1;
    float correction=(1.f-s->scale)*.125f;
    float value=s->adaptive-correction;if(!(value>0))value=0;
    *out=((double)high[s->mode]-low[s->mode])*value+low[s->mode];return 0;
}
int uc_game_difficulty_checkpoint(UCGameDifficulty*s,uint8_t disabled){
 static const float rate[]={.5f,1.f,1.6f,2.f};
 if(!s||s->mode>3||!isfinite(s->adaptive))return 1;if(disabled)return 0;
 float value=rate[s->mode]*.125f;value+=s->adaptive;
 if(value>=1.f)value=1.f;else if(!(value>0))value=0;
 s->adaptive=value;return 0;
}
