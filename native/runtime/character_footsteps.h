#pragma once
#include "../player_footstep.h"

//E1240, called by E2100 after the current animated pose and actor world update
//when either FootstepLeft/Right annotation is set. Material is actor+80;
//left/right are the live annotation bytes1A8/1A9, group is actor+94.
struct NativeCharacterFootstep {
    const float *world=nullptr;
    uint16_t group=0;
    uint8_t left=0,right=0;
    uint32_t material=0;
};
struct NativeCharacterFootstepServices {
    void *owner=nullptr;
    void (*bone)(void *,const char *name,float local[16])=nullptr;
    const UCCollisionResult *(*query)(void *,const float from[3],const float to[3],
        uint32_t type,uint32_t group,uint32_t mask,float impulse)=nullptr;
    void (*footprint)(void *,const UCPlayerFootprint &)=nullptr;
};
//Completes the posed-foot query and footprint request, not audio. The caller
//then supplies the real216C60 material sound and D0 RunFoley request. All
//services complete synchronously; query may update right, as the source reads
//it after the hit. Current bone/world, group and hit storage stay valid. The
//footprint receiver must complete2088D0; a successful call does not prove pixels.
//Throws on unsupported input/services; a post-query error is not retryable.
void native_character_footstep(NativeCharacterFootstep &,const NativeCharacterFootstepServices &);
