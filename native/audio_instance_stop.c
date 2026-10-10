#include "audio_instance_stop.h"
#include <stddef.h>
static UCAudioInstanceTrack *track(UCAudioLink*p){return (UCAudioInstanceTrack*)((char*)p - offsetof(UCAudioInstanceTrack,membership));}
static int valid(const UCAudioInstance*p){
    const UCAudioLink *head=&p->links[4],*node=head->next,*previous=head;unsigned n=0;
    if(!node||!head->previous)return 0;
    while(node!=head){if(!node||node->previous!=previous||++n>4096||track((UCAudioLink*)node)->parent!=p)return 0;previous=node;node=node->next;}
    return head->previous==previous;
}
int uc_audio_instance_stop(UCAudioInstance*p,uint32_t flags,const UCAudioInstanceStopServices*h,uint8_t*accepted){
    UCAudioLink *head,*node;uint8_t changed;unsigned visits=0;
    if(!p||!accepted||(flags&0x10000))return 1;
    if(p->state==8){*accepted=0;return 0;}
    if(!h||!h->preflight||!h->cancel_track||!h->stop_voice||!h->update_instance||!valid(p)||!h->preflight(h->context,p,flags))return 1;
    uc_audio_instance_state(p,7,NULL,&changed);p->flags|=flags;head=&p->links[4];node=head->next;
    while(node!=head){
        if(++visits>4096)return 2;UCAudioInstanceTrack*t=track(node);
        if(h->cancel_track(h->context,t))return 2;
        void*voice=t->voice;t->state.flags=(uint16_t)((t->state.flags&0xffdf)|4);
        if(voice&&h->stop_voice(h->context,voice,0x40))return 2;
        node=node->next;
    }
    if((flags&2)&&h->update_instance(h->context,p,UINT32_MAX))return 2;
    *accepted=1;return 0;
}
