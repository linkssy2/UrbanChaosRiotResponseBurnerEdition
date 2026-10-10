#include "scene_model_pass.h"
#include <math.h>
#include <string.h>
static void tint(const float color[4],const UCSceneModelPassState*s,const UCSceneModelPassHooks*h,void*c){
    *s->debug=0;*s->translucent=(uint8_t)(.999f>color[3]);
    for(unsigned i=0;i<4;++i)if(s->tint[i]!=color[i]){memcpy(s->tint,color,16);h->constant125(c,color);break;}
}
int uc_scene_model_pass(UCSceneModelRender*m,const float color[4],const UCSceneModelPass*p,
    const UCSceneModelPassState*s,const UCSceneModelPassHooks*h,void*c){
    static const float white[4]={1,1,1,1},zero[4]={0,0,0,0};
    if(!m||m->indexed!=1||m->modifier||(m->flags&0x1B0u)||!color||!p||!s||!s->tint||
       !s->translucent||!s->debug||s->translucent==s->debug||!s->method||!h||!h->constant125||!h->method||
       ((p->mode==0||p->mode==6)&&!h->depth)||(p->mode==2&&!h->base)||
       (p->mode==4&&!h->projected)||(p->mode==5&&!h->masked)||(p->mode==3&&!h->light))return 0;
    for(unsigned i=0;i<4;++i)if(!isfinite(color[i])||!isfinite(s->tint[i])||!isfinite(p->color[i]))return 0;
    tint(color,s,h,c);h->method(c,16);*s->method=16;
    switch(p->mode){
    case 0:h->depth(c,m,p->query,p->partition,0,p->color);break;
    case 6:if(!(m->flags&8u))h->depth(c,m,0,p->partition,1,zero);break;
    case 2:h->base(c,m,p->partition);break;
    case 4:h->projected(c,m,p->light,p->partition);break;
    case 5:h->masked(c,m,p->light,p->partition);break;
    case 3:h->light(c,m,p->light,p->partition);break;
    default:break;
    }
    tint(white,s,h,c);h->method(c,1);*s->method=1;return 1;
}
