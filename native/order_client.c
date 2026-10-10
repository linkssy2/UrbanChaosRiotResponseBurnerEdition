#include "order_client.h"
#include <string.h>
void uc_order_client_init(UCOrderClient*s){if(s){s->manager=-1;s->pending=-1;s->advertise=1;}}
int uc_order_client_event(UCOrderClient*s,const UCOrderClientServices*h,uint32_t event,const void*payload,int32_t sender){
 if(!s||!h)return 1;
 switch(event){
 case 127:s->pending=sender;return 0;
 case 130:s->manager=sender;s->advertise=0;return h->attached&&!h->attached(h->owner)?0:2;
 case 126:{int32_t command;if(!payload)return 1;memcpy(&command,payload,4);return h->choose&&!h->choose(h->owner,command)?0:2;}
 case 136:return h->completed&&!h->completed(h->owner)?0:2;
 default:return 1;
 }
}
int uc_order_client_update(UCOrderClient*s,const UCOrderClientServices*h,int32_t id,int32_t parent){
 if(!s||!h)return 1;uint8_t alive=0;
 if(!h->alive||h->alive(h->owner,parent,&alive))return 2;
 if(!alive)return h->remove&&!h->remove(h->owner)?0:2;
 if(s->manager==-1&&s->pending!=-1){
  float position[3];uint8_t found=0;s->advertise=0;
  if(!h->position||h->position(h->owner,s->pending,position,&found))return 2;
  if(found){uint8_t accepted=0;
   if(!h->accept||h->accept(h->owner,s->pending,position,&accepted))return 2;
   accepted=accepted!=0;
   if(!h->send||h->send(h->owner,s->pending,129,&accepted,1,id))return 2;
  }
  s->pending=-1;
 }
 if(s->advertise){const uint32_t zero=0;if(!h->broadcast||h->broadcast(h->owner,125,&zero,4,id))return 2;}
 return 0;
}
int uc_order_client_complete(const UCOrderClient*s,const UCOrderClientServices*h,int32_t id,int32_t command){
 if(!s||!h)return 1;return h->send&&!h->send(h->owner,s->manager,136,&command,4,id)?0:2;
}
int uc_order_client_remove(const UCOrderClient*s,const UCOrderClientServices*h,int32_t id){
 if(!s||!h)return 1;
 if(s->manager!=-1){const uint32_t zero=0;if(!h->send||h->send(h->owner,s->manager,132,&zero,4,id))return 2;}
 return h->retire&&!h->retire(h->owner)?0:2;
}
