#include "glass_shards.h"
#include <fenv.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <xmmintrin.h>
typedef struct Work {float impact[6],triangle[3][2];} Work;
static int push(Work **p,size_t *count,size_t *capacity,const Work *w){
    if(*count==*capacity){
        size_t n=*capacity+*capacity/2;if(n<=*count)n=*count+1;
        if(n>SIZE_MAX/sizeof **p)return 1;
        Work *q=(Work*)realloc(*p,n*sizeof *q);if(!q)return 1;*p=q;*capacity=n;
    }
    (*p)[(*count)++]=*w;return 0;
}
static double length(float x,float y){return sqrt((double)x*x+(double)y*y);}
static int triangle(const UCGlassShards *s,const Work *first){
    Work *stack=NULL;size_t count=0,capacity=0;int status=2;
    if(push(&stack,&count,&capacity,first))return 1;
    while(count){
        Work w=stack[--count];float (*t)[2]=w.triangle,r[4],center[2],offset[2];
        double sizes[3];for(unsigned j=0;j<3;j++)sizes[j]=length(t[j][0]-t[(j+1)%3][0],t[j][1]-t[(j+1)%3][1]);
        unsigned edge=sizes[0]>sizes[1]?(sizes[0]>sizes[2]?0:2):(sizes[1]>sizes[2]?1:2);
        for(unsigned j=0;j<2;j++){center[j]=((t[1][j]+t[0][j])+t[2][j])*(1.f/3.f);offset[j]=center[j]-w.impact[j];}
        if(uc_vector_random_signed(s->vectors,.25f,r))goto done;
        float threshold=(float)(length(offset[1],offset[0])*(.3f+r[0])+10.0);if(threshold>80)threshold=80;
        unsigned next=(edge+1)%3,opposite=(edge+2)%3;
        /* Selection uses rounded SSE differences; the final comparison uses
         * unrounded x87 subtraction, independently recomputed by the source. */
        double dx=(double)t[edge][0]-t[next][0],dy=(double)t[edge][1]-t[next][1];
        if(sqrt(dy*dy+dx*dx)>threshold){
            if(uc_vector_random_signed(s->vectors,.4f,r))goto done;
            float split=.5f+r[0],mid[2];for(unsigned j=0;j<2;j++)mid[j]=(t[next][j]-t[edge][j])*split+t[edge][j];
            Work a=w,b=w;memcpy(a.triangle[0],t[edge],8);memcpy(a.triangle[1],mid,8);memcpy(a.triangle[2],t[opposite],8);
            memcpy(b.triangle[0],mid,8);memcpy(b.triangle[1],t[next],8);memcpy(b.triangle[2],t[opposite],8);
            if(push(&stack,&count,&capacity,&a)||push(&stack,&count,&capacity,&b))goto done;
            continue;
        }
        int32_t word;if(uc_game_random_next(s->random,&word))goto done;
        if(!(.25f>(float)word*(1.f/2147483648.f)))continue;
        UCGlassShard shard;memcpy(shard.center,center,sizeof center);
        for(unsigned i=0;i<3;i++)for(unsigned j=0;j<2;j++)shard.triangle[i][j]=t[i][j]-center[j];
        if(uc_vector_random_signed(s->vectors,1,r))goto done;
        double inverse=1.0/sqrt(((double)r[2]*r[2]+(double)r[1]*r[1])+(double)r[0]*r[0]);
        for(unsigned j=0;j<3;j++)shard.axis[j]=(float)(r[j]*inverse);
        dx=(double)center[0]-w.impact[0];dy=(double)center[1]-w.impact[1];
        double distance=sqrt(dy*dy+dx*dx);float d=(float)distance;if(5.0>distance)d=5;
        float scale=(float)(1.0/pow(d,(double).7f));
        for(unsigned j=0;j<3;j++)shard.velocity[j]=(first->impact[j+2]*first->impact[5])*scale;
        if(uc_vector_random_signed(s->vectors,1,r))goto done;
        scale=(float)(1000.0/pow(d,.5));
        for(unsigned j=0;j<3;j++)shard.velocity[j]=shard.velocity[j]+r[j]*scale;
        shard.spin=100.f/d;if(s->submit(s->owner,&shard))goto done;
    }
    status=0;
done:free(stack);return status;
}
int uc_glass_shards_emit(const UCGlassShards *s,const float q[4][2],const float impact[6]){
    if(!s||!q||!impact||!s->vectors||!s->random||!s->preflight||!s->submit||
        (_mm_getcsr()&0xE040u)||fegetround()!=FE_TONEAREST||s->vectors->first>=28||s->vectors->second>=28||s->random->first>=250||s->random->second>=250)return 1;
    for(unsigned i=0;i<112;i++)if(!isfinite(s->vectors->values[i])||s->vectors->values[i]<0||s->vectors->values[i]>4000000)return 1;
    for(unsigned i=0;i<4;i++)for(unsigned j=0;j<2;j++)if(!isfinite(q[i][j])||fabsf(q[i][j])>4096)return 1;
    for(unsigned i=0;i<6;i++)if(!isfinite(impact[i])||fabsf(impact[i])>1e6f)return 1;
    if(s->preflight(s->owner,q,impact))return 1;
    Work w;memcpy(w.impact,impact,sizeof w.impact);memcpy(w.triangle,q,sizeof w.triangle);
    int result=triangle(s,&w);if(result)return result;
    memcpy(w.impact,impact,sizeof w.impact);memcpy(w.triangle,q+2,16);memcpy(w.triangle[2],q[0],8);
    return triangle(s,&w)?2:0;
}
