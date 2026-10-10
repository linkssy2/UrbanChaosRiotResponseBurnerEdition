#ifndef NV2A_REMOVAL_POLICY_H
#define NV2A_REMOVAL_POLICY_H
#include <stdint.h>
/* Diagnostic grouping only: never infer object identity from this hash. */
static uint32_t nv_removal_bucket(uint32_t offset,uint32_t format,uint32_t program,uint32_t mode){
 uint32_t h=2166136261u;
 h=(h^offset)*16777619u;h=(h^format)*16777619u;
 h=(h^program)*16777619u;return (h^mode)*16777619u;
}
static int nv_removal_eligible(int inline_array,int blend,int depth_write,int stencil,int query,uint32_t shader){
 return inline_array==1&&blend&&!depth_write&&!stencil&&!query&&shader==1;
}
static int nv_removal_selected(uint32_t key,uint32_t mask,uint32_t value){return (key&mask)==value;}
/* Geometry diagnostic deliberately changes depth coverage. Queries/stencil and
 * blended passes are excluded; this is NOT an occlusion-safe optimization. */
static int nv_removal_geometry_eligible(int indexed,int blend,int alpha,int stencil,int query,int failed,unsigned mode){
 (void)alpha; /* Alpha-tested world surfaces are geometry too. */
 return indexed&&!blend&&!stencil&&!query&&!failed&&(mode==5||mode==6);
}
#endif
