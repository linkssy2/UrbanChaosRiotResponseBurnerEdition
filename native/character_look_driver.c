#include "character_look_driver.h"
int uc_character_look_aim_point(UCCharacterLook*s,UCGameRandom*r,const UCCharacterLookServices*h,
    int32_t self,uint32_t frame,float delta,int32_t order,uint8_t holding,float*out){
    if(!s||!r||!h||!h->names||!out||holding>1)return 1;
    if(order!=-1&&holding){const int32_t*ids;uint32_t count=0;
        if(uc_entity_names_view(h->names,"EXTREMELY_INTERESTING_LOOK",&ids,&count))return 1;
        if(!count&&uc_character_look_target(s,r,h->player))return 1;
    }
    return uc_character_look_step(s,r,h,self,frame,delta,out);
}
