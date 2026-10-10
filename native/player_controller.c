#include "player_controller.h"
#include <math.h>
#include <string.h>

/* Original authored parameters; contact margins/iteration policy below belong
 * to this native replacement and are qualified independently of Havok. */
#define STEP (1.0f/30.0f)
#define GRAVITY (-17.0f)
#define SUPPORT_Z 0.4226182617f /* cos(65 degrees), 2A4AA0 config. */
#define CONTACTS 256
#define MOTION_STEP 0.045f
#define SEPARATION 0.0001f

float uc_player_eye_height(float current,UCPlayerEyeState state,float dt) {
    float target=state==UC_PLAYER_EYE_STANDING?170.0f:state==UC_PLAYER_EYE_CROUCH_IDLE?100.0f:115.0f;
    float difference=target-current;
    if(difference<0.0001f&&difference>-0.0001f)return target;
    float alpha=(state==UC_PLAYER_EYE_STANDING?5.0f:7.0f)*dt;
    if(alpha>1.0f)alpha=1.0f;
    return current+alpha*difference;
}

void uc_player_reset(UCPlayer *p,const float position[3]) {
    if(!p||!position)return;memset(p,0,sizeof(*p));memcpy(p->position,position,12);p->support_triangle=UINT32_MAX;
}
static size_t query(const UCCollisionAsset *a,const UCSolidBlockGeometry *const *blocks,size_t block_count,const UCPlayer *p,const float shape[3],float extra,UCCollisionContact contacts[CONTACTS]) {
    float lo[3]={p->position[0],p->position[1],p->position[2]+shape[0]};
    float hi[3]={lo[0],lo[1],p->position[2]+shape[1]};
    size_t count=uc_collision_capsule_contacts(a,lo,hi,shape[2]+extra,contacts,CONTACTS);
    if(count>CONTACTS)return count;
    for(size_t i=0;i<block_count;i++){
        UCCollisionContact hit;int status=uc_collision_block_contact(blocks[i],lo,hi,shape[2]+extra,&hit);
        if(status<0)return SIZE_MAX;if(status){if(count==CONTACTS)return CONTACTS+1;contacts[count++]=hit;}
    }
    return count;
}
static UCPlayerStatus resolve(const UCCollisionAsset *a,const UCSolidBlockGeometry *const *blocks,size_t block_count,UCPlayer *p,const float shape[3]) {
    UCCollisionContact contacts[CONTACTS];
    for(unsigned iteration=0;iteration<12;iteration++) {
        size_t count=query(a,blocks,block_count,p,shape,0,contacts);if(count>CONTACTS)return UC_PLAYER_CONTACT_LIMIT;
        const UCCollisionContact *deepest=NULL;
        for(size_t i=0;i<count;i++)if(contacts[i].penetration>SEPARATION&&(!deepest||contacts[i].penetration>deepest->penetration))deepest=&contacts[i];
        if(!deepest)return UC_PLAYER_OK;
        float normal[3];memcpy(normal,deepest->normal,12);float distance=deepest->penetration+SEPARATION;
        /* Steep upward faces block horizontal motion. Separating horizontally
         * by depth/|normalXY| avoids manufacturing uphill velocity each frame. */
        if(normal[2]>0&&normal[2]<SUPPORT_Z) {
            float horizontal=sqrtf(normal[0]*normal[0]+normal[1]*normal[1]);
            distance/=horizontal;normal[0]/=horizontal;normal[1]/=horizontal;normal[2]=0;
        }
        float into=0;
        for(unsigned i=0;i<3;i++){p->position[i]+=normal[i]*distance;into+=p->velocity[i]*normal[i];}
        if(into<0)for(unsigned i=0;i<3;i++)p->velocity[i]-=into*normal[i];
    }
    return UC_PLAYER_STUCK;
}
static UCPlayerStatus step_motion(const UCCollisionAsset *a,const UCSolidBlockGeometry *const *blocks,size_t block_count,UCPlayer p,const float shape[3],int resting,int gravity,UCPlayer *player) {
    UCCollisionContact contacts[CONTACTS];
    /* Static support at rest prevents gravity projection from manufacturing
     * sideways drift. This is replacement policy, not a Havok friction model. */
    if(resting)p.velocity[2]=0;
    /* Full-step gravity bounds displacement before subdivision. */
    float final_z=p.velocity[2]+(resting||!gravity?0:GRAVITY*STEP);
    float bound_z=fmaxf(fabsf(p.velocity[2]),fabsf(final_z));
    float travel=STEP*sqrtf(p.velocity[0]*p.velocity[0]+p.velocity[1]*p.velocity[1]+bound_z*bound_z);
    if(!isfinite(travel)||travel>128*MOTION_STEP)return UC_PLAYER_STUCK;
    unsigned steps=(unsigned)ceilf(travel/MOTION_STEP);if(!steps)steps=1;
    float dt=STEP/(float)steps;p.grounded=0;p.support_triangle=UINT32_MAX;
    for(unsigned step=0;step<steps;step++) {
        if(!resting&&gravity)p.velocity[2]+=GRAVITY*dt;
        for(unsigned i=0;i<3;i++)p.position[i]+=p.velocity[i]*dt;
        UCPlayerStatus status=resolve(a,blocks,block_count,&p,shape);if(status!=UC_PLAYER_OK)return status;
    }
    size_t count=query(a,blocks,block_count,&p,shape,.015f,contacts);if(count>CONTACTS)return UC_PLAYER_CONTACT_LIMIT;
    float support=SUPPORT_Z;
    for(size_t i=0;i<count;i++)if(contacts[i].normal[2]>=support) {
        p.grounded=1;p.support_triangle=contacts[i].triangle;support=contacts[i].normal[2];
    }
    if(p.grounded&&p.velocity[2]<0)p.velocity[2]=0;
    *player=p;return UC_PLAYER_OK;
}
UCPlayerStatus uc_player_step(const UCCollisionAsset *a,const UCPlayerInput *in,UCPlayer *player) {
    return uc_player_step_blocks(a,NULL,0,in,player);
}
UCPlayerStatus uc_player_step_blocks(const UCCollisionAsset *a,const UCSolidBlockGeometry *const *blocks,size_t block_count,const UCPlayerInput *in,UCPlayer *player) {
    if(!a||!in||!player||!isfinite(in->forward)||!isfinite(in->right)||!isfinite(in->yaw)||!isfinite(in->speed)||in->speed<0||in->speed>50||fabsf(in->forward)>1||fabsf(in->right)>1)return UC_PLAYER_ARGUMENT;
    for(unsigned i=0;i<3;i++)if(!isfinite(player->position[i])||!isfinite(player->velocity[i]))return UC_PLAYER_ARGUMENT;
    if(block_count>4096||(block_count&&!blocks))return UC_PLAYER_ARGUMENT;
    for(size_t i=0;i<block_count;i++){UCCollisionContact ignored;if(uc_collision_block_contact(blocks[i],player->position,player->position,0,&ignored)<0)return UC_PLAYER_ARGUMENT;}
    UCPlayer p=*player;UCCollisionContact contacts[CONTACTS];
    if(in->crouch)p.crouched=1;
    else if(p.crouched) {
        p.crouched=0;const float standing[3]={UC_PLAYER_CAPSULE_BOTTOM,UC_PLAYER_CAPSULE_STANDING_TOP,UC_PLAYER_CAPSULE_RADIUS};
        size_t count=query(a,blocks,block_count,&p,standing,0,contacts);if(count>CONTACTS)return UC_PLAYER_CONTACT_LIMIT;
        for(size_t i=0;i<count;i++)if(contacts[i].penetration>SEPARATION){p.crouched=1;break;}
    }
    float forward=in->forward,right=in->right,length=sqrtf(forward*forward+right*right);
    if(length>1){forward/=length;right/=length;}
    float s=sinf(in->yaw),c=cosf(in->yaw);
    p.velocity[0]=(-s*forward+c*right)*in->speed;
    p.velocity[1]=(c*forward+s*right)*in->speed;
    const float shape[3]={UC_PLAYER_CAPSULE_BOTTOM,p.crouched?UC_PLAYER_CAPSULE_CROUCH_TOP:UC_PLAYER_CAPSULE_STANDING_TOP,UC_PLAYER_CAPSULE_RADIUS};
    return step_motion(a,blocks,block_count,p,shape,p.grounded&&length==0,1,player);
}

