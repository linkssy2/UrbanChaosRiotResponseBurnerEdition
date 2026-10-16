#include "xbe_voice_mode.h"
struct Voice {uint32_t mode=0,pending_mode=UINT32_MAX;};
int main(){
    Voice a,b;
    if(uc_voice_mode(&a,1,1)||a.mode!=0)return 1;
    uc_voice_mode_commit(a);if(a.mode!=1||b.mode!=0)return 2;
    if(uc_voice_mode(&a,2,0)||a.mode!=2)return 3;
    if(!uc_voice_mode(&a,3,0)||!uc_voice_mode(&a,0,2)||!uc_voice_mode<Voice>(nullptr,0,0)||a.mode!=2)return 4;
    uc_voice_mode(&a,1,1);uc_voice_mode(&a,0,0);uc_voice_mode_commit(a);
    if(a.mode!=0)return 5;
    return 0;
}
