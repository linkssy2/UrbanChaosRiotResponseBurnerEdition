#include "animation_sound.h"
static int sound_type(const char *s){
    static const char name[]="sound";
    for(unsigned i=0;i<sizeof(name);i++){
        unsigned c=(unsigned char)s[i];if(c>='A'&&c<='Z')c+=32;
        if(c!=(unsigned char)name[i])return 0;
    }
    return 1;
}
UCAnimationStatus uc_animation_sound(const UCAnimationEvent *e,const void *owner,
    const UCAnimationSoundServices *h,uint8_t *handled){
    if(!e||!e->type||!handled)return UC_ANIMATION_ARGUMENT;
    if(!sound_type(e->type)){*handled=0;return UC_ANIMATION_OK;}
    if(e->parameter_count==1&&owner){
        if(!e->name||!h||!h->bank||!h->play)return UC_ANIMATION_ARGUMENT;
        const char *name=e->name;
        const void *bank=h->bank(h->context,owner);
        h->play(h->context,owner,bank,name);
    }
    *handled=1;return UC_ANIMATION_OK;
}
UCAnimationStatus uc_animation_sound_item(uint32_t state,int32_t active,
    int32_t previous,uint32_t *item){
    if(!item)return UC_ANIMATION_ARGUMENT;
    const int32_t selected=state!=1&&(active==0||active==26)?previous:active;
    if(selected<0||selected>=27)return UC_ANIMATION_ARGUMENT;
    *item=(uint32_t)selected;return UC_ANIMATION_OK;
}
