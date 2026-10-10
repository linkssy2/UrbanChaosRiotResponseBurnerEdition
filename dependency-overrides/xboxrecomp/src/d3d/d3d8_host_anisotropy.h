#ifndef D3D8_HOST_ANISOTROPY_H
#define D3D8_HOST_ANISOTROPY_H
/* Host quality setting: retain authored point/mixed filters, nonmipped lookups,
 * addresses, bias and LOD limits. 1 means no host filtering override. */
unsigned d3d8_GetHostAnisotropy(void);
/* Renderer-owner call: drains old queued draws before publishing the new level. */
int d3d8_SaveHostAnisotropy(unsigned);
static void d3d8_host_anisotropy(D3D11_SAMPLER_DESC *desc,unsigned level){
    if(level>1&&desc->MaxLOD>desc->MinLOD&&
       (desc->Filter==D3D11_FILTER_MIN_MAG_MIP_LINEAR||
        desc->Filter==D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT||
        desc->Filter==D3D11_FILTER_ANISOTROPIC)){
        desc->Filter=D3D11_FILTER_ANISOTROPIC;desc->MaxAnisotropy=level;
    }
}
#endif
