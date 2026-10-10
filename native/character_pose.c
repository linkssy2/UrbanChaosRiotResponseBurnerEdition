#include "character_pose.h"
#include "point_light_receiver.h"
#include <math.h>
#include <string.h>
#include "euler_rotation.h"
static int finite(const float*p,unsigned n){for(unsigned i=0;i<n;i++)if(!isfinite(p[i]))return 0;return 1;}
static int admitted(float (*local)[16],float (*world)[16],uint32_t count,uint32_t bone,uint32_t parent){
    return local&&world&&local!=world&&count&&count<=4096&&bone<count&&parent<count&&
        finite(local[bone],16)&&finite(world[bone],16)&&finite(world[parent],16);
}
int uc_character_pose_translate(float (*local)[16],float (*world)[16],uint32_t count,
    uint32_t bone,uint32_t parent,const float displacement[3]){
    float inverse[16],l[4],w[4];
    if(!admitted(local,world,count,bone,parent)||!displacement||!finite(displacement,3)||!uc_point_light_inverse(world[parent],inverse))return 1;
    for(unsigned i=0;i<3;i++){
        float d=(inverse[i]*displacement[0]+inverse[4+i]*displacement[1])+inverse[8+i]*displacement[2];
        l[i]=local[bone][12+i]+d;w[i]=world[bone][12+i]+displacement[i];
    }
    l[3]=local[bone][15]+0.f;w[3]=world[bone][15]+0.f;
    if(!finite(l,4)||!finite(w,4))return 1;memcpy(local[bone]+12,l,16);memcpy(world[bone]+12,w,16);return 0;
}
int uc_character_pose_orient(float (*local)[16],float (*world)[16],uint32_t count,
    uint32_t bone,uint32_t parent,const float target[16],float weight,const UCAnimationBlendMath*math){
    float inverse[16],desired[16],l[16],w[16];UCAnimationKey from,to;
    if(!admitted(local,world,count,bone,parent)||!target||!finite(target,16)||!math||!isfinite(weight)||weight<0||weight>1||
       local[bone][15]!=1||!uc_point_light_inverse(world[parent],inverse))return 1;
    memcpy(desired,local[bone],64);
    for(unsigned row=0;row<3;row++)for(unsigned i=0;i<4;i++){
        float v[4];for(unsigned j=0;j<4;j++)v[j]=row==0?-target[4+j]:target[(row==1?8:0)+j];
        desired[row*4+i]=(inverse[i]*v[0]+inverse[4+i]*v[1])+(inverse[8+i]*v[2]+inverse[12+i]*v[3]);
    }
    if(uc_animation_binding_key_from_matrix(local[bone],&from)||uc_animation_binding_key_from_matrix(desired,&to)||
       uc_animation_blend_rotation(math,from.rotation,to.rotation,weight,from.rotation)||uc_animation_blend_matrix(&from,l))return 1;
    memcpy(w,world[bone],64);
    for(unsigned row=0;row<3;row++)for(unsigned i=0;i<4;i++)
        w[row*4+i]=(world[parent][i]*l[row*4]+world[parent][4+i]*l[row*4+1])+
            (world[parent][8+i]*l[row*4+2]+world[parent][12+i]*l[row*4+3]);
    if(!finite(w,16))return 1;memcpy(local[bone],l,64);memcpy(world[bone],w,64);return 0;
}
static double triangle_angle(float opposite,float a,float b){
    return acos((((double)a*a+(double)b*b)-(double)opposite*opposite)/(((double)a*b)*2.));
}
static void rotate_axes(float matrix[16],const float q[4]){
    for(unsigned row=0;row<3;row++){
        float*v=matrix+row*4,x=v[0]*2,y=v[1]*2,z=v[2]*2;
        float c[3]={q[1]*z-y*q[2],x*q[2]-q[0]*z,q[0]*y-q[1]*x};
        float d[3]={q[1]*c[2]-c[1]*q[2],c[0]*q[2]-q[0]*c[2],q[0]*c[1]-q[1]*c[0]};
        for(unsigned i=0;i<3;i++)v[i]=(v[i]+d[i])-c[i]*q[3];
        v[3]=(v[3]+0.f)-0.f*q[3];
    }
}
int uc_character_pose_foot(float (*local)[16],float (*world)[16],uint32_t count,
    const UCCharacterFoot*f,const UCAnimationBlendMath*math){
    if(!f||!math||!admitted(local,world,count,f->foot,f->pelvis)||!admitted(local,world,count,f->shin,f->thigh)||
       !isfinite(f->upper_length)||f->upper_length<=0||!isfinite(f->lower_length)||f->lower_length<=0||
       !isfinite(f->weight)||f->weight<0||f->weight>1||!isfinite(f->yaw)||fabsf(f->yaw)>1e6f||!finite(f->offset,3))return 1;
    uint32_t bones[4]={f->thigh,f->shin,f->foot,f->pelvis};for(unsigned i=0;i<4;i++)for(unsigned j=i+1;j<4;j++)if(bones[i]==bones[j])return 1;
    if(f->weight==0)return 0;
    float rotation[16],target[3],d[3];uc_euler_rotation(0,f->yaw,0,rotation);
    for(unsigned i=0;i<3;i++){
        target[i]=f->offset[i]+((rotation[i]*world[f->foot][12]+rotation[4+i]*world[f->foot][13])+(rotation[8+i]*world[f->foot][14]+rotation[12+i]));
        d[i]=target[i]-world[f->thigh][12+i];
    }
    float length=(float)sqrt(((double)d[0]*d[0]+(double)d[2]*d[2])+(double)d[1]*d[1]);
    if(!(length>0)||!isfinite(length))return 1;
    float inverse=1.f/length,axis[3];for(unsigned i=0;i<3;i++)axis[i]=0.f-inverse*d[i];
    float hip=0,knee=0;const double pi=3.1415927410125732;
    if(f->upper_length+f->lower_length>length){hip=(float)triangle_angle(f->lower_length,length,f->upper_length);knee=(float)(triangle_angle(length,f->upper_length,f->lower_length)-pi);}
    float pitch=(float)((pi-atan2((double)d[2],(double)d[1]))+hip);
    float roll=(float)(atan2((double)d[0],(double)d[1])-pi);
    float first[16],second[16],third[16];uc_euler_rotation(pitch,0,roll,first);uc_euler_rotation(pitch+knee,0,roll,second);
    float zero=world[f->foot][9]*0.f;
    float facing=(float)-atan2((double)(zero-world[f->foot][10]),(double)(world[f->foot][8]-zero));
    float twist=(float)((double)facing+f->yaw),sine=(float)sin((double)twist*.5);double cosine=cos((double)twist*.5);
    float q[4];for(unsigned i=0;i<3;i++)q[i]=(float)((double)sine*axis[i]);
    double norm=sqrt((((double)q[0]*q[0]+(double)q[1]*q[1])+(double)q[2]*q[2])+cosine*cosine);
    if(!(norm>0)||!isfinite(norm))return 1;double scale=1./norm;
    for(unsigned i=0;i<3;i++)q[i]=(float)(scale*q[i]);q[3]=(float)(cosine*scale);
    rotate_axes(first,q);rotate_axes(second,q);uc_euler_rotation(0,twist,0,third);
    float old_local[3][16],old_world[3][16];for(unsigned i=0;i<3;i++){memcpy(old_local[i],local[bones[i]],64);memcpy(old_world[i],world[bones[i]],64);}
    int failed=uc_character_pose_orient(local,world,count,f->thigh,f->pelvis,first,f->weight,math)||
        uc_character_pose_orient(local,world,count,f->shin,f->thigh,second,f->weight,math)||
        uc_character_pose_orient(local,world,count,f->foot,f->shin,third,f->weight,math);
    if(failed)for(unsigned i=0;i<3;i++){memcpy(local[bones[i]],old_local[i],64);memcpy(world[bones[i]],old_world[i],64);}
    return failed;
}

