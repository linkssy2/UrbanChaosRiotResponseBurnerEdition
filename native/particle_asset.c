#include "particle_asset.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <xmmintrin.h>
void uc_particle_asset_reset(UCParticleAsset *a){
    if(!a)return;
    memset(a,0,sizeof *a);
    a->rate=10;a->burst=20;a->simulation.angle_max=1;
    a->simulation.force_min=100;a->simulation.force_max=200;
    a->simulation.start_size=a->simulation.end_size=10;a->simulation.lifetime=1;
    for(unsigned i=0;i<3;i++)for(unsigned j=0;j<4;j++)a->color[i][j]=1;
    a->color[2][3]=0;a->hold[0]=a->hold[1]=-1;
    a->blend=4;a->render_in_thermal=1;a->blur=.0333f;
    a->rotation[0]=-1;a->rotation[1]=a->rotation[3]=1;
    a->stretch_direction[1]=1;a->stretch_end=100;a->stretch_power=a->line_length=1;
}
enum Kind{FLOAT,HALF,INT,VEC2,VEC3,COLOR,ALPHA,BOOL,TEXT,EMITTER,SHAPE,PARTICLE,BLEND,RENDER};
typedef struct Property{const char *name;size_t offset;enum Kind kind;}Property;
#define P(name,field,kind) {name,offsetof(UCParticleAsset,field),kind}
static const Property properties[]={
 P("Emitter_Type",emitter_type,EMITTER),P("Emitter_Constant_Rate",rate,HALF),
 P("Emitter_Constant_Lifetime",emitter_lifetime,FLOAT),P("Emitter_Burst_Num",burst,INT),
 P("Emitter_Shape",simulation.shape,SHAPE),P("Emitter_Size",simulation.size,VEC3),
 P("Emitter_AngleMin",simulation.angle_min,FLOAT),P("Emitter_AngleMax",simulation.angle_max,FLOAT),
 P("Emitter_ForceMin",simulation.force_min,FLOAT),P("Emitter_ForceMax",simulation.force_max,FLOAT),
 P("Emitter_DepthBias",depth_bias,INT),P("Particle_Texture",texture,TEXT),P("Particle_Type",particle_type,PARTICLE),
 P("Particle_Rotation_SpeedMin",rotation[0],FLOAT),P("Particle_Rotation_SpeedMax",rotation[1],FLOAT),
 P("Particle_Rotation_OffsetMin",rotation[2],FLOAT),P("Particle_Rotation_OffsetMax",rotation[3],FLOAT),
 P("Particle_MotionBlurredSprite_BlurDuration",blur,FLOAT),
 P("Particle_StretchedSprite_AmountStart",stretch_start,FLOAT),P("Particle_StretchedSprite_AmountHold",stretch_hold,FLOAT),
 P("Particle_StretchedSprite_HoldStart",stretch_hold_start,FLOAT),P("Particle_StretchedSprite_HoldEnd",stretch_hold_end,FLOAT),
 P("Particle_StretchedSprite_AmountEnd",stretch_end,FLOAT),P("Particle_StretchedSprite_Power",stretch_power,FLOAT),
 P("Particle_StretchedSprite_Direction",stretch_direction,VEC2),
 P("Particle_Gravity",simulation.gravity,VEC3),P("Particle_Wind",simulation.wind,VEC3),P("Particle_Drag",simulation.drag,FLOAT),
 P("Particle_StartColour",color[0],COLOR),P("Particle_StartAlpha",color[0][3],ALPHA),
 P("Particle_HoldColour",color[1],COLOR),P("Particle_HoldAlpha",color[1][3],ALPHA),
 P("Particle_EndColour",color[2],COLOR),P("Particle_EndAlpha",color[2][3],ALPHA),
 P("Particle_HoldColourStart",hold[0],FLOAT),P("Particle_HoldColourEnd",hold[1],FLOAT),
 P("Particle_StartSize",simulation.start_size,FLOAT),P("Particle_EndSize",simulation.end_size,FLOAT),
 P("Particle_Lifetime",simulation.lifetime,FLOAT),P("Particle_Blend",blend,BLEND),P("Particle_RenderType",render_type,RENDER),
 P("Particle_CameraRelative",camera_relative,BOOL),P("Particle_Line_Length",line_length,FLOAT),
 P("Particle_GlobalWind",simulation.global_wind,FLOAT),P("Particle_RenderInThermal",render_in_thermal,BOOL),
 P("Particle_UseThermalColour",use_thermal_color,BOOL),P("Particle_ThermalColour",thermal_color,COLOR),
 P("Particle_ThermalColourAlpha",thermal_color[3],ALPHA),
 P("Particle_RotatingQuad_RotationSpeedMin",rotation[0],FLOAT),P("Particle_RotatingQuad_RotationSpeedMax",rotation[1],FLOAT),
 P("Particle_RotatingSprite_RotationSpeedMin",rotation[0],FLOAT),P("Particle_RotatingSprite_RotationSpeedMax",rotation[1],FLOAT)
};
#undef P
static int property(UCParticleAsset *a,const char *name,const char *value){
    const Property *p=NULL;
    for(size_t i=0;i<sizeof properties/sizeof properties[0];i++)if(!strcmp(name,properties[i].name)){p=&properties[i];break;}
    if(!p)return 0;
    unsigned char *out=(unsigned char*)a+p->offset;
    if(p->kind==TEXT){memcpy(out,value,strlen(value)+1);return 0;}
    if(p->kind==BOOL){
        if(!_stricmp(value,"true")||!_stricmp(value,"yes"))*out=1;
        else if(!_stricmp(value,"false")||!_stricmp(value,"no"))*out=0;
        else return 1;
        return 0;
    }
    if(p->kind>=EMITTER){
        static const char *const names[5][7]={{"Constant","Burst"},{"Cube","Sphere","Dome"},
            {"Sprite","MotionBlurredSprite","RotatingQuad","RotatingSprite","Line","StretchedSprite"},
            {"Opaque","Add","Alpha",NULL,"AlphaAdd","Subtract","AlphaSubtract"},{"Normal","HeatHaze"}};
        for(uint32_t i=0;i<7;i++)if(names[p->kind-EMITTER][i]&&!strcmp(value,names[p->kind-EMITTER][i])){memcpy(out,&i,4);return 0;}
        return 1;
    }
    if(p->kind==INT){
        char *end;long long v=strtoll(value,&end,10);
        if(end==value||v<INT32_MIN||v>INT32_MAX)return 1;
        int32_t result=(int32_t)v;memcpy(out,&result,4);return 0;
    }
    float v[4];memcpy(v,out,(p->kind==COLOR?4u:p->kind==VEC3?3u:p->kind==VEC2?2u:1u)*4);
    int count=p->kind==COLOR||p->kind==VEC3?3:p->kind==VEC2?2:1;
    if(sscanf(value,count==3?"%f %f %f":count==2?"%f %f":"%f",&v[0],&v[1],&v[2])!=count)return 1;
    if(p->kind==COLOR)count=4; // Original MULPS scales the existing alpha too.
    for(int i=0;i<count;i++){
        if(!isfinite(v[i])||fabsf(v[i])>1048576)return 1;
        if(p->kind==HALF)v[i]*=.5f;
        if(p->kind==COLOR||p->kind==ALPHA)v[i]*=1.0f/255.0f;
    }
    memcpy(out,v,(size_t)count*4);return 0;
}
static int name_char(unsigned char c){return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_';}
int uc_particle_asset_load(const void *data,size_t size,UCParticleAsset *out){
    if(!data||!out||size<4||size>1048576||(_mm_getcsr()&0xE040u))return 1;
    const unsigned char *bytes=data,*end=bytes+size,*at=bytes+4;
    uint32_t length=(uint32_t)bytes[0]|(uint32_t)bytes[1]<<8|(uint32_t)bytes[2]<<16|(uint32_t)bytes[3]<<24;
    if(length!=size-4)return 1;
    for(const unsigned char *p=at;p<end;p++)if(!*p||*p>=128)return 1;
    UCParticleAsset a;uc_particle_asset_reset(&a);
    while(at<end){
        while(at<end&&(*at==' '||*at=='\t'||*at=='\r'||*at=='\n'))at++;
        if(at==end)break;
        if(*at=='#'){while(at<end&&*at!='\r'&&*at!='\n')at++;continue;}
        char name[128],value[512];size_t n=0;
        do{if(n==sizeof name-1)return 1;name[n++]=(char)*at++;}while(at<end&&name_char(*at));name[n]=0;
        while(at<end&&(*at==' '||*at=='\t'))at++;
        if(at==end||*at=='\r'||*at=='\n')return 1;
        n=0;while(at<end&&*at!='\r'&&*at!='\n'){if(n==sizeof value-1)return 1;value[n++]=(char)*at++;}value[n]=0;
        if(property(&a,name,value))return 1;
    }
    if(a.hold[0]==-1&&a.hold[1]==-1){a.hold[0]=a.hold[1]=0;memcpy(a.color[1],a.color[0],sizeof a.color[0]);}
    *out=a;return 0;
}
