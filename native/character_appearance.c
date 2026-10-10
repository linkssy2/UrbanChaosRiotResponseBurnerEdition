#include "character_appearance.h"
int uc_character_appearance_enter(UCCharacterAppearance*s,UCCharacterLoopController*c,const UCCharacterAppearanceServices*h){
    if(!s||!c||!c->actor||!h||!h->random||h->random->first>=250||h->random->second>=250||
       !h->has_body||!h->skin||!h->property||!h->equipment||!h->exit)return 1;
    s->controller=c;s->actor=c->actor;s->active=1;
    int32_t value;
    if(h->has_body(h->owner,s->actor)){
        uc_game_random_range(h->random,0,9999,&value);h->skin(h->owner,s->actor,value);
    }
    int group=-1;
    if((uint8_t)h->property(h->owner,s->controller,"Use head gear 1"))group=0;
    else if((uint8_t)h->property(h->owner,s->controller,"Use head gear 2"))group=1;
    else if((uint8_t)h->property(h->owner,s->controller,"Use head gear 3"))group=2;
    else if(!(uint8_t)h->property(h->owner,s->controller,"Dont use head gear")){
        uc_game_random_range(h->random,0,4,&value);
        if(value>=0&&value<=2)group=value;
    }
    if(group>=0)h->equipment(h->owner,s->actor,(unsigned)group,1);
    static const unsigned groups[]={5,6,3,4,7,8};
    for(unsigned i=0;i<6;i++){
        uc_game_random_boolean(h->random,&value);h->equipment(h->owner,s->actor,groups[i],value);
    }
    h->exit(h->owner,s);return 0;
}
