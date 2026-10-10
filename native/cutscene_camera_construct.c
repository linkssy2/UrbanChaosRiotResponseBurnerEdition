#include "cutscene_camera_construct.h"
#include <math.h>
#include <xmmintrin.h>
static int get(const UCEntityProperties*p,unsigned t,const char*n,void*out){int found;return uc_entity_properties_get(p,t,n,out,&found);}
int uc_cutscene_camera_construct(const UCEntityFreshView*v,UCCutsceneCamera*c,UCSceneProjection*p,UCCutsceneCameraListener*l,
 const UCEntityProperties*d,const UCCutsceneCameraConstructServices*h){
 if(!v||!v->target.identity||!c||!p||!l||!d||!h||!h->preflight||!h->base||
    !h->register_listener||!h->append||!h->bone_event||!h->fov_event||(_mm_getcsr()&0xE040))return 1;
 float fov=65,near_z=25;int32_t bone=0,index=0;uint32_t automatic=1,control=1,override_near=0;
 if(get(d,2,"fovCurrent",&fov)||get(d,1,"Bone",&bone)||get(d,3,"auto take focus",&automatic)||
    get(d,3,"take control",&control)||get(d,1,"Player Num",&index)||
    get(d,3,"Set near plane",&override_near)||get(d,2,"Near plane",&near_z)||
    !isfinite(fov)||!isfinite(near_z)||((override_near&255)&&near_z<=0)||h->preflight(h->owner))return 1;
 if(h->base(h->owner))return 2;
 l->owner=NULL;l->count=l->capacity=0;
 p->fov=fov;c->bone=bone;c->auto_focus=(automatic&255)!=0;
 c->control_player=(control&255)!=0;c->player_index=index;
 if(override_near&255){
    float ratio=(p->parameters.far_z*p->parameters.near_z)/near_z;
    if(!isfinite(ratio))return 2;
    p->parameters.near_z=near_z;p->parameters.far_z=ratio;
 }
 c->pending=1;c->camera_seen=0;l->owner=v->target.identity;
 if(h->register_listener(h->owner))return 2;
 if(h->append(h->owner,*h->bone_event,0)||h->append(h->owner,*h->fov_event,1))return 2;
 return 0;
}
