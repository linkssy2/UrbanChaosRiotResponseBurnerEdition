#pragma once
#include <cstdint>
template<class Voice> uint32_t uc_voice_mode(Voice* v,uint32_t mode,uint32_t apply){
    if(!v||mode>2||apply>1)return 0x80070057u;
    if(apply)v->pending_mode=mode;
    else {v->mode=mode;v->pending_mode=UINT32_MAX;}
    return 0;
}
template<class Voice> void uc_voice_mode_commit(Voice& v){
    if(v.pending_mode!=UINT32_MAX){v.mode=v.pending_mode;v.pending_mode=UINT32_MAX;}
}
