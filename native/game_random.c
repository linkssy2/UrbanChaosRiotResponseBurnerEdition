#include "game_random.h"
#include <string.h>
static const uint32_t seed[250]={
#include "game_random_seed.inc"
};
int uc_game_random_next(UCGameRandom *s,int32_t *out){
    if(!s||!out||s->first>=250||s->second>=250)return 1;
    s->first=s->first==249?0:s->first+1;
    s->second=s->second==249?0:s->second+1;
    s->words[s->first]^=s->words[s->second];
    memcpy(out,s->words+s->first,sizeof(*out));return 0;
}
int uc_game_random_init(UCGameRandom *s,uint8_t warmup){
    if(!s)return 1;
    memcpy(s->words,seed,sizeof(seed));s->first=0;s->second=147;
    for(unsigned i=0;i<warmup;i++){int32_t discard;uc_game_random_next(s,&discard);}
    return 0;
}
int uc_game_random_range(UCGameRandom*s,int32_t first,int32_t last,int32_t*out){
    if(!s||!out||s->first>=250||s->second>=250)return 1;
    uint32_t bits=(uint32_t)last-(uint32_t)first;int32_t divisor;
    memcpy(&divisor,&bits,4);if(!divisor)return 1;
    unsigned a=s->first==249?0:s->first+1,b=s->second==249?0:s->second+1;
    if(divisor==-1&&(s->words[a]^s->words[b])==0x80000000u)return 1;
    int32_t word;uc_game_random_next(s,&word);
    bits=(uint32_t)(word%divisor)+(uint32_t)first;memcpy(out,&bits,4);return 0;
}
int uc_game_random_boolean(UCGameRandom*s,int32_t*out){
    if(!out)return 1;int32_t word;if(uc_game_random_next(s,&word))return 1;
    float value=(float)word;value*=0x1p-31f;*out=value>0.5f;return 0;
}
