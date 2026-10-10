#include "checkpoint_message.h"
#include <math.h>
#include <string.h>
static int valid(const char*t){size_t n=strlen(t);if(n>=128)return 0;for(size_t i=0;i<n;i++)if((unsigned char)t[i]<32||(unsigned char)t[i]>126)return 0;return !n||t[n-1]!=' ';}
int uc_checkpoint_message_add(UCCheckpointMessages*s,const char*t,uint8_t*accepted){
    if(!s||!t||!accepted||s->count>5||!valid(t))return 1;*accepted=0;
    if(s->count==5)return 0;UCCheckpointMessageRow*r=s->rows+s->count++;r->age=0;memcpy(r->text,t,strlen(t)+1);*accepted=1;return 0;
}
int uc_checkpoint_message_step(UCCheckpointMessages*s,float delta){
    if(!s||s->count>5||!isfinite(delta)||delta<0)return 1;
    if(!s->count){s->changed=1;return 0;}
    double age=(double)delta+s->rows[0].age;s->rows[0].age=(float)age;
    if(age>2.){--s->count;memmove(s->rows,s->rows+1,s->count*sizeof(*s->rows));s->changed=1;}else s->changed=0;return 0;
}
static uint16_t u16(const uint8_t*p){return p[0]|(uint16_t)p[1]<<8;}
static float measure(const UCFont*f,const char*t,float scale){
    float width=0;for(;*t;t++){unsigned c=(unsigned char)*t;unsigned i=c>f->maximum?0:u16(f->mapping+2*c);const uint8_t*g=f->glyphs+24*i;
        float advance=(float)(int16_t)u16(g+20)+(float)(int16_t)u16(g+16);advance*=scale;width=advance+width;}return width;
}
static float complete_width(const UCFont*f,const char*t,float scale){float width=measure(f,t,scale),space=measure(f," ",scale);width=(float)((double)space+width);return(float)((double)space+width);}
int uc_checkpoint_message_plan(const UCCheckpointMessages*s,const UCFont*f,float width,UCCheckpointMessagePlan*out){
    if(!s||!f||!out||!s->count||s->count>5||!isfinite(width)||width<=0||!isfinite(s->rows[0].age)||!valid(s->rows[0].text))return 1;
    const char*t=s->rows[0].text;float scale=1,extent=complete_width(f,t,scale),max=width*.85f;if(extent>max)scale=max/extent;
    extent=complete_width(f,t,scale);float half=width*.5f;half+=extent*.5f;float x=width-half,y=160.f-scale*15.f,blur=half*2.f,phase=s->rows[0].age/2.f;
    if(phase>.9f){float k=(phase-.9f)*10.f;if(k>=1)k=1;else if(!(k>0))k=0;x-=k*half;}
    else if(phase<.1f){float k=1-phase*10.f;if(k>=1)k=1;else if(!(k>0))k=0;x+=k*half;}else blur=0;
    x=(float)((double)measure(f," ",scale)+x);*out=(UCCheckpointMessagePlan){x,y,scale,blur};return 0;
}
