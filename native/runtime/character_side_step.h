#pragma once
#include "character_animation.h"
#include "character_navigation.h"
//3AFE0/3B030/3BC80 player-target branch. The source non-player3B910 branch is
//explicitly unsupported. Concrete body, source movement vector and world rays
//are required. Completion survives its helper until the borrowing graph retires.
class NativeCharacterSideStep {
    struct Completion;Completion *completion_=nullptr;
    int32_t target_=-1;float threshold_=0;
public:
    struct Services {
        void *owner=nullptr;const float *actor_world=nullptr;uint32_t region=0;
        int32_t child=-1;float delta=0;
        bool (*busy)(void*)=nullptr; //innerA8+84, actual2ACDE0 +20 task status
        int32_t (*player)(void*)=nullptr;
        const float *(*world)(void*,int32_t)=nullptr;
        int (*movement)(void*,int32_t,float out[3])=nullptr; //1 found; unsupported rejects
        void (*reset_navigation)(void*)=nullptr;
        UCCharacterClearanceRay ray=nullptr;
        void (*approach)(void*,const float point[3],const float direction[3],
            NativeCharacterAnimationCallback*,float rate)=nullptr;
    };
    NativeCharacterSideStep()=default;
    NativeCharacterSideStep(const NativeCharacterSideStep&)=delete;
    NativeCharacterSideStep&operator=(const NativeCharacterSideStep&)=delete;
    ~NativeCharacterSideStep();
    void reset(int32_t target,float threshold); //3B030: actual1072 threshold7.5
    void update(const Services &);
    //3B880 admits a separate StepAsideBehaviour child via3B2C0. The caller
    //owns that real constructor and attachment; this does not queue approach.
    bool force_requested(const Services &)const;
    void close(); //original callback+8 release; pending graph retains allocation
};
