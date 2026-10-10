#include "visibility_query.h"
void uc_visibility_query_init(UCVisibilityQuery*q,const void*p){q->provider=p;q->next=1;}
int uc_visibility_query_begin(UCVisibilityQuery*q,uint8_t enabled,const UCVisibilityQueryHooks*h,void*c,uint32_t*out){
    uint32_t token;
    if(!q||!out||!q->provider||!h||!h->activate||!h->begin)return 0;
    if(!enabled){*out=0;return 1;}
    token=(uint32_t)(uint16_t)(q->next-1)+1;
    q->next=token%0x3F00+1;
    h->activate(c,q->provider);h->begin(c);*out=token;return 1;
}
int uc_visibility_query_end(const UCVisibilityQuery*q,uint8_t enabled,uint32_t token,const UCVisibilityQueryHooks*h,void*c){
    if(!q||!q->provider||!h||!h->activate||!h->end)return 0;
    if(enabled){h->activate(c,q->provider);h->end(c,(uint16_t)(token-1));}
    return 1;
}
int uc_visibility_query_result(const UCVisibilityQuery*q,uint8_t enabled,uint8_t stall,uint32_t token,const UCVisibilityQueryHooks*h,void*c,UCVisibilityQueryResult*out){
    uint32_t samples=0,status;
    if(!q||!q->provider||!h||!h->activate||!h->data||!out)return 0;
    if(!enabled){*out=(UCVisibilityQueryResult){2,0};return 1;}
    if(stall)do{h->activate(c,q->provider);status=h->data(c,(uint16_t)(token-1),&samples);}while(status);
    h->activate(c,q->provider);status=h->data(c,(uint16_t)(token-1),&samples);
    *out=status?(UCVisibilityQueryResult){2,0}:(UCVisibilityQueryResult){samples!=0,samples};
    return 1;
}
