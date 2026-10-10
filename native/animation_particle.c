#include "animation_particle.h"
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
static int type(const char *s){
    const char *n="simpleparticlesystem";
    for(;;s++,n++){unsigned c=(unsigned char)*s;if(c>='A'&&c<='Z')c+=32;if(c!=(unsigned char)*n)return 0;if(!c)return 1;}
}
static int matrix_ok(const float *m){if(!m)return 0;for(unsigned i=0;i<16;i++)if(!isfinite(m[i])||fabsf(m[i])>1e10f)return 0;return 1;}
int uc_animation_particle(const UCAnimationEvent *e,const void *entity,const int32_t *id,
    const UCAnimationParticleServices *h,uint8_t *handled){
    if(!e||!e->type||!handled)return 1;
    if(!type(e->type)){*handled=0;return 0;}
    if(!entity){*handled=1;return 0;}
    if(!id||!h||!h->world||!h->named_matrix||!h->attached||!h->fixed||!h->submit||
       e->parameter_count!=3||!e->parameters||!e->parameters[0].value||
       !e->parameters[1].value||!e->parameters[2].value||(_mm_getcsr()&0xe040))return 1;
    const char *file=e->parameters[0].value,*attach=e->parameters[1].value;
    if((attach[0]!='0'&&attach[0]!='1')||attach[1])return 1;
    const char *bone=e->parameters[2].value;void *source;
    if(attach[0]=='1')source=h->attached(h->context,*id,bone,1);
    else{
        float local[16],result[16];const float *world=h->world(h->context,entity);
        if(!matrix_ok(world))return 2;memcpy(local,world,sizeof local);
        if(*bone&&h->named_matrix(h->context,entity,bone,local)){
            world=h->world(h->context,entity);if(!matrix_ok(local)||!matrix_ok(world))return 2;
            for(unsigned i=0;i<4;i++)for(unsigned j=0;j<4;j++){
                float a=local[i*4]*world[j],b=local[i*4+1]*world[4+j];
                float c=local[i*4+2]*world[8+j],d=local[i*4+3]*world[12+j];
                result[i*4+j]=(a+b)+(c+d);
            }
            memcpy(local,result,sizeof local);
        }
        if(!matrix_ok(local))return 2;source=h->fixed(h->context,*id,local);
    }
    if(!source)return 2;h->submit(h->context,source,file);*handled=1;return 0;
}
