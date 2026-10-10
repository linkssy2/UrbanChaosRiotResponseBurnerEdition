#include "cutscene_camera.h"
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
static int finite_matrix(const float *m){if(!m)return 0;for(unsigned i=0;i<16;i++)if(!isfinite(m[i]))return 0;return 1;}
/*21B0D0's actual once-initialized axis matrix. Preserve both multiplications
 * and their ADDPS grouping, including signed zero, rather than permuting rows. */
static const float axes[16]={1,0,0,0, 0,0,1,0, 0,1,0,0, 0,0,0,1};
static void multiply(float *out,const float *a,const float *b){
    for(unsigned r=0;r<4;r++)for(unsigned j=0;j<4;j++){
        float x=b[j]*a[r*4],y=b[j+4]*a[r*4+1];
        float z=b[j+8]*a[r*4+2],w=b[j+12]*a[r*4+3];
        out[r*4+j]=(x+y)+(z+w);
    }
}
int uc_cutscene_camera_late(const UCEntityFreshView*v,UCCutsceneCamera*c,const UCCutsceneCameraServices*s){
    if(!v||!c||!s||!v->transform||!v->target.parent_id||!s->registry||!s->names||
       !s->preflight||!s->current_provider||!s->player||!s->control||!s->activate||
       !s->mode||!s->bone_matrix||!s->parent_matrix||!s->remove||
       (_mm_getcsr()&0xE040)||!finite_matrix(v->transform)||s->preflight(s->owner))return 1;
    if(c->pending){
        if(c->auto_focus&&!c->camera_seen){
            const int32_t *ids=NULL;uint32_t count=0;
            if(uc_entity_names_view(s->names,"CAMERA",&ids,&count))return 2;
            if(count&&ids[0]!=-1)c->camera_seen=1;
        }
        if(c->pending&&c->camera_seen&&s->current_provider(s->owner)){
            int32_t index=c->player_index,id;void *p=s->current_provider(s->owner);
            if(!p||s->player(s->owner,p,index,&id))return 2;
            if(c->control_player&&s->control(s->owner,id))return 2;
            if(s->activate(s->owner,id))return 2;
            c->pending=0;
        }
    }
    void *p=s->current_provider(s->owner);
    if(!p||s->mode(s->owner,p,6,0))return 2;
    if(!c->bone)return 0;
    UCEntityEventTarget *parent=uc_entity_event_lookup(s->registry,*v->target.parent_id);
    if(!parent)return s->remove(s->owner)?2:0;
    float local[16],adjusted[16],world[16];uint8_t found=0;
    if(s->bone_matrix(s->owner,parent,c->bone,local,&found))return 2;
    if(!found)return 0;
    if(!finite_matrix(local))return 2;
    multiply(adjusted,axes,local);
    const float *matrix=s->parent_matrix(s->owner,parent);
    if(!finite_matrix(matrix)||!finite_matrix(adjusted))return 2;
    multiply(world,adjusted,matrix);if(!finite_matrix(world))return 2;
    memcpy(v->transform,world,sizeof world);return 0;
}
int uc_cutscene_camera_bone(UCCutsceneCamera*c,const void*p){if(!c||!p)return 1;memcpy(&c->bone,p,4);return 0;}
int uc_cutscene_camera_fov(UCSceneProjection*c,const void*p){if(!c||!p)return 1;memcpy(&c->fov,p,4);return 0;}
