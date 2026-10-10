#include "mainplayer_camera_bind.h"
int uc_mainplayer_camera_bind(int32_t id,int32_t *slot,const int32_t *player,
    const uint32_t *source,const uint32_t *activate,void *owner,
    const UCEntityEventRegistry *(*current)(void *)) {
    if(!slot||(id!=-1&&(!player||!source||!activate||!current)))return 1;
    *slot=id;if(id==-1)return 0;
    uint32_t sender=(uint32_t)*player,tag=0;uint8_t handled;
    const UCEntityEventRegistry *registry=current(owner);
    if(uc_entity_event_send(registry,id,*source,&tag,4,&sender,&handled))return 2;
    int32_t payload=*player;id=*slot;sender=(uint32_t)payload;
    registry=current(owner);
    return uc_entity_event_send(registry,id,*activate,&payload,4,&sender,&handled)?2:0;
}
