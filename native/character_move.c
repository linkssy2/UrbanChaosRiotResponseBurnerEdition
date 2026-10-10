#include "character_move.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "euler_rotation.h"
int uc_character_obstacle_directions(const float (*points)[3],uint32_t count,float (*out)[3],uint32_t capacity){
 if(!points||!out||!count||count>4096||capacity<(count==1?4:count*2))return 1;
 for(uint32_t i=0;i<count;i++)for(unsigned j=0;j<3;j++)if(!isfinite(points[i][j]))return 1;
 if(count==1){float s=(float)sqrt(2.);const float d[4][3]={{-s,0,s},{s,0,s},{s,0,-s},{-s,0,-s}};memcpy(out,d,sizeof d);return 0;}
 float (*result)[3]=malloc((size_t)count*2*sizeof *result);if(!result)return 1;
 float previous[3],normal[3];
 float dx=points[0][0]-points[count-1][0],dz=points[0][2]-points[count-1][2];
 double length=sqrt((double)dz*dz+(double)dx*dx);if(!isfinite(length)||length==0){free(result);return 1;}
 float inverse=(float)(1./length);previous[0]=inverse*dx;previous[1]=(float)((double)inverse*0.f);previous[2]=inverse*dz;
 normal[0]=0.f-previous[2];normal[1]=0;normal[2]=previous[0];
 for(uint32_t i=0;i<count;i++){
  uint32_t next=(i+1)%count;dx=points[next][0]-points[i][0];dz=points[next][2]-points[i][2];
  length=sqrt((double)dz*dz+(double)dx*dx);if(!isfinite(length)||length==0){free(result);return 1;}
  inverse=(float)(1./length);float direction[3]={inverse*dx,(float)((double)inverse*0.f),inverse*dz},n[3]={0.f-direction[2],0,direction[0]};
  double dot=((double)n[2]*normal[2]+(double)n[1]*normal[1])+(double)n[0]*normal[0];
  float scale=(float)sin(acos(dot)*.5);if(!isfinite(scale)){free(result);return 1;}
  for(unsigned j=0;j<3;j++){result[2*i][j]=scale*previous[j]+normal[j];result[2*i+1][j]=n[j]-scale*direction[j];}
  memcpy(previous,direction,sizeof previous);memcpy(normal,n,sizeof normal);
 }
 memcpy(out,result,(size_t)count*2*sizeof *result);free(result);return 0;
}
int uc_character_obstacle_expand(const float (*points)[3],uint32_t count,const float (*directions)[3],float radius,float (*out)[3],uint32_t capacity){
 uint32_t rows=count==1?4:count*2;if(!points||!directions||!out||!count||count>4096||capacity<rows||!isfinite(radius)||radius<0)return 1;
 for(uint32_t i=0;i<rows;i++)for(unsigned j=0;j<3;j++){
  float p=points[count==1?0:i/2][j],d=directions[i][j];if(!isfinite(p)||!isfinite(d)||!isfinite(d*radius+p))return 1;
 }
 for(uint32_t i=0;i<rows;i++)for(unsigned j=0;j<3;j++)out[i][j]=directions[i][j]*radius+points[count==1?0:i/2][j];return 0;
}
int uc_character_obstacle_cast(float height,const float position[3],const float from[3],const float to[3],
 const float center[3],float half_height,float bounding_radius,uint8_t allow_inside,
 const float (*expanded)[3],uint32_t count,float *fraction){
 if(!position||!from||!to||!center||!expanded||!fraction||!count||count>8192||
    !isfinite(height)||height<0||!isfinite(half_height)||half_height<0||
    !isfinite(bounding_radius)||bounding_radius<0||!isfinite(*fraction)||*fraction<0)return -1;
 for(unsigned j=0;j<3;j++)if(!isfinite(position[j])||!isfinite(from[j])||!isfinite(to[j])||!isfinite(center[j])||
    fabsf(position[j])>1e6f||fabsf(from[j])>1e6f||fabsf(to[j])>1e6f||fabsf(center[j])>1e6f)return -1;
 for(uint32_t i=0;i<count;i++)for(unsigned j=0;j<3;j++)if(!isfinite(expanded[i][j])||fabsf(expanded[i][j])>1e6f)return -1;
 if((center[1]-half_height>from[1]+height&&center[1]-half_height>to[1]+height)||
    (from[1]>half_height+center[1]&&to[1]>half_height+center[1]))return 0;
 float dx=position[0]-center[0],dz=position[2]-center[2];
 if(!allow_inside&&(double)bounding_radius+50.f>sqrt((double)(dx*dx+dz*dz)))return 0;
 dx=to[0]-from[0];dz=to[2]-from[2];float squared=dz*dz+dx*dx;
 double length=sqrt((double)squared);if(length<.001f)return 0;
 float distance=(float)length,x=(float)((double)dx/length),z=(float)((double)dz/length),nearest=*fraction;
 for(uint32_t i=0;i<count;i++){
  const float *a=expanded[i],*b=expanded[(i+1)%count];
  float ex=b[0]-a[0],ez=b[2]-a[2],edge_squared=ex*ex+ez*ez;if(edge_squared<.0001f)continue;
  float inverse=(float)(1./sqrt((double)edge_squared)),nx=inverse*ex,nz=inverse*ez;
  float denominator=nz*x-nx*z;if(denominator==0)continue;
  float numerator=(from[2]-a[2])*nx-(from[0]-a[0])*nz;
  float t=(numerator/denominator)/distance;if(t<0||t>nearest)continue;
  float along=t*distance,px=from[0]+along*x,pz=from[2]+along*z;
  double edge_t=((double)(pz-a[2])*nz+(double)(px-a[0])*nx)/sqrt((double)edge_squared);
  if(edge_t>=0&&(float)edge_t<1)nearest=t;
 }
 if(*fraction<=nearest)return 0;*fraction=nearest;return 1;
}
int uc_character_obstacles_cast(float height,const float position[3],const float from[3],const float to[3],
 const UCCharacterObstacle *obstacles,uint32_t count,UCCharacterObstacleHit *out){
 if(!position||!from||!to||(count&&!obstacles)||count>4096||!isfinite(height)||height<0)return -1;
 for(unsigned j=0;j<3;j++)if(!isfinite(position[j])||!isfinite(from[j])||!isfinite(to[j])||
    fabsf(position[j])>1e6f||fabsf(from[j])>1e6f||fabsf(to[j])>1e6f)return -1;
 float fraction=1;int32_t selected=-1;
 for(uint32_t i=0;i<count;i++){
  const UCCharacterObstacle *o=obstacles+i;
  int hit=uc_character_obstacle_cast(height,position,from,to,o->center,o->half_height,o->bounding_radius,o->allow_inside,o->expanded,o->count,&fraction);
  if(hit<0)return -1;if(hit){if(!out)return 1;selected=(int32_t)i;}
 }
 if(out){UCCharacterObstacleHit result;result.index=selected;
  for(unsigned j=0;j<3;j++)result.position[j]=from[j]+(to[j]-from[j])*fraction;*out=result;}
 return selected!=-1;
}
int uc_character_obstacle_detours(const float from[3],const float to[3],const float (*expanded)[3],uint32_t count,
 float (*forward)[3],float (*reverse)[3],uint32_t capacity,uint32_t sizes[2]){
 if(!from||!to||!expanded||!forward||!reverse||!sizes||!count||count>8192||capacity<count+2)return -1;
 for(unsigned j=0;j<3;j++)if(!isfinite(from[j])||!isfinite(to[j])||fabsf(from[j])>1e6f||fabsf(to[j])>1e6f)return -1;
 for(uint32_t i=0;i<count;i++)for(unsigned j=0;j<3;j++)if(!isfinite(expanded[i][j])||fabsf(expanded[i][j])>1e6f)return -1;
 float dx=to[0]-from[0],dz=to[2]-from[2],squared=dz*dz+dx*dx;
 double length=sqrt((double)squared);sizes[0]=sizes[1]=0;if(length<.001f)return 0;
 double inverse=1./length;float x=(float)(dx*inverse),z=(float)(dz*inverse),distance=(float)length,y=from[1]+50.f;
 float near_t=0,far_t=0,near_point[3]={0},far_point[3]={0};int32_t first=-1,last=-1;
 for(uint32_t i=0;i<count;i++){
  const float *a=expanded[i],*b=expanded[(i+1)%count];float ex=b[0]-a[0],ez=b[2]-a[2],square=ex*ex+ez*ez;
  float edge_length=(float)sqrt((double)square);if(square<.0001f)continue;
  float inv=(float)(1./sqrt((double)square)),nx=inv*ex,nz=inv*ez,denominator=nz*x-nx*z;if(denominator==0)continue;
  float t=(((from[2]-a[2])*nx-(from[0]-a[0])*nz)/denominator)/distance;
  float along=t*distance,px=along*x+from[0],pz=along*z+from[2];
  float edge_t=((pz-a[2])*nz+(px-a[0])*nx)/edge_length;if(edge_t<0||edge_t>=1)continue;
  float point[3]={a[0]+edge_t*ex,y,a[2]+edge_t*ez};
  if(first==-1||near_t>=t){first=(int32_t)i;near_t=t;memcpy(near_point,point,12);}
  if(last==-1||t>=far_t){last=(int32_t)i;far_t=t;memcpy(far_point,point,12);}
 }
 if(first==last)return 0;
 memcpy(forward[0],near_point,12);memcpy(reverse[0],near_point,12);uint32_t a=1,b=1;
 for(uint32_t i=((uint32_t)first+1)%count;;i=(i+1)%count){memcpy(forward[a],expanded[i],12);forward[a++][1]=y;if(i==(uint32_t)last)break;}
 for(uint32_t i=(uint32_t)first;;){memcpy(reverse[b],expanded[i],12);reverse[b++][1]=y;i=(i+count-1)%count;if(i==(uint32_t)last)break;}
 memcpy(forward[a++],far_point,12);memcpy(reverse[b++],far_point,12);sizes[0]=a;sizes[1]=b;return 1;
}
int uc_character_agent_steer(UCCharacterAgentSteering *s,float radius,float height,float horizon,
 const float *velocity,int32_t self,const float position[3],const float goal[3],
 const UCCharacterAgentEntry *agents,uint32_t count,UCCharacterCandidateClear clear,void *owner){
 if(!s||!position||!goal||!clear)return -1;
 for(unsigned j=0;j<3;j++)if(!isfinite(goal[j])||fabsf(goal[j])>1e6f)return -1;
 UCCharacterAgentHit hit;int found=uc_character_agents_cast(radius,height,horizon,velocity,self,0,agents,count,position,s->waypoint,&hit);
 if(found<0)return -1;if(!found){s->blocker=0;return 1;}
 const UCCharacterAgent *other=NULL;
 for(uint32_t i=0;i<count;i++)if(agents[i].entity==hit.entity){other=&agents[i].body;break;}
 if(!other)return -1;
 UCCharacterAgentSteering next=*s;
 if(!next.blocker||next.blocker!=hit.entity){next.blocker=hit.entity;next.changed=1;}
 double gx=(double)goal[0]-other->position[0],gz=(double)goal[2]-other->position[2];
 if(other->radius>sqrt(gz*gz+gx*gx)){memcpy(next.waypoint,hit.position,12);*s=next;return 1;}
 float dx=other->position[0]-position[0],dz=other->position[2]-position[2],squared=dz*dz+dx*dx;
 if(squared>=.0001f){
  double inverse=1./sqrt((double)squared);float ox=(float)(dx*inverse),oz=(float)(dz*inverse);
  dx=s->waypoint[0]-position[0];dz=s->waypoint[2]-position[2];squared=dz*dz+dx*dx;
  if(squared<.0001f){*s=next;return 1;}
  float inv=(float)(1./sqrt((double)squared)),direction[3]={inv*dx,0,inv*dz};
  float side=direction[0]*oz+(0.f-direction[2])*ox,combined=other->radius+radius;
  float offset[3]={((0.f-direction[2])*combined)*1.2000000476837158f,0,(direction[0]*combined)*1.2000000476837158f};
  for(unsigned attempt=0;attempt<4;attempt++){
   int subtract=(side>0)^(attempt&1);
   for(unsigned j=0;j<3;j++){
    next.waypoint[j]=subtract?hit.position[j]-offset[j]:hit.position[j]+offset[j];
    if(attempt>=2)next.waypoint[j]-=direction[j]*100.f;
   }
   int allowed=clear(owner,next.waypoint,next.blocker);if(allowed<0)return -1;
   if(allowed){*s=next;return 1;}
  }
  float d[3];for(unsigned j=0;j<3;j++)d[j]=position[j]-other->position[j];
  double distance=sqrt(((double)d[2]*d[2]+(double)d[1]*d[1])+(double)d[0]*d[0]);
  if(distance>((double)other->radius+radius)*1.2000000476837158f){memcpy(next.waypoint,s->waypoint,12);*s=next;return 1;}
 }
 memcpy(next.waypoint,position,12);next.status=3;*s=next;return 0;
}
int uc_character_agent_cast(float radius,float height,const float *velocity,const float from[3],const float to[3],const UCCharacterAgent *other,float *fraction){
 if(!from||!to||!other||!fraction||!isfinite(radius)||radius<0||!isfinite(height)||height<0||!isfinite(*fraction)||*fraction<=0||!isfinite(other->radius)||other->radius<0||!isfinite(other->height)||other->height<0)return -1;
 for(unsigned i=0;i<3;i++)if(!isfinite(from[i])||!isfinite(to[i])||!isfinite(other->position[i])||!isfinite(other->velocity[i])||(velocity&&!isfinite(velocity[i])))return -1;
 if((other->position[1]>from[1]+height&&other->position[1]>to[1]+height)||
    (from[1]>other->height+other->position[1]&&to[1]>other->height+other->position[1])||!velocity)return 0;
 float dx=to[0]-from[0],dz=to[2]-from[2],x=dx**fraction,z=dz**fraction,y=0.f**fraction;
 float length=(float)sqrt(((double)z*z+(double)x*x)+(double)y*y);if(!isfinite(length)||length<=0)return -1;
 double speed=sqrt(((double)velocity[0]*velocity[0]+(double)velocity[1]*velocity[1])+(double)velocity[2]*velocity[2]);
 float divisor=speed<.0001f?250.f:(float)speed,time=length/divisor,inverse=1.f/length;
 float ox=other->position[0]-from[0],oz=other->position[2]-from[2];
 float facing=((z*inverse)*oz+(inverse*x)*ox)+(inverse*y)*0.f;if(facing<0)return 0;
 x-=other->velocity[0]*time;z-=other->velocity[2]*time;y-=other->velocity[1]*time;
 double exact=sqrt(((double)z*z+(double)x*x)+(double)y*y);length=(float)exact;if(exact<.0001f)return 0;
 if(!isfinite(length)||length<=0)return -1;inverse=1.f/length;
 float distance=(float)sqrt((double)oz*oz+(double)ox*ox),combined=other->radius+radius;
 float overlap=combined>distance?combined-distance:0;
 float projection=((z*inverse)*oz+(inverse*x)*ox)+(inverse*y)*0.f;
 if(projection<=0||projection-combined>distance)return 0;
 float perpendicular=distance*distance-projection*projection;if(perpendicular<0)perpendicular=0;
 perpendicular=(float)sqrt((double)perpendicular);
 if(perpendicular>combined||(overlap>0&&overlap>=combined-perpendicular))return 0;
 float along=(float)((double)projection-sqrt((double)combined*combined-(double)perpendicular*perpendicular));
 if(overlap<=0&&(along<0||along>length))return 0;
 float u=inverse*along;
 float hx=(other->velocity[0]*time+(from[0]+u*x))-from[0];
 float hz=(other->velocity[2]*time+(from[2]+z*u))-from[2];
 double full=sqrt((double)dz*dz+(double)dx*dx);
 float result=(float)(sqrt((double)hz*hz+(double)hx*hx)/full);if(!isfinite(result))return -1;
 *fraction=result;return 1;
}
int uc_character_agents_cast(float radius,float height,float horizon,const float *velocity,int32_t self,int32_t ignore,const UCCharacterAgentEntry *agents,uint32_t count,const float from[3],const float to[3],UCCharacterAgentHit *out){
 if(!from||!to||!isfinite(radius)||radius<0||!isfinite(height)||height<0||!isfinite(horizon)||horizon<0||(count&&!agents)||count>65536)return -1;
 for(unsigned i=0;i<3;i++)if(!isfinite(from[i])||!isfinite(to[i])||(velocity&&!isfinite(velocity[i])))return -1;
 double x=(double)to[0]-from[0],z=(double)to[2]-from[2],exact=sqrt(z*z+x*x);float length=(float)exact;
 if(exact<.0001f||!velocity)return 0;
 double speed=sqrt(((double)velocity[0]*velocity[0]+(double)velocity[1]*velocity[1])+(double)velocity[2]*velocity[2]);
 float limited=speed<.1f?.1f:(float)speed,fraction=(horizon*limited)/length;if(fraction>=1)fraction=1;if(!isfinite(fraction))return -1;
 int32_t selected=0;
 for(uint32_t i=0;i<count;i++){
  const UCCharacterAgentEntry *a=agents+i;if(a->entity==self||(ignore&&a->entity==ignore)||fraction==0)continue;
  if(a->entity<=0)return -1;
  int hit=uc_character_agent_cast(radius,height,velocity,from,to,&a->body,&fraction);if(hit<0)return -1;
  if(hit){if(!out)return 1;selected=a->entity;}
 }
 if(out){UCCharacterAgentHit result;result.entity=selected;for(unsigned i=0;i<3;i++)result.position[i]=selected?from[i]+(to[i]-from[i])*fraction:to[i];*out=result;}
 return selected!=0;
}
int uc_character_search_enqueue(UCCharacterSearchRecord *records,uint32_t count,int32_t id,
    int32_t *open,uint32_t *size,uint32_t capacity){
    if(!records||!open||!size||*size>=capacity||capacity>1000000||id<0||(uint32_t)id>=count||!isfinite(records[id].total))return 1;
    for(uint32_t i=0;i<*size;i++)if(open[i]<0||(uint32_t)open[i]>=count||!isfinite(records[open[i]].total))return 1;
    uint32_t at=0;while(at<*size&&records[open[at]].total<=records[id].total)++at;
    memmove(open+at+1,open+at,(*size-at)*sizeof *open);open[at]=id;++*size;
    if(records[id].state==1)for(uint32_t i=at+1;i<*size;i++)if(open[i]==id){memmove(open+i,open+i+1,(*size-i-1)*sizeof *open);--*size;break;}
    records[id].state=1;return 0;
}
typedef struct NavigationEndpoint {int32_t node;float distance;} NavigationEndpoint;
static int endpoint_compare(const void *a,const void *b){
    const NavigationEndpoint *x=a,*y=b;
    return x->distance<y->distance?-1:x->distance>y->distance?1:x->node<y->node?-1:x->node>y->node;
}
int uc_character_navigation_endpoint(const UCLevelLogic *logic,const float position[3],
    uint32_t region,UCCharacterClearanceRay ray,void *owner,int32_t *node){
    if(!logic||!position||!node||!ray||(logic->node_count&&!logic->nodes)||(logic->trigger_count&&!logic->triggers)||logic->node_count>100000)return 1;
    for(unsigned j=0;j<3;j++)if(!isfinite(position[j])||fabsf(position[j])>1e6f)return 1;
    for(uint32_t i=0;i<logic->node_count;i++){
        const UCLevelNode *n=logic->nodes+i;if(n->key!=UINT32_MAX&&n->key>=logic->trigger_count)return 1;
        for(unsigned j=0;j<3;j++)if(!isfinite(n->position[j])||fabsf(n->position[j])>1e6f)return 1;
    }
    NavigationEndpoint *candidates=logic->node_count?malloc(logic->node_count*sizeof *candidates):NULL;
    if(logic->node_count&&!candidates)return 1;uint32_t count=0;
    for(uint32_t i=0;i<logic->node_count;i++){
        const UCLevelNode *n=logic->nodes+i;if(n->key!=UINT32_MAX&&!(logic->triggers[n->key].flags&16))continue;
        double x=(double)n->position[0]-position[0],y=(double)n->position[1]-position[1],z=(double)n->position[2]-position[2];
        float distance=(float)sqrt((z*z+y*y)+x*x);
        if(distance<=2000)candidates[count++]=(NavigationEndpoint){(int32_t)i,distance};
    }
    if(count>1)qsort(candidates,count,sizeof *candidates,endpoint_compare);
    float from[3]={position[0],position[1]+10.f,position[2]};int32_t selected=-1;
    for(uint32_t i=0;i<count;i++){
        uint8_t clear;int32_t blocking;
        if(uc_character_clearance(from,logic->nodes[candidates[i].node].position,40,region,0,ray,owner,&clear,&blocking)){free(candidates);return 1;}
        if(clear){selected=candidates[i].node;break;}
    }
    free(candidates);*node=selected;return 0;
}
int uc_character_search_expand(UCCharacterSearchRecord *records,uint32_t *count,uint32_t capacity,
    int32_t current,int32_t ordinal,uint8_t revisit,const UCCharacterSearchServices *s,uint8_t *more){
    if(!records||!count||!more||!s||!s->neighbor||!s->admit||!s->cost||!s->heuristic||!s->enqueue||
       *count>capacity||capacity>1000000||current<0||(uint32_t)current>=*count||ordinal<0)return 1;
    int32_t next=s->neighbor(s->owner,current,ordinal);
    if(*count>capacity)return 1;
    // Original admits index==count; native storage requires a constructed record.
    if(next<0||(uint32_t)next>*count){*more=0;return 0;}
    if((uint32_t)next==*count)return 1;
    UCCharacterSearchRecord *r=records+next;
    if((revisit||!r->state)&&s->admit(s->owner,next)){
        double edge=s->cost(s->owner,next);float cost=(float)(edge+records[current].cost);
        double h=s->heuristic(s->owner,next);float heuristic=(float)h,total=(float)(h+cost);
        if(!isfinite(edge)||!isfinite(cost)||!isfinite(h)||!isfinite(heuristic)||!isfinite(total))return 1;
        if(!r->state||total<=r->total){r->parent=current;r->cost=cost;r->heuristic=heuristic;r->total=total;s->enqueue(s->owner,next);}
    }
    *more=1;return 0;
}
int uc_character_navigation_penalty(const float point[3],const UCCharacterNavigationInfluence *influences,uint32_t count,double *out){
    if(!point||!out||(count&&!influences)||count>4096)return 1;
    for(unsigned j=0;j<3;j++)if(!isfinite(point[j]))return 1;
    float sum=0;
    for(uint32_t i=0;i<count;i++){
        const UCCharacterNavigationInfluence *a=influences+i;
        if(!isfinite(a->radius)||a->radius<0||!isfinite(a->weight))return 1;
        for(unsigned j=0;j<3;j++)if(!isfinite(a->position[j]))return 1;
        double x=(double)point[0]-a->position[0],y=(double)point[1]-a->position[1],z=(double)point[2]-a->position[2];
        double distance=sqrt((z*z+y*y)+x*x);
        if(distance<a->radius)sum+=(1.f-(float)distance/a->radius)*a->weight;
    }
    if(count)sum/=(float)count;if(!isfinite(sum))return 1;*out=(double)sum+1.;return 0;
}
int uc_character_navigation_cost(const float *from,const float to[3],const UCCharacterNavigationInfluence *influences,uint32_t count,double *out){
    if(!out)return 1;if(!from){*out=0;return 0;}
    double a,b;if(uc_character_navigation_penalty(from,influences,count,&a)||uc_character_navigation_penalty(to,influences,count,&b))return 1;
    float d[3];for(unsigned j=0;j<3;j++){d[j]=to[j]-from[j];if(!isfinite(d[j]))return 1;}
    double length=sqrt(((double)d[2]*d[2]+(double)d[1]*d[1])+(double)d[0]*d[0]);
    double result=length*((b+(float)a)*.5);if(!isfinite(result))return 1;*out=result;return 0;
}
int uc_character_world_blocked(const float from[3],const float to[3],float radius,
    uint32_t region,uint8_t extend_center,uint8_t block_scripted,
    UCCharacterClearanceRay ray,UCCharacterScriptedObstacle scripted,void *owner,uint8_t *blocked){
    if(!from||!to||!blocked||!ray||!scripted||!isfinite(radius)||radius<0)return 1;
    float d[3];for(unsigned i=0;i<3;i++){
        if(!isfinite(from[i])||!isfinite(to[i])||fabsf(from[i])>1e6f||fabsf(to[i])>1e6f)return 1;
        d[i]=to[i]-from[i];
    }
    float length2=(d[2]*d[2]+d[0]*d[0])+d[1]*d[1];
    if(length2<.0001f){*blocked=0;return 0;}
    uint8_t clear;int32_t entity=0;
    if(uc_character_clearance(from,to,radius,region,extend_center,ray,owner,&clear,&entity))return 1;
    *blocked=!clear&&(!entity||!scripted(owner,entity)||block_scripted);return 0;
}
int uc_character_candidate_clear(void *owner,const float point[3],int32_t ignored){
 const UCCharacterCandidateQuery *q=owner;
 if(!q||!point||!q->position||!q->destination||!q->ray||!q->scripted)return -1;
 uint8_t blocked;
 if(uc_character_world_blocked(q->position,point,q->radius,q->region,0,0,q->ray,q->scripted,q->owner,&blocked))return -1;
 if(blocked)return 0;
 int hit=uc_character_obstacles_cast(q->height,q->position,q->position,point,q->obstacles,q->obstacle_count,NULL);
 if(hit)return hit<0?-1:0;
 hit=uc_character_agents_cast(q->radius,q->height,q->horizon,q->velocity,q->self,ignored,q->agents,q->agent_count,q->position,point,NULL);
 if(hit)return hit<0?-1:0;
 if(uc_character_world_blocked(point,q->destination,q->radius,q->region,0,0,q->ray,q->scripted,q->owner,&blocked))return -1;
 if(blocked)return 0;
 hit=uc_character_obstacles_cast(q->height,q->position,point,q->destination,q->obstacles,q->obstacle_count,NULL);
 return hit<0?-1:!hit;
}
int uc_character_route_clear_options(const UCCharacterCandidateQuery*q,const float point[3],float margin,uint8_t direct,uint8_t block_scripted){
 if(!q||!point||!q->position||!isfinite(margin)||margin<0)return -1;
 for(unsigned j=0;j<3;j++)if(!isfinite(point[j])||!isfinite(q->position[j])||fabsf(point[j])>1e6f||fabsf(q->position[j])>1e6f)return -1;
 float x=q->position[0]-point[0],z=q->position[2]-point[2];
 if(direct&&fabs((double)q->position[1]-point[1])>=100&&margin*margin>=z*z+x*x)return 0;
 uint8_t blocked;
 if(uc_character_world_blocked(q->position,point,q->radius,q->region,0,block_scripted,q->ray,q->scripted,q->owner,&blocked))return -1;
 if(blocked)return 0;
 int hit=uc_character_obstacles_cast(q->height,q->position,q->position,point,q->obstacles,q->obstacle_count,NULL);
 if(hit)return hit<0?-1:0;
 hit=uc_character_agents_cast(q->radius,q->height,q->horizon,q->velocity,q->self,0,q->agents,q->agent_count,q->position,point,NULL);
 return hit<0?-1:!hit;
}
int uc_character_route_clear(const UCCharacterCandidateQuery*q,const float point[3],float margin,uint8_t direct){
 return uc_character_route_clear_options(q,point,margin,direct,1);
}
static int route_clearance(void *owner,const float*a,const float*b,float radius,uint32_t region,uint8_t *clear){
 const UCCharacterCandidateQuery*q=owner;int32_t entity;
 return uc_character_clearance(a,b,radius,region,0,q->ray,q->owner,clear,&entity);
}
int uc_character_route_adopt(UCCharacterRouteProgress*s,const float(*points)[3],const uint8_t*tags,uint32_t count,const UCCharacterCandidateQuery*q){
 if(!s||!points||!tags||!count||count>4096||!q||!q->position||!q->ray||!q->scripted)return 1;
 for(unsigned j=0;j<3;j++)if(!isfinite(q->position[j])||fabsf(q->position[j])>1e6f)return 1;
 for(uint32_t i=0;i<count;i++)for(unsigned j=0;j<3;j++)if(!isfinite(points[i][j])||fabsf(points[i][j])>1e6f)return 1;
 s->status=2;s->cursor=0;
 while(!tags[s->cursor]&&s->cursor+1<count){int clear=uc_character_route_clear(q,points[s->cursor+1],0,0);if(clear<0)return 2;if(!clear)break;s->cursor++;}
 memcpy(s->anchor,q->position,12);s->elapsed=0;return 0;
}
int uc_character_route_update(UCCharacterRouteProgress*s,const float(*points)[3],const uint8_t*tags,
 uint32_t count,float margin,float delta,const UCCharacterCandidateQuery*q,uint8_t*changed,float waypoint[3]){
 if(!s||!points||!tags||!count||count>4096||s->cursor>=count||s->status!=2||!q||!q->position||!q->ray||!q->scripted||!changed||!waypoint||
    !isfinite(margin)||margin<0||!isfinite(delta)||delta<0||!isfinite(s->elapsed)||!isfinite(s->elapsed+delta))return 1;
 for(unsigned j=0;j<3;j++)if(!isfinite(s->anchor[j])||!isfinite(q->position[j])||fabsf(q->position[j])>1e6f)return 1;
 for(uint32_t i=s->cursor;i<count;i++)for(unsigned j=0;j<3;j++)if(!isfinite(points[i][j])||fabsf(points[i][j])>1e6f)return 1;
 float x=q->position[0]-s->anchor[0],z=q->position[2]-s->anchor[2];
 if(sqrt((double)(x*x+z*z))>101){memcpy(s->anchor,q->position,12);s->elapsed=0;}else s->elapsed+=delta;
 if(s->elapsed>2){int clear=uc_character_route_clear(q,points[s->cursor],margin,1);if(clear<0)return 2;
  if(!clear){s->status=1;return 0;}s->elapsed=0;
 }
 if(!tags[s->cursor]&&s->cursor+1<count){int clear=uc_character_route_clear(q,points[s->cursor+1],margin,0);if(clear<0)return 2;
  if(clear){s->cursor++;*changed=1;}
 }
 uint8_t active;
 if(uc_character_path_advance(points,count,&s->cursor,q->position,margin,100,50,q->region,route_clearance,(void*)q,changed,&active,waypoint))return 2;
 if(!active)s->status=4;return 0;
}
int uc_character_navigation_search(float *elapsed,float delta,uint8_t continuing,
    const UCCharacterNavigationServices *s){
    if(!elapsed||!s||!s->search||!isfinite(*elapsed)||!isfinite(delta)||delta<0||!isfinite(*elapsed+delta))return 1;
    *elapsed+=delta;if(!s->search(s->owner,continuing!=0))*elapsed=0;return 0;
}
int uc_character_navigation_blocked(float *elapsed,float delta,float radius,const float *velocity,uint8_t *restart){
    if(!elapsed||!restart||!isfinite(*elapsed)||!isfinite(delta)||delta<0||!isfinite(*elapsed+delta)||!isfinite(radius)||radius<0)return 1;
    double speed=0;if(velocity){for(unsigned i=0;i<3;i++)if(!isfinite(velocity[i]))return 1;
        speed=sqrt(((double)velocity[0]*velocity[0]+(double)velocity[1]*velocity[1])+(double)velocity[2]*velocity[2]);}
    *elapsed+=delta;*restart=(velocity&&speed>radius)||*elapsed>2.5f;return 0;
}
int uc_character_navigation_arrived(float *elapsed,float delta,float radius,const float position[3],const float goal[3],uint8_t *restart){
    if(!elapsed||!restart||!position||!goal||!isfinite(*elapsed)||!isfinite(delta)||delta<0||!isfinite(*elapsed+delta)||!isfinite(radius)||radius<0)return 1;
    for(unsigned i=0;i<3;i++)if(!isfinite(position[i])||!isfinite(goal[i]))return 1;
    double x=(double)position[0]-goal[0],z=(double)position[2]-goal[2];
    *elapsed+=delta;*restart=sqrt(x*x+z*z)>radius;return 0;
}
int uc_character_navigation_active(float *elapsed,float delta,const float position[3],
    float waypoint[3],const UCCharacterNavigationServices *s){
    if(!elapsed||!position||!waypoint||!s||!s->route||!s->obstacles||!s->agents||!s->scripted_obstacle||
       !isfinite(*elapsed)||!isfinite(delta)||delta<0||!isfinite(*elapsed+delta))return 1;
    for(unsigned i=0;i<3;i++)if(!isfinite(position[i])||!isfinite(waypoint[i]))return 1;
    uint8_t changed=*elapsed==0;*elapsed+=delta;
    if(!s->route(s->owner,waypoint,&changed)||!s->obstacles(s->owner,waypoint,&changed)||!s->agents(s->owner,waypoint,&changed))*elapsed=0;
    else s->scripted_obstacle(s->owner,position,waypoint);
    return 0;
}
static double graph_curve(float x){
    if(x>=1)return 1;if(x<=0)return 0;
    return (1.-cos((double)x*3.1415927410125732))*.5;
}
int uc_character_path_advance(const float (*points)[3],uint32_t count,uint32_t *next,
    const float position[3],float horizontal,float vertical,float radius,uint32_t region,
    UCCharacterPathClearance clear,void *owner,uint8_t *changed,uint8_t *active,float waypoint[3]){
    if((count&&!points)||count>4096||!next||*next>count||!position||!changed||!active||!waypoint||!clear||
       !isfinite(horizontal)||horizontal<0||!isfinite(vertical)||vertical<0||!isfinite(radius)||radius<0)return 1;
    for(unsigned j=0;j<3;j++)if(!isfinite(position[j])||fabsf(position[j])>1e6f)return 1;
    for(uint32_t i=*next;i<count;i++)for(unsigned j=0;j<3;j++)if(!isfinite(points[i][j])||fabsf(points[i][j])>1e6f)return 1;
    uint32_t cursor=*next;uint8_t moved=*changed;
    if(cursor==count)moved=1;
    while(cursor<count){
        float x=points[cursor][0]-position[0],y=points[cursor][1]-position[1],z=points[cursor][2]-position[2];
        float distance=(float)sqrt((double)x*x+(double)z*z);
        if(distance>horizontal||fabsf(y)>vertical)break;
        if(distance>40&&cursor+1<count){uint8_t pass=0;
            if(clear(owner,position,points[cursor+1],radius,region,&pass))return 2;
            if(!pass)break;
        }
        cursor++;moved=1;
    }
    *next=cursor;*changed=moved;*active=cursor<count;
    if(cursor<count)memcpy(waypoint,points[cursor],12);
    return 0;
}
static int graph_time(float elapsed,float duration,float delta,const float*w,float*out){
    if(!w||!out||!isfinite(elapsed)||elapsed<0||!isfinite(duration)||duration<0||
        !isfinite(delta)||delta<=0||!isfinite(elapsed+delta))return 0;
    for(unsigned i=0;i<5;i++)if(!isfinite(w[i]))return 0;
    return 1;
}
int uc_character_graph_fade(UCCharacterGraphFade*s,const UCCharacterScheduleServices*h,
    float delta,const float weights[5],float output[5]){
    if(!s||!h||!h->destroy||!h->sample||!graph_time(s->elapsed,s->duration,delta,weights,output))return 1;
    if(s->current){
        s->elapsed+=delta;
        if(s->elapsed>s->duration){h->destroy(h->owner,s->current);s->current=NULL;}
        else {
            float phase=(float)((double)s->elapsed/s->duration),scaled[5];double scale=1.-graph_curve(phase);
            for(unsigned i=0;i<5;i++)scaled[i]=(float)((double)weights[i]*scale);
            h->sample(h->owner,s->current,delta,scaled,output);return 0;
        }
    }
    memset(output,0,5*sizeof(float));return 0;
}
int uc_character_graph_transition(UCCharacterGraphTransition*s,const UCCharacterScheduleServices*h,
    float delta,const float weights[5],float output[5]){
    if(!s||!s->current||!h||!h->destroy||!h->notify||!h->sample||
        !graph_time(s->elapsed,s->duration,delta,weights,output))return 1;
    s->elapsed+=delta;float phase=s->elapsed/s->duration,used[5]={0},part[5],scaled[5];
    if(phase>1){
        if(s->previous)h->destroy(h->owner,s->previous);s->previous=NULL;
        if(s->callback){h->notify(h->owner,s->callback);s->callback=NULL;}
        h->sample(h->owner,s->current,delta,weights,part);
        for(unsigned i=0;i<5;i++)used[i]+=part[i];
    }else {
        float scale=(float)graph_curve(phase);
        for(unsigned i=0;i<5;i++)scaled[i]=(float)((double)weights[i]*scale);
        h->sample(h->owner,s->current,delta,scaled,part);
        for(unsigned i=0;i<5;i++)used[i]+=part[i];
        if(s->previous){
            scale=1.f-scale;for(unsigned i=0;i<5;i++)scaled[i]=weights[i]*scale;
            h->sample(h->owner,s->previous,delta,scaled,part);
            for(unsigned i=0;i<5;i++)used[i]+=part[i];
        }
    }
    memcpy(output,used,sizeof used);return 0;
}
int uc_character_schedule_queue(UCCharacterSchedule*s,const UCCharacterScheduleServices*h,
    void *graph,void *callback,uint8_t interrupt){
    if(!s||!h||!graph||!h->interruptor||!h->wrap||!h->destroy||!h->cancel)return 1;
    void *owner=h->owner;
    if(!h->interruptor(owner,graph))graph=h->wrap(owner,graph);
    if(s->pending)h->destroy(owner,s->pending);
    if(s->pending_callback)h->cancel(owner,s->pending_callback);
    s->pending=graph;s->pending_callback=callback;s->interrupt=interrupt;return 0;
}
int uc_character_schedule_stop(UCCharacterSchedule*s,const UCCharacterScheduleServices*h,void *callback){
    if(!s||!h||!h->interruptor||!h->interrupt||!h->destroy||!h->cancel||!h->notify)return 1;
    void *owner=h->owner;
    if(s->pending)h->destroy(owner,s->pending);
    s->pending=NULL;
    if(s->pending_callback){h->cancel(owner,s->pending_callback);s->pending_callback=NULL;}
    if(s->current){
        void *control=h->interruptor(owner,s->current);
        if(control){
            h->interrupt(owner,control,1);
            if(s->stop_callback)h->cancel(owner,s->stop_callback);
            s->stop_callback=callback;return 0;
        }
    }
    if(callback)h->notify(owner,callback);return 0;
}
int uc_character_schedule_update(UCCharacterSchedule*s,const UCCharacterScheduleServices*h,
    float delta,const float weights[5],float output[5]){
    if(!s||!h||!weights||!output||!h->finished||!h->interruptor||!h->interrupt||
        !h->destroy||!h->transition||!h->fade||!h->combine||!h->notify||!h->sample||
        !isfinite(delta)||delta<0)return 1;
    for(unsigned i=0;i<5;i++)if(!isfinite(weights[i]))return 1;
    void *owner=h->owner;
    if(s->fading&&h->finished(owner,s->fading)){
        if(s->fading)h->destroy(owner,s->fading);
        s->fading=NULL;
    }
    if(s->pending){
        if(s->current){void *control=h->interruptor(owner,s->current);if(control)h->interrupt(owner,control,s->interrupt);}
        if(!s->current||h->finished(owner,s->current)){
            s->current=h->transition(owner,s->pending,s->current,s->pending_callback);
            s->pending=s->pending_callback=NULL;
            if(s->stop_callback){h->notify(owner,s->stop_callback);s->stop_callback=NULL;}
        }
    }else if(s->current&&h->finished(owner,s->current)){
        void *old=s->fading;
        void *tail=h->fade(owner,s->current);
        s->fading=old?h->combine(owner,tail,old):tail;s->current=NULL;
        if(s->stop_callback){h->notify(owner,s->stop_callback);s->stop_callback=NULL;}
    }
    float used[5]={0},remaining[5],next[5];
    if(s->current){
        for(unsigned i=0;i<5;i++)remaining[i]=weights[i]-used[i];
        h->sample(owner,s->current,delta,remaining,next);
        for(unsigned i=0;i<5;i++)used[i]+=next[i];
    }
    if(s->fading){
        for(unsigned i=0;i<5;i++)remaining[i]=weights[i]-used[i];
        h->sample(owner,s->fading,delta,remaining,next);
        for(unsigned i=0;i<5;i++)used[i]+=next[i];
    }
    memcpy(output,used,sizeof used);return 0;
}
int uc_character_clip_rotation(const UCCharacterClipRequest *requests,const uint8_t *root,
    uint32_t count,float *scale){
    if(!scale||count>32||(count&&(!requests||!root)))return 1;
    double total=0;
    for(uint32_t i=count;i--;){
        const UCCharacterClipRequest *r=requests+i;
        if(!isfinite(r->weight)||r->weight<0||r->weight>1||!isfinite(r->root_scale))return 1;
        float contribution=root[i]?r->root_scale*r->weight:0;
        if(!isfinite(contribution))return 1;
        total+=contribution;
    }
    float result=(float)total;if(!isfinite(result))return 1;
    *scale=result;return 0;
}
static int valid(const UCCharacterMove*s,const UCCharacterMoveServices*h){
    return s&&h&&s->world&&s->count<=4096&&s->index>=0&&
        (s->count<=0||s->nodes)&&h->reset&&h->resolve&&h->target&&
        h->run&&h->transition&&h->actor_world;
}
int uc_character_move_enter(UCCharacterMove*s,UCCharacterLoopController*c,const UCCharacterMoveServices*h){
    if(!valid(s,h)||!c||!c->actor)return 1;
    s->controller=c;s->actor=c->actor;s->active=1;s->index=0;s->phase=0;
    h->reset(h->owner,c);return 0;
}
int uc_character_move_update(UCCharacterMove*s,const UCCharacterMoveServices*h){
    if(!valid(s,h)||!s->controller)return 1;
    if(s->phase==2){h->transition(h->owner,s,s->next);s->next=-1;return 0;}
    if(s->phase==0){
        if(s->index>=s->count){s->phase=2;goto publish;}
        int32_t node=s->nodes[s->index++];
        const float *point=h->resolve(h->owner,node);
        if(!point)goto publish;
        float target[3];memcpy(target,point,sizeof target);
        h->target(h->owner,s->controller,target);s->phase=1;
    }
    if(s->phase==1&&h->run(h->owner,s->controller)==4)s->phase=0;
publish:
    if(s->actor)memcpy(s->world,h->actor_world(h->owner,s->actor),16*sizeof(float));
    return 0;
}
int uc_character_walk_update(UCCharacterMove*s,const UCCharacterMoveServices*h,const UCCharacterWalkServices*w){
    if(!valid(s,h)||!s->controller||!w||!w->status||!w->waypoint||
       !w->near_target||!w->request||!w->stop||!w->clear_heading)return 1;
    if(s->phase==2){h->transition(h->owner,s,s->next);s->next=-1;return 0;}
    if(s->phase==0){
        if(s->index>=s->count){s->phase=2;goto publish;}
        const float *point=h->resolve(h->owner,s->nodes[s->index++]);
        if(!point)goto publish;
        float target[3];memcpy(target,point,sizeof target);
        h->target(h->owner,s->controller,target);s->phase=1;
    }
    if(s->phase==1){
        int32_t status=w->status(w->owner);
        if(status>=1&&status<=4){
            if(!s->actor)return 1;
            if(status<=2){
                float target[3];w->waypoint(w->owner,target);
                int32_t near_target=w->near_target(w->owner);
                w->request(w->owner,target,1,near_target);
            }else{
                if(status==4)s->phase=0;
                w->stop(w->owner);
            }
            w->clear_heading(w->owner);
        }
    }
publish:
    if(s->actor)memcpy(s->world,h->actor_world(h->owner,s->actor),16*sizeof(float));
    return 0;
}
static double distance(const float*a,const float*b){
    float x=a[0]-b[0],y=a[1]-b[1],z=a[2]-b[2];
    float square=(x*x+y*y)+z*z;return sqrt((double)square);
}
int uc_character_run(const UCCharacterRunServices*h,const uint8_t flags[5],int32_t*out){
    if(!h||!flags||!out||!h->goal||!h->actor_position||!h->player_id||
       !h->position||!h->stop||!h->status||!h->emote_if_blocked||
       !h->blocker||!h->emote||!h->waypoint||!h->request||!h->clear_heading)return 1;
    void*p=h->owner;
    if(flags[4]){
        float goal[3];h->goal(p,goal);double d=distance(h->actor_position(p),goal);
        if(d<350.0&&(float)d>100.0f&&h->player_id(p)!=-1){
            const float*player=h->position(p,h->player_id(p));
            if(player&&distance(player,goal)<150.0){h->stop(p);*out=3;return 0;}
        }
    }
    int32_t status=h->status(p);
    if(status>0&&status<=3){
        if(status==3&&flags[3]&&(uint8_t)h->emote_if_blocked(p)){
            int32_t player=h->player_id(p),blocker=h->blocker(p);
            h->emote(p,blocker==player?19:20,50);
        }
        float target[3];h->waypoint(p,target);h->request(p,target,flags[2]?1:0);
        if(flags[1])h->clear_heading(p);
    }
    *out=status;return 0;
}
/*19C40 retains the source's float pi constant in double fmod arithmetic. */
static float angle(float value){
    const double pi=3.1415927410125732421875,tau=6.283185482025146484375;
    return value>0?(float)(fmod((double)value+pi,tau)-pi):
        (float)-(fmod(pi-(double)value,tau)-pi);
}
int uc_character_locomotion_step(uint8_t *finished,uint8_t first_pending,
    const float position[3],float heading,const float target[3],uint8_t backwards,
    uint8_t override_heading,float relative_heading,int32_t original_mode,int32_t mode,
    float weight,UCCharacterLocomotionStep *out){
    if(!finished||!position||!target||!out||!isfinite(heading)||fabsf(heading)>1e6f||
       !isfinite(relative_heading)||fabsf(relative_heading)>1e6f||!isfinite(weight)||weight<0||weight>1)return 1;
    for(unsigned i=0;i<3;i++)if(!isfinite(position[i])||!isfinite(target[i])||
        fabsf(position[i])>1e6f||fabsf(target[i])>1e6f)return 1;
    UCCharacterLocomotionStep result={0};uint8_t done=*finished;
    if(!done){
        double dx=(double)target[0]-position[0],dz=(double)target[2]-position[2];
        double d=sqrt(dx*dx+dz*dz);
        float x=target[0]-position[0],z=target[2]-position[2];
        float reciprocal=(x||z)?(float)(1./sqrt((double)x*x+(double)z*z)):0;
        float bearing=angle((float)-atan2((double)(x*reciprocal),(double)(z*reciprocal)));
        if(!first_pending){
            if(d<40)done=1;
            else{
                float goal=angle(bearing-(backwards?angle(3.1415927410125732f):angle(0)));
                float desired=override_heading?angle(angle(relative_heading)+heading):bearing;
                if(fabsf(angle(desired-goal))>1.7453293800354004f||
                   fabsf(angle(goal-heading))>1.2217305898666382f)done=1;
                result.goal_heading=goal;result.heading_emitted=1;
            }
            if(original_mode!=mode)done=1;
        }
        if(!override_heading&&d>40&&weight>0){
            result.steering=angle(bearing-heading);result.steering_weight=weight;result.steering_emitted=1;
        }
    }
    *finished=done;*out=result;return 0;
}
int uc_character_heading(UCCharacterHeading*s,float delta){
    if(!s||!isfinite(delta)||delta<=0||delta>1||!isfinite(s->gain)||s->gain<0||
        !isfinite(s->max_acceleration)||s->max_acceleration<0||!isfinite(s->rate)||
        !isfinite(s->value)||fabsf(s->value)>1e6f||!isfinite(s->target)||fabsf(s->target)>1e6f)return 1;
    float denominator=s->gain*delta+1.f;if(!isfinite(denominator))return 1;
    float desired=((1.f-1.f/denominator)*angle(s->target-s->value))/delta;
    float change=desired-s->rate;
    if(fabsf(desired)>fabsf(s->rate)){
        if(!(s->max_acceleration>change))change=s->max_acceleration;
        if(!(change> -s->max_acceleration))change= -s->max_acceleration;
    }
    UCCharacterHeading next=*s;next.rate=change+s->rate;
    if(!isfinite(next.rate)||!isfinite(next.rate*delta))return 1;
    next.value=angle(angle(next.rate*delta)+s->value);
    if(fabsf(angle(next.value-s->target))<.0010000000474974513f){next.value=s->target;next.rate=0;}
    *s=next;return 0;
}
int uc_character_near(UCCharacterNear*s,const float position[3],float heading,
    const float inverse[16],float delta,float weight,UCCharacterNearOutput*out){
    if(!s||!position||!inverse||!out||!isfinite(delta)||delta<=0||delta>1||!isfinite(weight)||weight<0||weight>1||
       !isfinite(s->phase)||s->phase<0||s->phase>1e6f||!isfinite(s->rate)||s->rate<0||s->rate>1e6f||
       !isfinite(heading)||fabsf(heading)>1e6f||!isfinite(s->initial_turn))return 1;
    for(unsigned i=0;i<3;i++)if(!isfinite(position[i])||fabsf(position[i])>1e6f||!isfinite(s->target[i])||
        fabsf(s->target[i])>1e6f||!isfinite(s->direction[i])||!isfinite(s->initial_offset[i]))return 1;
    for(unsigned i=0;i<16;i++)if(!isfinite(inverse[i]))return 1;
    UCCharacterNear n=*s;UCCharacterNearOutput o={0};o.weight=weight;
    if(weight>=.800000011920929f){
        float target_heading=angle((float)-atan2((double)n.direction[0],(double)n.direction[2]));
        if(!n.initialized){
            n.initial_offset[0]=n.target[0]-position[0];n.initial_offset[1]=0;n.initial_offset[2]=n.target[2]-position[2];
            n.initial_turn=angle(target_heading-heading);n.swap_feet=n.initial_turn<.1745329350233078f;n.initialized=1;
        }
        float old=n.phase;n.phase=n.rate*delta+old;
        if(old>=1){o.completed=n.callback_pending!=0;n.callback_pending=0;}
        else{
            float phase=n.phase<1?n.phase:1,old_curve=(float)graph_curve(old),curve=(float)graph_curve(phase);
            if(fabs((double)old_curve-curve)>.0010000000474974513){
                float fraction=(curve-old_curve)/(1.f-old_curve);o.helper_emitted=1;
                o.helper[0]=(n.target[0]-position[0])*fraction;o.helper[1]=0*fraction;o.helper[2]=(n.target[2]-position[2])*fraction;
                o.helper[3]=angle(target_heading-heading)*fraction;
            }
            float local[3];for(unsigned i=0;i<3;i++)local[i]=(inverse[i]*n.initial_offset[0]+inverse[4+i]*n.initial_offset[1])+inverse[8+i]*n.initial_offset[2];
            float distance=(float)sqrt(((double)n.initial_offset[0]*n.initial_offset[0]+(double)n.initial_offset[1]*n.initial_offset[1])+(double)n.initial_offset[2]*n.initial_offset[2]);
            float steps[2];for(unsigned i=0;i<2;i++){float start=(i!=(unsigned)(n.swap_feet!=0))?.5f:0.f;float x=(curve-start)/.5f;steps[i]=x>=1?1:x<=0?0:x;}
            for(unsigned i=0;i<2;i++){
                float relative=steps[i]-curve;for(unsigned j=0;j<3;j++)o.feet[i][j]=local[j]*relative;
                o.feet[i][1]+=(float)(sin((double)steps[i]*3.1415927410125732)*distance*.15000000596046448);
                o.foot_yaw[i]=n.initial_turn*relative;
            }
            float amplitude=(n.small_bob?.10000000149011612f:.18000000715255737f)*distance;
            o.bob[1]=(float)((cos((double)phase*6.2831854820251465)-1.)*amplitude*.5);
        }
    }
    for(unsigned i=0;i<4;i++)if(!isfinite(o.helper[i]))return 1;
    for(unsigned i=0;i<3;i++)if(!isfinite(o.feet[0][i])||!isfinite(o.feet[1][i])||!isfinite(o.bob[i]))return 1;
    if(!isfinite(o.foot_yaw[0])||!isfinite(o.foot_yaw[1]))return 1;
    *s=n;*out=o;return 0;
}
int uc_character_locomotion_plan(const float*p,float heading,const float*t,
    uint8_t override_heading,float relative_heading,int32_t mode,uint8_t near_target,
    UCCharacterLocomotionPlan*out){
    if(!p||!t||!out||!isfinite(heading)||fabsf(heading)>1e6f||
       !isfinite(relative_heading)||fabsf(relative_heading)>1e6f)return 1;
    for(unsigned i=0;i<3;i++)if(!isfinite(p[i])||!isfinite(t[i])||fabsf(p[i])>1e6f||fabsf(t[i])>1e6f)return 1;
    float x=t[0]-p[0],z=t[2]-p[2];if(x==0&&z==0)return 1;
    float reciprocal=(float)(1.0/sqrt((double)x*x+(double)z*z));
    float bearing=angle((float)-atan2((double)(x*reciprocal),(double)(z*reciprocal)));
    float desired=override_heading?angle(angle(relative_heading)+heading):bearing;
    UCCharacterLocomotionPlan result={0,12,0,1};
    result.backwards=fabsf(angle(desired-bearing))>1.6580629348754883f;
    float goal=angle(bearing-(result.backwards?angle(3.1415927410125732f):angle(0)));
    result.base_action=result.backwards?2:mode==1?1:0;
    double dx=(double)t[0]-p[0],dz=(double)t[2]-p[2];
    float distance_xz=(float)sqrt(dx*dx+dz*dz);
    int push_off=!result.backwards&&!mode&&(!near_target||distance_xz>=200.f);
    float turn=angle(goal-heading);
    if(override_heading){
        float from=angle(desired-heading),to=angle(desired-goal);
        if((to>0)!=(from>0)&&(turn>0)!=(from>0))turn+=turn>0?-6.2831854820251465f:6.2831854820251465f;
    }
    if(turn>2.356194496154785f){result.turn_action=push_off?9:4;result.turn_scale=turn*.31830987334251404f;}
    else if(turn>1.0995573997497559f){result.turn_action=push_off?8:3;result.turn_scale=turn*.6366197466850281f;}
    else if(turn< -2.356194496154785f){result.turn_action=push_off?11:6;result.turn_scale=turn*-.31830987334251404f;}
    else if(turn< -1.0995573997497559f){result.turn_action=push_off?10:5;result.turn_scale=turn*-.6366197466850281f;}
    else if(push_off&&!mode)result.turn_action=7;
    *out=result;return 0;
}
int uc_character_motion(const float*m,const float*p,float heading,float scale,
    const float*helper,const float*displacement,float*out_position,float*out_heading){
    if(!m||!p||!helper||!displacement||!out_position||!out_heading||
       !isfinite(heading)||!isfinite(scale))return 1;
    for(unsigned i=0;i<16;i++)if(!isfinite(m[i]))return 1;
    for(unsigned i=0;i<4;i++)if(!isfinite(helper[i]))return 1;
    for(unsigned i=0;i<3;i++)if(!isfinite(p[i])||!isfinite(displacement[i]))return 1;
    float rotation=(float)(-atan2((double)m[8],(double)m[10])*(double)scale);
    float next=(helper[3]+rotation)+heading;
    float middle=angle((next-heading)*.5f+heading),world[16],position[3];
    if(!isfinite(next)||!isfinite(middle))return 1;
    uc_euler_rotation(0,middle,0,world);
    for(unsigned i=0;i<3;i++){
        float moved=(world[i]*m[12]+world[4+i]*m[13])+(world[8+i]*m[14]+world[12+i]);
        position[i]=(displacement[i]+moved)+p[i];
        position[i]+=helper[i];if(!isfinite(position[i]))return 1;
    }
    memcpy(out_position,position,sizeof position);*out_heading=next;return 0;
}
int uc_character_clearance(const float*p,const float*t,float radius,uint32_t region,
    uint8_t extend,UCCharacterClearanceRay ray,void*owner,uint8_t*clear,int32_t*blocking){
    if(!p||!t||!ray||!clear||!blocking||!isfinite(radius)||radius<0||radius>1e6f)return 1;
    for(unsigned i=0;i<3;i++)if(!isfinite(p[i])||!isfinite(t[i])||fabsf(p[i])>1e6f||fabsf(t[i])>1e6f)return 1;
    float dx=(float)((double)t[0]-p[0]),dz=(float)((double)t[2]-p[2]);
    double length=sqrt(((double)t[2]-p[2])*dz+(double)dx*dx);
    if(length<.10000000149011612){*clear=1;*blocking=0;return 0;}
    float inv=1.f/(float)length,x=inv*dx,z=inv*dz;
    float left_x=(0.f-z)*radius,left_z=x*radius,y=radius*0.f;
    float right_x=0.f-left_x,right_z=0.f-left_z,right_y=0.f-y;
    float from[3][3]={{p[0]+left_x,(y+p[1])+50.f,p[2]+left_z},
        {p[0]+right_x,(right_y+p[1])+50.f,p[2]+right_z},{p[0],p[1]+50.f,p[2]}};
    float to[3][3]={{t[0]+left_x,(t[1]+y)+50.f,t[2]+left_z},
        {t[0]+right_x,(t[1]+right_y)+50.f,t[2]+right_z},{t[0],t[1]+50.f,t[2]}};
    if(extend){to[2][0]+=left_z;to[2][1]+=(inv*0.f)*radius;to[2][2]+=z*radius;}
    for(unsigned i=0;i<3;i++){
        uint8_t hit=0;int32_t entity=0;
        if(ray(owner,from[i],to[i],region,0x20,0x2380,&hit,&entity))return 1;
        if(hit){*clear=0;*blocking=entity;return 0;}
    }
    *clear=1;*blocking=0;return 0;
}
int uc_character_clip_request(float*previous,uint8_t root,float frequency,float delta,
    float phase,const float*weights,float scale,float*output,UCCharacterClipRequest*request,uint8_t*emitted){
    if(!previous||!weights||!output||!request||!emitted||!isfinite(*previous)||
       *previous<0||*previous>=1||!isfinite(frequency)||frequency<=0||
       !isfinite(delta)||delta<=0||!isfinite(phase)||phase<0||phase>1||!isfinite(scale))return 1;
    for(unsigned i=0;i<5;i++)if(!isfinite(weights[i])||weights[i]<0||weights[i]>1)return 1;
    if(phase>=.9999899864196777f)phase=.9999899864196777f;
    UCCharacterClipRequest next={weights[0],*previous,phase,0,scale};
    uint8_t emit=weights[0]>0;
    if(emit){float advance=phase>=*previous?phase-*previous:phase-(*previous-1.f);
        next.rate=(advance/frequency)/delta;if(!isfinite(next.rate))return 1;}
    float result[5]={root?weights[0]:0,0,0,0,0};
    memcpy(output,result,sizeof result);if(emit)*request=next;*previous=phase;*emitted=emit;return 0;
}
int uc_character_clip_loop(UCCharacterClipLoop*s,UCCharacterClipCompletion*c,uint8_t root,
    float frequency,float delta,const float*weights,float*output,UCCharacterClipRequest*request,uint8_t*emitted){
    if(!s||!weights||!output||!request||!emitted||!isfinite(s->phase)||s->phase<0||s->phase>=1||
       !isfinite(frequency)||frequency<=0||!isfinite(delta)||delta<=0||
       (c&&!isfinite(c->total)))return 1;
    float advance=(float)((double)frequency*delta),unwrapped=s->phase+advance;
    if(!isfinite(unwrapped)||unwrapped>=2147483648.f)return 1;
    UCCharacterClipLoop next=*s;next.phase=(float)fmod((double)unwrapped,1.);
    UCCharacterClipCompletion completion={0};
    if(c){completion=*c;uint32_t cycles=(uint32_t)c->cycles+(uint32_t)(int32_t)unwrapped;
        memcpy(&completion.cycles,&cycles,4);completion.total+=advance;
        if(!isfinite(completion.total))return 1;if(unwrapped>1)completion.crossed=1;}
    float result[5];UCCharacterClipRequest node;uint8_t emit;
    if(uc_character_clip_request(&next.leaf_phase,root,frequency,delta,next.phase,weights,1,result,&node,&emit))return 1;
    *s=next;if(c)*c=completion;memcpy(output,result,sizeof result);
    if(emit)*request=node;*emitted=emit;return 0;
}
int uc_character_clip_sequence(UCCharacterClipSequence*s,UCCharacterClipCompletion*c,
    const uint8_t*root,const float*frequency,float fade,float scale,float delta,
    const float*weights,float*output,UCCharacterClipRequest*requests,uint8_t*emitted,uint8_t*finished){
    if(!s||!root||!frequency||!weights||!output||!requests||!emitted||!finished||
       !isfinite(s->elapsed)||s->elapsed<0||!isfinite(fade)||fade<0||!isfinite(scale)||
       !isfinite(delta)||delta<=0||(c&&!isfinite(c->total)))return 1;
    for(unsigned i=0;i<2;i++)if(!isfinite(frequency[i])||frequency[i]<=0||
        !isfinite(s->leaf_phase[i])||s->leaf_phase[i]<0||s->leaf_phase[i]>=1)return 1;
    for(unsigned i=0;i<5;i++)if(!isfinite(weights[i])||weights[i]<0||weights[i]>1)return 1;
    UCCharacterClipSequence next=*s;next.elapsed+=delta;
    float duration=(float)(1./frequency[0]),phase=(float)((double)frequency[0]*next.elapsed);
    float second=(float)(((double)next.elapsed-duration)*frequency[1]);
    float prior=(float)(((double)s->elapsed-duration)*frequency[1]);
    if(!isfinite(duration)||!isfinite(next.elapsed)||!isfinite(phase)||!isfinite(second)||
       !isfinite(prior)||second>=2147483648.f||second< -2147483648.f||
       prior>=2147483648.f||prior< -2147483648.f)return 1;
    uint8_t complete=next.elapsed>=duration&&next.first_pending;if(complete)next.first_pending=0;
    float normalized=(float)((double)frequency[0]*fade),blend=1;
    if(!isfinite(normalized))return 1;
    if((double)phase>1.-normalized){
        float x=(1.f-phase)/normalized;
        if(x<=0)blend=0;else if(x>=1)blend=1;
        else blend=(float)((1.-cos((double)x*3.1415927410125732))*.5);
    }
    UCCharacterClipCompletion completion={0};if(c){completion=*c;completion.cycles=(int32_t)floor(second);
        completion.total=second;if(floor(second)!=floor(prior))completion.crossed=1;}
    float input[5],parts[2][5]={{0}},result[5];UCCharacterClipRequest nodes[2];uint8_t emit[2]={0};
    for(unsigned i=0;i<5;i++)input[i]=(1.f-blend)*weights[i];
    if(uc_character_clip_request(&next.leaf_phase[1],root[1],frequency[1],delta,
        (float)((double)second-floor(second)),input,1,parts[1],nodes+1,emit+1))return 1;
    if(phase<1){
        for(unsigned i=0;i<5;i++)input[i]=blend*weights[i];
        if(uc_character_clip_request(&next.leaf_phase[0],root[0],frequency[0],delta,
            phase,input,scale,parts[0],nodes,emit))return 1;
    }
    for(unsigned i=0;i<5;i++)result[i]=(0.f+parts[1][i])+parts[0][i];
    *s=next;if(c)*c=completion;memcpy(output,result,sizeof result);
    for(unsigned i=0;i<2;i++){if(emit[i])requests[i]=nodes[i];emitted[i]=emit[i];}*finished=complete;return 0;
}
