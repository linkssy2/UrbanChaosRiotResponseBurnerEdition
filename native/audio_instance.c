#include "audio_instance.h"
#include <string.h>
static void begin_link(UCAudioLink *p){p->next=p->previous=p;}
static int valid(const UCAudioLink *p){return p&&p->next&&p->previous&&p->next->previous==p&&p->previous->next==p;}
int uc_audio_instance_begin(UCAudioInstance *p){
    unsigned i;if(!p)return 1;memset(p,0,sizeof *p);
    for(i=0;i<6;i++)begin_link(&p->links[i]);
    p->references=1;p->event=p->index=UINT32_MAX;return 0;
}
int uc_audio_instance_track_begin(UCAudioInstanceTrack *p){
    if(!p)return 1;memset(p,0,sizeof *p);begin_link(&p->membership);begin_link(&p->commands);
    p->references=1;p->state.subsound=-1;
    p->parameters[3]=p->parameters[4]=p->parameters[5]=0x7fff;p->parameters[6]=0x1f40;return 0;
}
int uc_audio_link_append(UCAudioLink *head,UCAudioLink *node){
    if(!valid(head)||!node||node==head||node->next!=node||node->previous!=node)return 1;
    node->next=head;node->previous=head->previous;node->previous->next=node;head->previous=node;return 0;
}
int uc_audio_link_remove(UCAudioLink *node){
    if(!valid(node))return 1;node->next->previous=node->previous;node->previous->next=node->next;begin_link(node);return 0;
}
int uc_audio_instance_track_bind(UCAudioInstanceTrack *p,UCAudioInstance *parent,
                                void *bank,const uint8_t *definition,size_t bytes){
    if(!p||(definition&&bytes<5))return 1;p->parent=parent;p->definition=definition;p->bank=bank;
    if(definition&&(definition[4]&1))p->state.flags|=0x200;return 0;
}
int uc_audio_instance_state(UCAudioInstance *p,int32_t next,int32_t *previous,uint8_t *accepted){
    int32_t old;int allow=1;if(!p||!accepted)return 1;old=p->state;
    switch(next){
    case 0:allow=(old>=0&&old<=1)||old==6;break;
    case 2:allow=old!=2&&(old<=6||old>8);break;
    case 3:allow=old!=0&&(old<=6||old>8);break;
    case 4:allow=old<7||old>8;break;
    case 5:allow=old<6||old>8;break;
    case 6:allow=old!=5&&(old<=6||old>8);break;
    case 7:allow=old!=8;break;
    default:break;
    }
    if(allow){if(previous)*previous=old;p->state=next;}*accepted=(uint8_t)allow;return 0;
}
uint32_t uc_audio_instance_retain(UCAudioInstance *p){return ++p->references;}
int uc_audio_instance_release(UCAudioInstance *p,void (*destroy)(void *,UCAudioInstance *),void *ctx,uint32_t *remaining){
    uint32_t n;if(!p||!destroy||!remaining)return 1;n=--p->references;
    if(n==1&&(p->flags&1))p->references=n=0;
    if(!n){destroy(ctx,p);*remaining=0;}else *remaining=n;return 0;
}
int uc_audio_instance_attach(UCAudioLink *head,UCAudioInstance *p){
    if(!p)return 0;if(!valid(&p->links[5])||!valid(head)||head==&p->links[5])return 1;
    if(p->links[5].next==&p->links[5]){uc_audio_instance_retain(p);return uc_audio_link_append(head,&p->links[5]);}return 0;
}
int uc_audio_instance_detach(UCAudioInstance *p,void (*destroy)(void *,UCAudioInstance *),void *ctx,uint32_t *remaining){
    if(!p||!destroy||!remaining||!valid(&p->links[5]))return 1;
    uc_audio_link_remove(&p->links[5]);return uc_audio_instance_release(p,destroy,ctx,remaining);
}
int uc_audio_instance_bind_buffer(UCAudioInstance *p,void *buffer,const UCAudioInstanceBinding *h){
    void *old;if(!p)return 1;if(!buffer)return 0;
    if(!h||!h->preflight||!h->retain_buffer||!h->clear_buffer||!h->buffer_members||!h->release_buffer||
       !valid(&p->links[5])||!h->preflight(h->context,p,buffer))return 1;
    old=p->buffer;p->buffer=buffer;h->retain_buffer(h->context,buffer);
    h->clear_buffer(h->context,p->buffer);
    if(p->links[5].next==&p->links[5]){
        UCAudioLink *head=h->buffer_members(h->context,p->buffer);
        uc_audio_instance_retain(p);
        /* preflight guarantees the completing hooks leave this actual ring valid. */
        p->links[5].next=head;p->links[5].previous=head->previous;
        head->previous->next=&p->links[5];head->previous=&p->links[5];
    }
    if(old)h->release_buffer(h->context,old);return 0;
}
