#include "particle_billboard_batch.h"
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
static int batch(const UCParticleSprite *sprites,uint32_t count,UCParticleBillboard *out,int heat){
    if(count>39||(_mm_getcsr()&0xE040u)||(count&&(!sprites||!out)))return 1;
    for(uint32_t i=0;i<count;i++){
        const UCParticleSprite *p=&sprites[i];
        if(!isfinite(p->radius)||!isfinite(p->angle)||fabsf(p->angle)>65536)return 1;
        for(unsigned k=0;k<3;k++)if(!isfinite(p->center[k]))return 1;
        for(unsigned k=0;k<4;k++)if(!isfinite(p->color[k])||fabsf(p->color[k])>65536)return 1;
        if(heat&&fabsf(p->color[2]*p->color[3])>65536)return 1;
    }
    for(uint32_t i=0;i<count;i++){
        const UCParticleSprite *p=&sprites[i];UCParticleBillboard b;
        memcpy(b.center,p->center,sizeof b.center);b.radius=p->radius;
        b.sine=(float)sin((double)p->angle);b.cosine=(float)cos((double)p->angle);
        float color[4];memcpy(color,p->color,sizeof color);
        if(heat)color[2]=color[3]*color[2];
        if(!uc_particle_color_pack(color,&b.color))return 1;out[i]=b;
    }
    return 0;
}
int uc_particle_billboard_batch(const UCParticleSprite *sprites,uint32_t count,UCParticleBillboard *out){return batch(sprites,count,out,0);}
int uc_particle_billboard_heat_batch(const UCParticleSprite *sprites,uint32_t count,UCParticleBillboard *out){return batch(sprites,count,out,1);}
