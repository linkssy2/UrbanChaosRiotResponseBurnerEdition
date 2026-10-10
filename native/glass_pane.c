#include "glass_pane.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <xmmintrin.h>
typedef struct Sink{UCGlassBulletSink bullet;UCGlassShards shards;UCGlassStore*air;}Sink;
struct UCGlassPane{UCGlassTriangle triangles[2];float quad[4][2],world[16];uint8_t triggered;const UCGlassBulletSink*sink;Sink*owned;};
static int finite(const float*p,unsigned n){if(!p)return 0;for(unsigned i=0;i<n;i++)if(!isfinite(p[i]))return 0;return 1;}
static int ready(void*c,const float q[4][2],const float impact[6]){(void)q;(void)impact;return !uc_glass_store_valid(((Sink*)c)->air);}
static int shard(void*c,const UCGlassShard*p){uint32_t accepted;return uc_glass_store_submit(((Sink*)c)->air,p,1,&accepted);}
static int admit(void*c,const float q[4][2],const float impact[6]){for(unsigned i=0;i<8;i++)if(!isfinite(q[i/2][i%2])||fabsf(q[i/2][i%2])>4096)return 0;for(unsigned i=0;i<6;i++)if(!isfinite(impact[i])||fabsf(impact[i])>1e6f)return 0;return !ready(c,q,impact);}
static int emit(void*c,const float q[4][2],const float impact[6]){return uc_glass_shards_emit(&((Sink*)c)->shards,q,impact);}
UCGlassPane*uc_glass_pane_create(UCGlassPaneUV*table,const float world[16],const float quad[4][2],UCGlassStore*air,UCVectorRandom*vectors,UCGameRandom*random){
 if(!table||!quad||!finite(world,16)||!finite(quad[0],8)||!air||!uc_glass_store_valid(air)||!air->ground||!vectors||!random||(_mm_getcsr()&0xE040u))return NULL;
 if(vectors->first>=28||vectors->second>=28||random->first>=250||random->second>=250)return NULL;for(unsigned i=0;i<112;i++)if(!isfinite(vectors->values[i])||vectors->values[i]<0||vectors->values[i]>4000000)return NULL;
 UCGlassPaneUV uv=*table;if(!(uv.initialized&1)){uv.initialized|=1;const float values[4][2]={{0,0},{1,0},{1,1},{0,1}};memcpy(uv.uv,values,sizeof values);}if(!finite(uv.uv[0],8))return NULL;
 UCGlassTriangle triangles[2];const unsigned order[2][3]={{0,1,2},{0,2,3}};
 for(unsigned t=0;t<2;t++){triangles[t].alpha=1;for(unsigned k=0;k<3;k++){unsigned n=order[t][k];for(unsigned j=0;j<3;j++)triangles[t].vertex[k].position[j]=(world[j]*quad[n][0]+world[4+j]*quad[n][1])+(world[8+j]*0.f+world[12+j]);if(!finite(triangles[t].vertex[k].position,3))return NULL;memcpy(triangles[t].vertex[k].uv,uv.uv[n],8);}}
 UCGlassPane*p=malloc(sizeof*p);if(!p)return NULL;Sink*s=malloc(sizeof*s);if(!s){free(p);return NULL;}
 s->air=air;s->shards=(UCGlassShards){vectors,random,s,ready,shard};s->bullet=(UCGlassBulletSink){s,admit,emit};p->owned=s;p->sink=&s->bullet;p->triggered=0;memcpy(p->world,world,64);memcpy(p->quad,quad,32);*table=uv;memcpy(p->triangles,triangles,sizeof triangles);return p;
}
int uc_glass_pane_triangles(const UCGlassPane*p,const UCGlassTriangle**triangles,int32_t*count){if(!p||!triangles||!count)return 1;if(p->triggered)*count=0;else{*triangles=p->triangles;*count=2;}return 0;}
UCGlassBullet uc_glass_pane_bullet(UCGlassPane*p){return p?(UCGlassBullet){p->world,p->quad,&p->triggered,&p->sink}:(UCGlassBullet){0};}
int uc_glass_pane_destroy(UCGlassPane*p){if(!p||!p->owned||uc_glass_store_destroy(p->owned->air))return 1;free(p->owned->air);free(p->owned);free(p);return 0;}