static float steer_spring(float delta,float value,float target,float gain,float acceleration,float *rate){
    float desired=((1.f-1.f/(delta*gain+1.f))*(target-value))/delta,change=desired-*rate;
    if(fabsf(desired)>fabsf(*rate)){float cap=delta*acceleration;
        if(!(cap>change))change=cap;if(!(change> -cap))change= -cap;}
    *rate+=change;float result=*rate*delta+value;
    if(fabs((double)result-target)<.0010000000474974513){*rate=0;return target;}return result;
}
static void pose_multiply(const float a[16],const float b[16],float out[16]){
    float result[16];memcpy(result+12,a+12,16);for(unsigned r=0;r<3;r++)for(unsigned c=0;c<4;c++)
        result[r*4+c]=(b[c]*a[r*4]+b[4+c]*a[r*4+1])+(b[8+c]*a[r*4+2]+b[12+c]*a[r*4+3]);
    memcpy(out,result,64);
}
int uc_character_pose_steer(UCCharacterSteering *state,float (*local)[16],
    const float (*world)[16],uint32_t count,float delta,const float *root_delta,const UCAnimationBlendMath *math){
    static const float limits[15][2]={{0},{0},{0},{0},{0},{0},{1.5707963705062866f,1.5707963705062866f},{0},
        {1.2217305898666382f,.8726646900177002f},{0},{.8726646900177002f,1.3962634801864624f},{0},
        {.8726646900177002f,1.3962634801864624f},{.5235987901687622f,.2617993950843811f},{.5235987901687622f,.2617993950843811f}};
    static const float redistribution[15][3]={{0},{0},{0},{0},{.2f,.1f,.5f},{.5f,.2f,.5f},{.5f,.2f,.5f},{0},{0},{0},{.5f,.5f,.5f},{0},{.5f,.5f,.5f},{0},{0}};
    if(!state||!local||!world||local==world||!math||!count||count>4096||!isfinite(delta)||delta<=0||delta>1||
       (root_delta&&(!isfinite(*root_delta)||fabsf(*root_delta)>1e6f)))return 1;
    for(unsigned i=0;i<15;i++){
        if(state->parents[i]>=15|| (i>=3&&state->parents[i]>=i)||state->bones[i]<-1||
            state->bones[i]>=(int32_t)count||!finite(state->spring[i],4))return 1;
        if(state->bones[i]>=0&&(!finite(local[state->bones[i]],16)||!finite(world[state->bones[i]],16)))return 1;
    }
    for(unsigned c=0;c<2;c++)if(!finite(&state->channel[c].weight,79))return 1;
    if(state->pelvis<-1||state->pelvis>=(int32_t)count||(state->pelvis>=0&&!finite(world[state->pelvis],16)))return 1;
    UCCharacterSteering next=*state;float r[15][7]={{0}},rot[15][3]={{0}},result[15][16];uint8_t changed[15]={0};
    for(unsigned i=3;i<15;i++){
        float *v=r[i],*parent=r[next.parents[i]],*spring=next.spring[i],*previous=next.spring[next.parents[i]];
        for(unsigned c=0;c<2;c++){
            const UCCharacterSteeringChannel *ch=&next.channel[c];const float *profile=ch->profile[i];
            float w=((1.f-v[0])*profile[4])*ch->weight;
            v[1]+=ch->yaw*w;v[2]+=profile[0]*w;v[3]+=ch->pitch*w;
            v[4]+=profile[1]*w;v[5]+=profile[2]*w;v[6]+=profile[3]*w;v[0]+=w;
        }
        if(root_delta)v[1]-=*root_delta;
        for(unsigned k=2;k<=5;k++)if(k!=3)v[k]=(v[k]-parent[k])*v[0]+parent[k];
        v[1]*=v[2];v[3]*=v[4];
        for(unsigned axis=0;axis<2;axis++){
            unsigned k=1+axis*2;float d=v[k]-parent[k],cap=limits[i][axis],clamped=d;
            if(!(cap>d))clamped=cap;if(!(clamped> -cap))clamped= -cap;
            v[k]=((clamped-d)*v[0]+d)+parent[k];
        }
        if(v[0]>0){
            if(root_delta){float d=v[1]-spring[0];if((d>0?1:-1)!=(*root_delta>0?1:-1)){
                float cap=(float)fabs((double)d),change=0.f-(*root_delta*v[0]);
                if(!(cap>change))change=cap;if(!(change> -cap))change= -cap;spring[0]+=change;
            }}
            spring[0]=steer_spring(delta,spring[0],v[1],v[6],v[6]*2.f,spring+1);
            spring[2]=steer_spring(delta,spring[2],v[3],v[6],v[6]*2.f,spring+3);
            if(v[0]<1)for(unsigned k=0;k<4;k++)spring[k]=(spring[k]-previous[k])*v[0]+previous[k];
        }else memcpy(spring,previous,16);
        v[1]=spring[0];v[3]=spring[2];
    }
    float crouch=next.channel[0].crouch+next.channel[1].crouch;
    if(crouch>0){if(crouch>1)crouch=1;float floor=((crouch*90.f)*.0027777778450399637f-.25f)*6.2831854820251465f;
        if(floor>r[10][3])r[10][3]=floor;}
    for(unsigned i=3;i<15;i++){rot[i][0]=r[i][1];rot[i][1]=r[i][3];rot[i][2]=r[i][5];}
    for(unsigned i=14;i>3;i--){unsigned p=next.parents[i];for(unsigned a=0;a<3;a++){
        float baseline=r[p][1+2*a];rot[i][a]=(r[i][1+2*a]-rot[i][a])*r[p][0]+rot[i][a];
        rot[p][a]+=(rot[i][a]-baseline)*redistribution[i][a];
    }}
    float relative[15][3]={{0}};for(unsigned i=3;i<15;i++)for(unsigned a=0;a<3;a++)relative[i][a]=rot[i][a]-rot[next.parents[i]][a];
    float compensation[4]={0,0,0,1};
    if(next.compensate&&next.pelvis>=0){float m[16]={0};const float*w=world[next.pelvis];m[15]=1;
        for(unsigned k=0;k<4;k++){m[k]=-w[8+k];m[4+k]=w[k];m[8+k]=w[4+k];}
        UCAnimationKey key;if(uc_animation_binding_key_from_matrix(m,&key))return 1;
        for(unsigned k=0;k<4;k++)compensation[k]=0.f-key.rotation[k];
    }
    for(unsigned i=3;i<15;i++){
        int32_t bone=next.bones[i],parent=next.bones[next.parents[i]];float *v=relative[i];
        if(bone<0||parent<0||(!(v[2]>0)&&v[0]==0&&v[1]==0))continue;
        float m[16],inverse[16];pose_multiply(local[bone],world[parent],m);
        if(v[2]>0){float q[4],identity[4]={0,0,0,1};
            if(uc_animation_blend_rotation(math,identity,compensation,v[2],q))return 1;rotate_axes(m,q);}
        if(v[0]!=0||v[1]!=0){float rotation[16];uc_euler_rotation(v[1],v[0],0,rotation);pose_multiply(m,rotation,m);}
        if(!uc_point_light_inverse(world[parent],inverse))return 1;pose_multiply(m,inverse,result[i]);
        if(!finite(result[i],16))return 1;changed[i]=1;
    }
    for(unsigned i=0;i<15;i++)if(!finite(next.spring[i],4))return 1;
    for(unsigned i=3;i<15;i++)if(changed[i])memcpy(local[next.bones[i]],result[i],64);*state=next;return 0;
}
