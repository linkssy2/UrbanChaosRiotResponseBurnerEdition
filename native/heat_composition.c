#include "heat_composition.h"
#include <math.h>
#include <xmmintrin.h>
static float bound(float x,float end){return x<=0?0:x>=end?end:x;}
int uc_heat_composition(uint32_t width,uint32_t height,const float view[4],
    const float grid[300][2],UCHeatComposition *out){
    if(!view||!grid||!out||width<2||height<2||width>4096||height>4096||(_mm_getcsr()&0xE040u))return 1;
    for(unsigned i=0;i<4;i++)if(!isfinite(view[i])||fabsf(view[i])>4096)return 1;
    for(unsigned i=0;i<300;i++)for(unsigned j=0;j<2;j++)if(!isfinite(grid[i][j])||fabsf(grid[i][j])>65536)return 1;
    UCHeatComposition p;const float w=(float)(width/2),h=(float)(height/2);
    const float xy[4][2]={{0,0},{w,0},{0,h},{w,h}};
    for(unsigned i=0;i<4;i++){
        p.scene[i].position[0]=p.mask[i].position[0]=xy[i][0];p.scene[i].position[1]=p.mask[i].position[1]=xy[i][1];
        p.scene[i].position[2]=p.mask[i].position[2]=1;
        p.scene[i].uv[0]=view[0]+((i&1)?view[2]:0);p.scene[i].uv[1]=view[1]+((i&2)?view[3]:0);
        p.mask[i].uv[0]=(i&1)?(float)width:0;p.mask[i].uv[1]=(i&2)?(float)height:0;
    }
    const float dx=view[2]/19.f,dy=view[3]/14.f;
    for(unsigned col=0;col<19;col++)for(unsigned row=0;row<15;row++)for(unsigned side=0;side<2;side++){
        UCHeatVertex*v=&p.strips[col][row*2+side];const float*g=grid[row*20+col+side];
        v->position[0]=(float)(col+side)*dx+view[0];v->position[1]=(float)row*dy+view[1];v->position[2]=1;
        v->uv[0]=5.f+bound(g[0]*w,w);v->uv[1]=5.f+bound(g[1]*h,h);
    }
    *out=p;return 0;
}
