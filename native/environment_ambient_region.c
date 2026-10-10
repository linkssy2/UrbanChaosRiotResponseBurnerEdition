#include "environment_ambient_region.h"
#include <float.h>
#include <math.h>
#include <string.h>
static int xyz(const float*p){if(!p)return 0;for(unsigned k=0;k<3;k++)if(!isfinite(p[k]))return 0;return 1;}
static int rgb(const float*p){if(!xyz(p))return 0;for(unsigned k=0;k<3;k++)if(fabsf(p[k])>4096)return 0;return 1;}
int uc_environment_ambient_region_init(UCEnvironmentAmbientRegion*s,const float*c,const float*f){
 if(!s||(c&&!rgb(c))||(f&&!isfinite(*f)))return 0;UCEnvironmentAmbientRegion n={0};for(unsigned k=0;k<3;k++)n.rgb[k]=n.start_rgb[k]=c?c[k]:255.f;n.fade=f?*f:0;n.initial=1;*s=n;return 1;
}
int uc_environment_ambient_region_contains(const float*lo,const float*hi,const float*p,uint32_t*out){
 if(!out||!xyz(lo)||!xyz(hi)||!xyz(p))return 0;for(unsigned k=0;k<3;k++)if(lo[k]>hi[k])return 0;
 *out=p[0]>=lo[0]&&p[0]<=hi[0]&&p[2]>=lo[2]&&p[2]<=hi[2]&&p[1]>=lo[1]&&p[1]<=hi[1];return 1;
}
int uc_environment_ambient_region_update(UCEnvironmentAmbientRegion*s,const UCEnvironmentAmbientRegionHooks*h,void*ctx){
 if(!s||!rgb(s->rgb)||!rgb(s->start_rgb)||!xyz(s->start_position)||!isfinite(s->fade)||s->initial>1||s->active>1||!h||!h->membership||!h->position||!h->current_rgb||!h->initial_rgb||!h->target_rgb)return 0;
 uint8_t member=(uint8_t)h->membership(ctx);
 if(s->initial){if(member){h->initial_rgb(ctx,s->rgb);for(unsigned k=0;k<3;k++)s->start_position[k]=FLT_MAX;s->active=1;}s->initial=0;return 1;}
 if(!member){s->active=0;return 1;}
 if(s->fade<=0){h->target_rgb(ctx,s->rgb);s->active=1;return 1;}
 float p[3];h->position(ctx,p);if(!xyz(p))return 0;
 if(!s->active){memcpy(s->start_position,p,sizeof p);h->current_rgb(ctx,s->start_rgb);if(!rgb(s->start_rgb))return 0;}
 float d[3];for(unsigned k=0;k<3;k++)d[k]=s->start_position[k]-p[k];
 double yy=(double)d[1]*d[1],zz=(double)d[2]*d[2],xx=(double)d[0]*d[0];float distance=(float)sqrt((yy+zz)+xx),fraction=distance<s->fade?distance/s->fade:1.f;
 float c[3];for(unsigned k=0;k<3;k++){float difference=s->rgb[k]-s->start_rgb[k],change=difference*fraction;c[k]=s->start_rgb[k]+change;}
 h->target_rgb(ctx,c);s->active=1;return 1;
}
