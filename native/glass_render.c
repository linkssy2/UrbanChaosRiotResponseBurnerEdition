#include "glass_render.h"
#include <string.h>
static int span(const UCGlassRender*p){return p&&p->count<=256&&(p->count<=0||p->triangles);}
static int hooks(const UCGlassRenderHooks*h){return h&&h->cull&&h->begin&&h->attribute&&h->end;}
int uc_glass_render_draw(const UCGlassRender*p,const UCGlassRenderHooks*h,void*c){
 if(!span(p)||!hooks(h))return 1;
 if(p->count>0){
  h->cull(c,0);h->begin(c,5);
  for(int32_t i=0;i<p->count;++i){
   const UCGlassTriangle*t=p->triangles+i;float color[4]={1,1,1,t->alpha};
   h->attribute(c,2,4,color);
   for(unsigned k=0;k<3;++k){
    const float normal[4]={0,0,1,1};float position[4];
    h->attribute(c,3,2,t->vertex[k].uv);h->attribute(c,1,4,normal);
    memcpy(position,t->vertex[k].position,12);position[3]=0;h->attribute(c,UINT32_MAX,4,position);
   }
  }
  h->end(c);h->cull(c,0x900);
 }
 return 0;
}
int uc_glass_render(UCGlassRender*p,const float identity[16],const UCGlassRenderHooks*h,void*c){
 float saved[16];void*owner;const float*m;
 if(!p||!p->source||!p->material||!identity||!hooks(h)||!h->triangles||!h->finish||!h->matrix_owner||!h->matrix||!h->set_matrix||!h->material)return 1;
 h->triangles(c,p->source,&p->triangles,&p->count);
 if(!span(p))return 2;
 if(p->count>0){
  owner=h->matrix_owner(c);if(!owner)return 2;m=h->matrix(c,owner);if(!m)return 2;memcpy(saved,m,sizeof saved);
  owner=h->matrix_owner(c);if(!owner)return 2;h->set_matrix(c,owner,identity);
  h->material(c,p->material,p);
  owner=h->matrix_owner(c);if(!owner)return 2;h->set_matrix(c,owner,saved);
 }
 h->finish(c,p->source);p->triangles=NULL;return 0;
}
