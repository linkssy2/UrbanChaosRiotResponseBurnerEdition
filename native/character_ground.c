#include "character_ground.h"
#include <string.h>
int uc_character_ground(const float p[3],uint32_t group,const UCCharacterGroundServices *s,
    float *height,float normal[3],uint8_t *found){
    float from[3],to[3],point[3],hit_normal[3];int hit;
    if(!p||!s||!s->ray||!height)return 1;
    from[0]=to[0]=p[0];from[2]=to[2]=p[2];from[1]=p[1]+100.f;to[1]=from[1]-99999.f;
    hit=s->ray(s->owner,from,to,0x11,group,0xFFFF,0,point,hit_normal);
    if(hit<0||hit>1)return 2;
    if(hit){*height=point[1];if(normal)memcpy(normal,hit_normal,12);}
    else{*height=from[1];if(normal){normal[0]=0;normal[1]=1;normal[2]=0;}}
    if(found)*found=(uint8_t)hit;return 0;
}
