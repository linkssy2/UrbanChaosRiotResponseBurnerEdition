#include "mainplayer_muzzle.h"
int uc_mainplayer_muzzle_notify(const UCEntityEventRegistry*r,int32_t flare,
    uint32_t sender,uint32_t item,uint32_t event,uint8_t*handled){
    if(item!=2&&item!=17)return 1;
    UCMainplayerMuzzlePayload payload={item,0,{0,0,0}};
    return uc_entity_event_send(r,flare,event,&payload,sizeof payload,&sender,handled);
}
