#include "character_interaction.h"
#include <math.h>
int uc_character_interaction_update(UCCharacterInteraction*s,float delta,uint8_t expensive,const UCCharacterInteractionServices*h){
    if(!s||!h||!h->player_present||!h->busy||!h->flags_ready||!h->in_range||!h->stop_turn||!h->turn||!h->turn_ready||
       !h->speech_ready||!h->say||!h->speaking||!h->script||!h->cancel_speech||!h->transition||!h->base_update||
       !s->text||s->phase>6||s->phase==5||!isfinite(s->elapsed)||!isfinite(s->range)||!isfinite(s->facing)||
       !isfinite(delta)||delta<0||delta>1||expensive>1)return 1;
    int value=h->player_present(h->owner);if(value<0)return 2;if(!value)return 0;
    s->elapsed=(float)((double)s->elapsed+delta);
    switch(s->phase){
    case 0:
        value=h->busy(h->owner);if(value<0)return 2;
        if(!value){value=h->flags_ready(h->owner);if(value<0)return 2;
            if(value){if(s->range>=0){value=h->in_range(h->owner,s->range,s->facing);if(value<0)return 2;}
                if(value){if(h->stop_turn(h->owner)||h->turn(h->owner,1))return 2;s->phase=2;}}}
        //17BF7E repeats the busy query even after the successful start.
        if(h->busy(h->owner)<0)return 2;
        break;
    case 2:
        value=h->turn_ready(h->owner);if(value<0)return 2;if(value)s->phase=1;
        break;
    case 1:
        value=h->busy(h->owner);if(value<0)return 2;if(value)break;
        if(*s->text){value=h->speech_ready(h->owner);if(value<0)return 2;if(!value)break;
            if(h->say(h->owner,s->text))return 2;}
        s->elapsed=0;s->phase=3;break;
    case 3:
        if(h->turn(h->owner,0))return 2;
        value=h->speaking(h->owner);if(value<0)return 2;if(value)break;
        if(s->script_index!=-1&&h->script(h->owner,s->script_type,s->script_index))return 2;
        if(h->cancel_speech(h->owner))return 2;s->elapsed=0;s->phase=4;break;
    case 4:
        value=h->busy(h->owner);if(value<0)return 2;if(!value){s->elapsed=0;s->phase=6;}
        break;
    case 6:
        value=h->busy(h->owner);if(value<0)return 2;
        if(!value&&s->next!=-1&&h->transition(h->owner,s->next))return 2;
        break;
    }
    return h->base_update(h->owner,expensive)?2:0;
}
