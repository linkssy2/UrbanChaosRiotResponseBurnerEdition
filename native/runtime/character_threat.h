#pragma once
#include <cstdint>
//2FCF0/2FE60 retained perception state. Names describe observed consumers;
//owner callbacks resolve the actual actor, controller, collision and RNG.
struct NativeCharacterThreat {
    bool initialized=false,head=false,side_test=false;
    int32_t current=-1;
    float target[3]{},anchor[3]{},distance=0,direction[3]{},facing_dot=0,side_dot=0;
    float aim[3]{},jitter[3]{},last_clear[3]{};
    bool moved=false,facing=false,visible=false,in_cone=false,ever_in_cone=false;
    bool blocked=false,sides_clear=false,reserved84=false,near_line=false,changed=false;
    float cone_time=0,seen_time=0,unseen_time=0,shield_time=0,near_time=0,away_time=0,accuracy=0,phase=0,heading=0;
};
struct NativeCharacterThreatServices {
    void*owner=nullptr;int32_t actor=-1;double delta=0;float time=0;
    const float*position=nullptr;
    int32_t(*target)(void*)=nullptr; //33B40
    int(*resolve)(void*,int32_t,float[3])=nullptr; //1 found,0 missing,-1 error
    int(*body_forward)(void*,int32_t,float[3])=nullptr; //virtual2C row2,0 success
    int(*direction)(void*,bool right,float[3])=nullptr; //DE460/DE4D0,0 success
    int(*property)(void*,int32_t,const char*,int32_t*)=nullptr; //1 found,0 missing,-1 error
    int(*point)(void*,int32_t,const char*,float[3])=nullptr; //same result policy
    int(*visible)(void*,int32_t,float cosine,bool head,const float offset[3],bool characters)=nullptr;
    int(*near_line)(void*,int32_t,float radius)=nullptr; //332C0,0/1 or-1 error
    int(*recent)(void*,float*)=nullptr; //DE230,0/1 or-1 error
    int(*segment_clear)(void*,const float from[3],const float to[3])=nullptr; //4F4C0(...,0), 1 clear
    double(*difficulty)(void*,float minimum,float maximum)=nullptr; //16960
    double(*random)(void*,float minimum,float maximum)=nullptr; //2D3780,shared game RNG
};
int native_character_threat_begin(NativeCharacterThreat&,const NativeCharacterThreatServices&,bool head,bool sides);
int native_character_threat_step(NativeCharacterThreat&,const NativeCharacterThreatServices&,bool expensive);
int native_character_threat_shot(NativeCharacterThreat&,const NativeCharacterThreatServices&,bool fired);
