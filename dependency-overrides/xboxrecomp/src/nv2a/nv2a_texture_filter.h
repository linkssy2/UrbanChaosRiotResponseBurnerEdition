#ifndef NV2A_TEXTURE_FILTER_H
#define NV2A_TEXTURE_FILTER_H
/* Base-level spatial filtering only. Mipmap selection, LOD bias, convolution,
 * anisotropy and signed component conversion are separate contracts. */
static unsigned nv_texture_spatial_filter(unsigned packed, unsigned magnify)
{
    unsigned mode=magnify?((packed>>24)&15):((packed>>16)&255);
    if(magnify)return mode==1?1:mode==2?2:0;
    switch(mode){
    case 1:case 3:case 5:return 1;
    case 2:case 4:case 6:return 2;
    default:return 0;
    }
}
static float nv_texture_lod_bias(unsigned packed)
{
    unsigned raw=packed&8191;
    return (float)((int)raw-(raw&4096?8192:0))/256.0f;
}
#endif
