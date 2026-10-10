#include "weapon_spread.h"
#include <fenv.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
static int32_t signed_word(uint32_t x) { int32_t y;memcpy(&y,&x,4);return y; }
static float random_range(int32_t word,float low,float high) {
    return (float)(((double)word*((double)high-low))*0x1p-31+low);
}
/* Existing2D3DC0 leaf is private in frozen look/placement modules. */
static void rotation(float x,float y,float z,float m[12]) {
    float sx=(float)sin(x),sy=(float)sin(y),sz=(float)sin(z);
    float cx=(float)cos(x),cy=(float)cos(y);
    double dz=cos(z);float cz=(float)dz;
    memset(m,0,12*sizeof(*m));
    m[0]=(float)(dz*cy);m[1]=(float)(-((double)cy*sz));m[2]=sy;
    m[4]=(cz*sy)*sx+cx*sz;m[5]=cz*cx-(sz*sy)*sx;
    m[6]=(float)(-((double)cy*sx));
    m[8]=sz*sx-(cz*cx)*sy;m[9]=(cx*sz)*sy+cz*sx;m[10]=cy*cx;
}
UCMainplayerStatus uc_weapon_spread(const UCWeaponSpreadInput *in,float direction[3]) {
    float bound,sample[3],inverse,radius,angles[3],matrix[12],result[3];
    if(!in||!direction||(in->item!=2&&in->item!=17)
       ||!isfinite(in->definition_spread)||!isfinite(in->accuracy))return UC_MAINPLAYER_ARGUMENT;
    for(unsigned i=0;i<3;++i)if(!isfinite(direction[i]))return UC_MAINPLAYER_ARGUMENT;
    if((_mm_getcsr()&0xE040u)||fegetround()!=FE_TONEAREST)return UC_MAINPLAYER_UNSUPPORTED;
    if(in->item==17) {
        int32_t numerator=signed_word((uint32_t)in->pellet_index*5u);
        int32_t divisor=signed_word((uint32_t)in->pellet_count*(in->alternate?2u:1u));
        if(!divisor||(numerator==INT32_MIN&&divisor==-1))return UC_MAINPLAYER_UNSUPPORTED;
        int32_t factor=signed_word((uint32_t)(numerator/divisor)+1u);
        bound=in->definition_spread*.2f;bound=(float)factor*bound;
    } else {
        bound=1.0f-in->accuracy;bound=bound*in->definition_spread;bound=bound*.5f;
    }
    if(!isfinite(bound))return UC_MAINPLAYER_UNSUPPORTED;
    for(unsigned i=0;i<3;++i)sample[i]=random_range(in->random[i],-1,1);
    double length=((double)sample[1]*sample[1]+(double)sample[0]*sample[0])
        +(double)sample[2]*sample[2];
    if(!(length>0))return UC_MAINPLAYER_UNSUPPORTED;
    inverse=(float)(1.0/sqrt(length));radius=random_range(in->random[3],0,bound);
    if(!isfinite(inverse)||!isfinite(radius))return UC_MAINPLAYER_UNSUPPORTED;
    for(unsigned i=0;i<3;++i) {
        angles[i]=inverse*sample[2-i];angles[i]=radius*angles[i];
        angles[i]=angles[i]*0.01745329238474369f;
        if(!isfinite(angles[i]))return UC_MAINPLAYER_UNSUPPORTED;
    }
    rotation(angles[0],angles[1],angles[2],matrix);
    for(unsigned i=0;i<3;++i) {
        result[i]=(matrix[i]*direction[0]+matrix[4+i]*direction[1])
            +(matrix[8+i]*direction[2]+0.0f);
        if(!isfinite(result[i]))return UC_MAINPLAYER_UNSUPPORTED;
    }
    memcpy(direction,result,sizeof(result));return UC_MAINPLAYER_OK;
}
