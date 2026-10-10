#include "shell_emitter.h"
#include <fenv.h>
#include <math.h>
#include <string.h>
#include <xmmintrin.h>
int uc_shell_bind(const UCModel *m,int32_t out[2]){
    static const char *names[]={"dummyshelleject","dummyshelleject02"};
    if(!m||!out||!m->bones||!m->bone_count||m->bone_count>4096)return 0;
    int32_t result[2]={-1,-1};
    for(unsigned tag=0;tag<2;tag++)for(uint32_t i=0;i<m->bone_count;i++){
        const uint8_t *p=m->bones[i].source_metadata+4;unsigned j;
        for(j=0;j<16;j++){unsigned a=p[j];if(a>='A'&&a<='Z')a+=32;if(a!=(uint8_t)names[tag][j]||!a)break;}
        if(j==16||(!p[j]&&!names[tag][j])){result[tag]=(int32_t)i;break;}
    }
    memcpy(out,result,sizeof result);return 1;
}
int uc_shell_emit(const UCShellEmitterView *v,UCGameRandom *rng,const UCShellEmitterServices *h){
    if(!v||!rng||!h||!v->suppressed||!v->equipped||!v->entity_id||!v->ground_y||
       !h->preflight||!h->tag_world||!h->create||rng->first>=250||rng->second>=250||
       (*v->equipped!=2&&*v->equipped!=5&&*v->equipped!=17)||!isfinite(*v->ground_y)||fabsf(*v->ground_y)>1e6f||
       fegetround()!=FE_TONEAREST||(_mm_getcsr()&0xE040u)||!h->preflight(h->context,v,rng))return 1;
    if(*v->suppressed)return 0;
    for(unsigned index=0;index<(*v->equipped==5?2u:1u);index++){
        UCShellRequest request={0};request.mode=4;request.bone_tag=index+4;
        request.properties.present=15;request.properties.weapon_type=(uint32_t)*v->equipped;
        float sample[3],world[16],rotated[3];int32_t word;
        static const float low[3]={-200,1500,-200},width[3]={400,600,400};
        for(unsigned j=0;j<3;j++){
            uc_game_random_next(rng,&word);sample[j]=(float)(((double)word*width[j])*0x1p-31+low[j]);
        }
        h->tag_world(h->context,index+4,world);
        /* Full2D3DC0(bitsBF060A92,0,bitsBF060A92,{0,0,0,1}). */
        static const float rotation[12]={0x1.bb67aep-1f,.5f,0,0,
            -0x1.bb67aep-2f,.75f,.5f,0,
            .25f,-0x1.bb67aep-2f,0x1.bb67aep-1f,0};
        for(unsigned j=0;j<3;j++)rotated[j]=(rotation[j]*sample[0]+rotation[4+j]*sample[1])+(rotation[8+j]*sample[2]+0.f);
        for(unsigned j=0;j<3;j++)request.properties.velocity[j]=(world[j]*rotated[0]+world[4+j]*rotated[1])+(world[8+j]*rotated[2]+0.f);
        request.properties.start_bone=(int32_t)index+4;request.properties.ground_y=*v->ground_y;
        request.parent_id=*v->entity_id;h->create(h->context,&request,rng);
    }
    return 0;
}
