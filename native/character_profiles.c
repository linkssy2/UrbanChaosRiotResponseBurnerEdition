#include "character_profiles.h"
#include <math.h>
#include <string.h>
static int rows_valid(const UCCharacterProfileRow *r,uint32_t n){
    if(n>4||(n&&!r))return 0;for(uint32_t i=0;i<n;i++)if(r[i].joint>=15||!isfinite(r[i].yaw)||!isfinite(r[i].pitch)||!isfinite(r[i].roll)||!isfinite(r[i].speed))return 0;return 1;
}
int uc_character_profile_request(uint32_t channel,const UCCharacterProfileRow *rows,uint32_t count,uint8_t crouch,
    const float weights[5],UCCharacterProfileRequest *out,float used[5]){
    if(channel>=2||!weights||!out||!used||!rows_valid(rows,count)||!isfinite(weights[channel+1]))return 1;
    const float weight=weights[channel+1];memset(used,0,5*sizeof(float));used[channel+1]=weight;
    out->weight=weight;out->channel=channel;out->rows=rows;out->count=count;out->crouch=crouch;out->emitted=weight>0;return 0;
}
void uc_character_profiles_clear(UCCharacterSteeringChannel channels[2]){if(channels)memset(channels,0,2*sizeof(*channels));}
int uc_character_profile_accumulate(UCCharacterSteeringChannel channels[2],const UCCharacterProfileRequest *r){
    if(!channels||!r||r->channel>=2||!isfinite(r->weight)||!rows_valid(r->rows,r->count))return 1;
    if(!r->emitted)return 0;
    UCCharacterSteeringChannel *c=channels+r->channel;
    for(uint32_t i=0;i<r->count;i++){const UCCharacterProfileRow *row=r->rows+i;float *p=c->profile[row->joint];
        p[0]=r->weight*row->yaw+p[0];p[1]=r->weight*row->pitch+p[1];
        p[2]=r->weight*row->roll+p[2];p[3]=r->weight*row->speed+p[3];p[4]=p[4]+r->weight;
    }
    c->crouch=r->weight*(r->crouch?1.0f:0.0f)+c->crouch;return 0;
}
int uc_character_steering_accumulate(UCCharacterSteeringChannel channels[2],uint32_t channel,float weight,float yaw,float pitch){
    if(!channels||channel>=2||!isfinite(weight)||!isfinite(yaw)||!isfinite(pitch))return 1;
    UCCharacterSteeringChannel *c=channels+channel;c->weight+=weight;c->yaw=yaw*weight+c->yaw;c->pitch=pitch*weight+c->pitch;return 0;
}