static UCPlayerStatus character_step(const UCCollisionAsset *a,const UCSolidBlockGeometry *const *blocks,size_t count,const float planar[3],int gravity,UCPlayer *body) {
    if(!a||!body||!planar||count>4096||(count&&!blocks))return UC_PLAYER_ARGUMENT;
    for(unsigned i=0;i<3;i++)if(!isfinite(planar[i])||((i<2||!gravity)&&fabsf(planar[i])>50))return UC_PLAYER_ARGUMENT;
    for(unsigned i=0;i<3;i++)if(!isfinite(body->position[i])||!isfinite(body->velocity[i]))return UC_PLAYER_ARGUMENT;
    for(size_t i=0;i<count;i++){UCCollisionContact ignored;if(uc_collision_block_contact(blocks[i],body->position,body->position,0,&ignored)<0)return UC_PLAYER_ARGUMENT;}
    /*2BA430 Cinfo+4C=.05, copied by2E5D30 to proxy+58.2E5A30 subtracts
     * that keep-distance from contact separation. Include it in the PC query
     * envelope, retaining the authored .35 capsule and .45/1.70 endpoints.
     * Omitting it sinks the origin5cm and stalls original waypoint Y tests. */
    const float shape[3]={.45f,1.6999999284744263f,.35f+.05f};
    UCPlayer next=*body;next.crouched=0;memcpy(next.velocity,planar,12);
    return step_motion(a,blocks,count,next,shape,gravity&&next.grounded&&planar[0]==0&&planar[1]==0,gravity,body);
}
UCPlayerStatus uc_character_step_blocks(const UCCollisionAsset *a,const UCSolidBlockGeometry *const *blocks,size_t count,const float planar[2],UCPlayer *body) {
    if(!planar||!body)return UC_PLAYER_ARGUMENT;
    const float motion[3]={planar[0],planar[1],body->velocity[2]>0?0:body->velocity[2]};
    return character_step(a,blocks,count,motion,1,body);
}
UCPlayerStatus uc_character_step_authored_blocks(const UCCollisionAsset *a,const UCSolidBlockGeometry *const *blocks,size_t count,const float velocity[3],UCPlayer *body) {
    return character_step(a,blocks,count,velocity,0,body);
}
