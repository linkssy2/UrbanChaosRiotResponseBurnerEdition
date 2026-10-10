#include "normal_palette.h"
#include <math.h>

static uint32_t channel(float value) {
    int value8=(int)((value+1.0f)*127.5f);
    return (uint32_t)(value8<0?0:value8>255?255:value8);
}
void uc_normal_palette(uint32_t out[256]) {
    unsigned count=0,longitude=0,segments=1;
    float latitude=0,step=0.06544984877109528f;
    if(!out)return;
    while(count<255) {
        double angle=(double)longitude*6.2831854820251465/(double)segments;
        double ring=sin((double)latitude);
        float x=(float)(cos(angle)*ring),y=(float)cos((double)latitude),z=(float)(sin(angle)*ring);
        float inverse=(float)(1.0/sqrt(((double)z*z+(double)y*y)+(double)x*x));
        uint32_t color=0xFF000000u|(channel(x*inverse)<<16)|(channel(y*inverse)<<8)|channel(z*inverse);
        unsigned i=0;while(i<count&&out[i]!=color)++i;
        if(i==count)out[count++]=color;
        if(++longitude>=segments) {
            longitude=0;segments+=4;
            if(segments<8)segments=8;else if(segments>32)segments=32;
            latitude=step+latitude;step=step*1.100000023841858f;
        }
    }
    out[255]=0x0080FF80u;
}
