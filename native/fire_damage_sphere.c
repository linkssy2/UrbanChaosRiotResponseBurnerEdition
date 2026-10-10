#include "fire_damage_sphere.h"
#include <math.h>
#include <string.h>
static int point(const float*p){return p&&isfinite(p[0])&&isfinite(p[1])&&isfinite(p[2]);}
static int inside(const UCFireDamageSphere*s,const float*p){
 float z=p[2]-s->center[2],y=p[1]-s->center[1],x=p[0]-s->center[0];
 float square=z*z+y*y;square=square+x*x;if(s->radius_squared>square)return 1;
 y=(p[1]+170.f)-s->center[1];square=z*z+y*y;square=square+x*x;return s->radius_squared>square;
}
int uc_fire_damage_step(UCFireDamageSphere*s,const UCFireDamageServices*h){
 if(!s||!h||!h->session_mode||(*h->session_mode&7)||!h->player_count||!h->player||!h->multiplayer||!h->blocked||!h->damage||!h->delta||!h->remove||!point(s->center)||!isfinite(s->remaining)||!isfinite(s->radius_squared)||s->radius_squared<0)return 1;
 for(uint32_t i=0;;i++){
  int32_t count=h->player_count(h->owner);if(count<0||count>4||i>64)return 2;if(i>=(uint32_t)count)break;
  UCFirePlayer player={0};int found=h->player(h->owner,i,&player);if(found<0)return 2;if(!found)continue;
  if(!point(player.position)||!player.actor)return 2;if(!inside(s,player.position))continue;
  int multiplayer=h->multiplayer(h->owner);if(multiplayer<0)return 2;
  if(multiplayer){float to[3]={player.position[0],player.position[1]+40.f,player.position[2]};int blocked=h->blocked(h->owner,s->center,to,player.actor,player.group);if(blocked<0)return 2;if(blocked)continue;}
  UCPlayerFireDamage d={0};d.source=d.other=-1;d.kind=12;d.owner=s->owner;d.session=s->session;d.unique=s->unique;memcpy(d.center,s->center,12);
  float x=player.position[0]-s->center[0],y=player.position[1]-s->center[1],z=player.position[2]-s->center[2];double square=(double)z*z+(double)y*y;square=square+(double)x*x;
  float inverse=(float)(1./sqrt(square));if(!isfinite(inverse))return 2;d.direction[0]=x*inverse;d.direction[1]=y*inverse;d.direction[2]=z*inverse;
  if(h->damage(h->owner,player.actor,50.f,&d))return 2;
 }
 /* Actual offline1A5EB0 does not touch the remote-character list. */
 if(*h->session_mode&7)return 2;double delta=h->delta(h->owner);if(!isfinite(delta)||delta<0)return 2;
 double remaining=(double)s->remaining-delta;s->remaining=(float)remaining;if(!isfinite(s->remaining))return 2;
 if(remaining<=0&&h->remove(h->owner))return 2;return 0;
}
int uc_navigation_box(int32_t id,const float center[3],const float size[3],uint8_t flag,UCNavigationBox*out){
 if(!out||!point(center)||!point(size)||size[0]<=0||size[1]<0||size[2]<=0)return 1;UCNavigationBox b={0};b.id=id;b.height=size[1];b.allow_inside=flag;
 for(unsigned i=0;i<4;i++)memcpy(b.points[i],center,12);float x=size[0]*.5f,z=size[2]*.5f;
 b.points[0][0]-=x;b.points[0][2]+=z;b.points[1][0]+=x;b.points[1][2]+=z;b.points[2][0]+=x;b.points[2][2]-=z;b.points[3][0]-=x;b.points[3][2]-=z;
 for(unsigned i=0;i<4;i++){if(!point(b.points[i]))return 1;for(unsigned j=0;j<3;j++)b.center[j]+=b.points[i][j];}
 for(unsigned j=0;j<3;j++)b.center[j]*=.25f;
 x=b.points[0][0]-b.center[0];z=b.points[0][2]-b.center[2];b.radius=(float)sqrt((double)z*z+(double)x*x);if(!isfinite(b.radius))return 1;*out=b;return 0;
}

int uc_navigation_boxes_overlap(const UCNavigationBox*a,const UCNavigationBox*b){
 if(!a||!b||!point(a->center)||!point(b->center)||!isfinite(a->radius)||!isfinite(b->radius)||a->radius<0||b->radius<0)return -1;
 for(unsigned i=0;i<4;i++)if(!point(a->points[i])||!point(b->points[i]))return -1;
 if(a->allow_inside!=b->allow_inside)return 0;
 float x=a->center[0]-b->center[0],z=a->center[2]-b->center[2];
 if(sqrt((double)z*z+(double)x*x)>(double)a->radius+b->radius+150.f)return 0;
 float lo[2][3],hi[2][3];const UCNavigationBox*p[2]={a,b};
 for(unsigned k=0;k<2;k++){memcpy(lo[k],p[k]->points[0],12);memcpy(hi[k],p[k]->points[0],12);
  for(unsigned i=1;i<4;i++)for(unsigned j=0;j<3;j++){float v=p[k]->points[i][j];if(lo[k][j]>v)lo[k][j]=v;else if(v>hi[k][j])hi[k][j]=v;}}
 for(unsigned j=0;j<3;j++)if(lo[0][j]>hi[1][j]||lo[1][j]>hi[0][j])return 0;return 1;
}
