#include "decal_scenery.h"
typedef struct Submission {const UCDecalScenery *owner;int failed;} Submission;
static int preflight(void *p,const UCPlayerFootprint *request,float fade){(void)p;(void)request;return fade==0;}
static const UCCollisionResult *query(void *p,const float start[3],const float end[3],uint32_t type,uint32_t group,uint32_t mask,float impulse){
    Submission *s=p;const UCCollisionResult *result=NULL;
    if(uc_collision_scenery_trace(&s->owner->trace,start,end,type,group,mask,impulse,&result)<0)s->failed=2;
    return result;
}
static void *manager(void *p){return ((Submission*)p)->owner->ring;}
static void *cache(void *p){return ((Submission*)p)->owner->resources;}
static int ready(void *p,const char *name,const float corners[4][3]){(void)p;(void)name;(void)corners;return 1;}
static int material(void *p,const char *name,void **out){
    const UCMaterialResource *m=NULL;
    if(uc_material_resources_get(((Submission*)p)->owner->resources,name,&m))return 1;
    *out=(void*)m;return 0;
}
static int light(void *p,const float point[3],uint32_t *argb){
    const UCDecalScenery *o=((Submission*)p)->owner;
    return uc_scene_ambient_query(*o->ambient,o->lights,o->light_count,point,argb);
}
static void publish(void *p,void *ring,void *resources,const char *name,const float corners[4][3],uint32_t flags,float fade){
    Submission *s=p;const UCDecalRingServices h={p,ready,material,light};
    if(resources!=s->owner->resources||uc_decal_ring_insert(ring,name,corners,flags,fade,&h))s->failed=2;
}
int uc_decal_scenery_submit(const UCDecalScenery *o,const char *shader,const float point[3],const float normal[3],float size,UCVectorRandom *random,uint8_t *submitted){
    if(!o||!o->trace.queries||!o->trace.hits||(o->ring&&(!o->resources||!o->ambient||(o->light_count&&!o->lights))))return 1;
    Submission s={o,0};const UCFootprintServices h={&s,preflight,query,manager,cache,publish};
    int result=uc_bullet_decal_submit(shader,point,normal,size,random,&h,submitted);
    return s.failed?s.failed:result;
}
