#include "particle_quad_batch.h"
#include "particle_billboard.h"
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
int uc_particle_quad_batch(const UCParticleQuad *quads,uint32_t count,UCParticleQuadVertex *out){
    if(count>128||(_mm_getcsr()&0xE040u)||(count&&(!quads||!out)))return 1;
    for(uint32_t i=0;i<count;i++){
        for(unsigned j=0;j<4;j++){
            if(!isfinite(quads[i].color[j])||fabsf(quads[i].color[j])>65536)return 1;
            for(unsigned k=0;k<3;k++)if(!isfinite(quads[i].position[j][k]))return 1;
        }
    }
    static const unsigned corner[4]={0,1,3,2};
    static const float uv[4][2]={{1,0},{1,1},{0,1},{0,0}};
    for(uint32_t i=0;i<count;i++){
        uint32_t color;if(!uc_particle_color_pack(quads[i].color,&color))return 1;
        for(unsigned j=0;j<4;j++){
            UCParticleQuadVertex *v=out+i*4+j;
            memcpy(v->position,quads[i].position[corner[j]],sizeof v->position);
            v->color=color;memcpy(v->uv,uv[j],sizeof v->uv);
        }
    }
    return 0;
}
