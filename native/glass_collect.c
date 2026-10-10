#include "glass_collect.h"
#include <math.h>
#include <xmmintrin.h>
static int finite16(const float *p){if(!p)return 0;for(unsigned i=0;i<16;i++)if(!isfinite(p[i]))return 0;return 1;}
int uc_glass_collect(const UCGlassRegistry *o,const UCSceneMatrixProvider *const *current,
 void (*append)(void *,const UCGlassRegistry *,uint32_t,uint32_t,float,uint32_t),void *context){
 if(!o||!current||!*current||!append||(_mm_getcsr()&0xE040u))return 1;
 const float *world=uc_glass_registry_matrix(o),*material=(*current)->material,*view=(*current)->view;float row[4];
 if(!finite16(world)||!finite16(material)||!finite16(view))return 1;
 for(unsigned j=0;j<4;j++){row[j]=(material[j]*world[12]+material[4+j]*world[13])+(material[8+j]*world[14]+material[12+j]*world[15]);if(!isfinite(row[j]))return 1;}
 float z=(view[2]*row[0]+view[6]*row[1])+(view[10]*row[2]+view[14]*row[3]);
 if(!isfinite(z))return 1;append(context,o,5,0x110,0.f-z,0);return 0;
}
