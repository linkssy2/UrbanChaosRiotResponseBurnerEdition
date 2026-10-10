#ifndef NV2A_CULL_H
#define NV2A_CULL_H
#include <stdint.h>
/* Host D3D8 winding modes for a top-left pixel-coordinate draw path.
 * 0 = unknown, 1 = none, 2 = clockwise, 3 = counterclockwise, 4 = both.
 * Both-face rejection applies only to polygons, not points/lines. */
static unsigned nv_cull_mode(uint32_t enabled,uint32_t face,uint32_t front)
{
    if(!enabled)return 1;
    if(face==0x408)return 4;
    if((face!=0x404&&face!=0x405)||(front!=0x900&&front!=0x901))return 0;
    int clockwise=(front==0x900);
    if(face==0x405)clockwise=!clockwise;
    return clockwise?2:3;
}
#endif
