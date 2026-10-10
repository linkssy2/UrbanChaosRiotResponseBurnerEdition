#include "linear_impulse.h"
#include <cmath>
#include <cstring>
#include <cfenv>
#include <xmmintrin.h>

int native_linear_impulse_apply(const float*m,float magnitude,const std::vector<uint32_t>&targets,
    const UCLevelEntitySlots&slots,const UCEntityEventRegistry&registry,
    const NativeLinearImpulseServices&s,float*out){
    if(!m||!out||!s.body||!s.impulse||targets.size()>4096||!std::isfinite(magnitude)||
       std::fabs(magnitude)>10000||(_mm_getcsr()&0xE040u)||std::fegetround()!=FE_TONEAREST)return 1;
    for(unsigned i=0;i<12;i++)if(!std::isfinite(m[i])||std::fabs(m[i])>1e6f)return 1;
    //3BD10 removes translation;174310 transforms local(0,0,1) with the original
    //pairwise float32 order.2A2910 then swaps Y/Z and changes units to metres.
    float render[3];for(unsigned i=0;i<3;i++)render[i]=(m[i]*0.f+m[4+i]*0.f)+(m[8+i]*1.f+0.f);
    float v[4]={render[0]*.01f,render[2]*.01f,render[1]*.01f,0.f};
    const float length=(v[2]*v[2])+(v[1]*v[1]+v[0]*v[0]);
    if(!std::isfinite(length)||length<1e-20f)return 1;
    const float estimate=_mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(length)));
    const float inverse=(.5f*estimate)*(3.f-(length*estimate)*estimate);
    for(unsigned i=0;i<4;i++)out[i]=(inverse*v[i])*magnitude;
    for(uint32_t target:targets){
        int32_t id=-1;if(uc_level_entity_lookup(&slots,target,&id))return 2;
        if(!uc_entity_event_lookup(&registry,id))continue;
        void*body=nullptr;if(s.body(s.owner,id,&body))return 2;
        if(!body)continue;
        if(s.body(s.owner,id,&body)||!body||s.impulse(s.owner,body,out))return 2;
    }
    return 0;
}
int NativeLinearImpulse::construct(NativeMissionWorld&w,uint32_t source,const NativeLinearImpulseServices&s){
    if(world_||entity.aux||source>=w.source_count()||!s.body||!s.impulse)return 1;
    const auto&m=w.metadata(source);const auto*d=m.definition;
    if(!d||std::strcmp(d->class_name,"LinearImpulse")||m.asset_present||*w.active_descriptor()!=&m)return 1;
    const uint8_t*cursor=*w.property_cursor(source);float magnitude=1000;int32_t count=0;int found;
    if(uc_entity_stream_get(d,&cursor,2,&magnitude,&found)||!found||
       uc_entity_stream_get(d,&cursor,1,&count,&found)||!found||count>4096||
       !std::isfinite(magnitude)||std::fabs(magnitude)>10000)return 1;
    std::vector<uint32_t> targets(count>0?size_t(count):0);
    for(auto&t:targets)if(uc_entity_stream_get(d,&cursor,1,&t,&found)||!found)return 1;
    world_=&w;services_=s;magnitude_=magnitude;targets_=std::move(targets);
    if(uc_entity_fresh_construct_authored(&w.context(),&entity.view,w.active_descriptor()))return failure_=2;
    *w.property_cursor(source)=cursor;
    if(entity.script.attach_actor(entity.view,m,w.script_services()))return failure_=2;
    return 0;
}
int NativeLinearImpulse::update(){
    if(!world_||failure_)return failure_?failure_:1;
    if(!started_){
        started_=true;entity.flags|=0x10000100;
        if(native_linear_impulse_apply(entity.transform,magnitude_,targets_,world_->slots(),world_->registry(),services_,momentum_))return failure_=2;
    }
    if(entity.script.update())return failure_=2;return 0;
}
int NativeLinearImpulse::late(){
    if(!world_||failure_)return failure_?failure_:1;
    if(!(entity.actor.word90&1)){for(unsigned i=12;i<15;i++){if(!std::isfinite(entity.transform[i]))return failure_=2;entity.transform[i]+=0.f;}entity.transform[15]=1.f;}return 0;
}
int NativeLinearImpulse::close_detached(){
    if(entity.script.close_detached())return 1;
    if(entity.aux&&uc_entity_fresh_aux_destroy(entity.aux))return 1;
    entity.aux=nullptr;targets_.clear();world_=nullptr;return 0;
}
