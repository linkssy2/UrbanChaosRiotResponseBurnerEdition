#include "particle_attachment.h"
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
int uc_particle_attachment_bind(const UCParticleAttachmentServices*h,int32_t id,const char*name,uint8_t hidden,UCParticleAttachment*out){
    if(!h||!h->registry||!h->find_bone||!name||!out)return 1;
    UCEntityEventTarget*t=uc_entity_event_lookup(h->registry,id);if(!t)return 1;
    int32_t bone=h->find_bone(h->owner,t,name);
    out->entity=id;out->bone=bone;out->dead_when_hidden=hidden;return 0;
}
int uc_particle_attachment_dead(const UCParticleAttachmentServices*h,const UCParticleAttachment*s,uint8_t*out){
    if(!h||!h->registry||!h->flags||!s||!out)return 1;
    UCEntityEventTarget*t=uc_entity_event_lookup(h->registry,s->entity);
    uint8_t dead=t==0;if(t&&s->dead_when_hidden){const uint32_t*f=h->flags(h->owner,t);if(!f)return 2;dead=(*f&0x20)!=0;}
    *out=dead;return 0;
}
int uc_particle_attachment_matrix(const UCParticleAttachmentServices*h,const UCParticleAttachment*s,float*out){
    static const float identity[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    if(!h||!h->registry||!h->bone_matrix||!h->world||!s||!out||(_mm_getcsr()&0xE040u))return 1;
    UCEntityEventTarget*t=uc_entity_event_lookup(h->registry,s->entity);float bone[16],value[16];
    if(t&&s->bone!=-1&&h->bone_matrix(h->owner,t,s->bone,bone)){
        const float*w=h->world(h->owner,t);if(!w)return 2;
        for(unsigned i=0;i<16;i++)if(!isfinite(bone[i])||!isfinite(w[i]))return 2;
        for(unsigned r=0;r<4;r++)for(unsigned c=0;c<4;c++){
            unsigned row=r==1?2:r==2?1:r;const float*b=bone+row*4;
            float p0=b[0]*w[c],p1=b[1]*w[4+c],p2=b[2]*w[8+c],p3=b[3]*w[12+c];value[r*4+c]=(p0+p1)+(p2+p3);
            if(!isfinite(value[r*4+c]))return 2;
        }
    }else memcpy(value,identity,64);
    memcpy(out,value,64);return 0;
}
