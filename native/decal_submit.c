#include "decal_submit.h"
#include <string.h>
int uc_bullet_decal_submit(const char *shader,const float point[3],const float normal[3],
    float size,UCVectorRandom *random,const UCFootprintServices *h,uint8_t *submitted){
    if(!point||!normal||!random||!h||!submitted||!h->preflight||!h->query||!h->manager||!h->material_cache||!h->publish)return 1;
    if(!shader){*submitted=0;return 0;}
    UCPlayerFootprint request={0};request.shader=shader;memcpy(request.point,point,12);memcpy(request.normal,normal,12);
    request.size[0]=request.size[1]=size;request.mask=0x4000;
    if(!h->preflight(h->context,&request,0))return 1;
    float corners[4][3];if(uc_decal_quad(point,normal,request.size,random,corners))return 1;
    *submitted=0;
    for(unsigned i=0;i<4;i++){
        float start[3],end[3];for(unsigned j=0;j<3;j++){float delta=normal[j]*2.f;start[j]=delta+corners[i][j];end[j]=corners[i][j]-delta;}
        if(!h->query(h->context,start,end,1,65535,0x4000,0))return 0;
    }
    void *manager=h->manager(h->context);if(!manager)return 0;
    void *cache=h->material_cache(h->context);if(!cache)return 2;
    h->publish(h->context,manager,cache,shader,corners,0,0);*submitted=1;return 0;
}
