#include "mission_sound.h"
#include <math.h>
int uc_mission_sound_step(UCMissionSound*s,const UCMissionSoundServices*h){
 if(!s||!h)return 1;
 if(!s->started){if(s->has_bank&&s->has_start){if(!h->play||h->play(h->owner,0))return 2;s->started=1;}return 0;}
 if(s->wait_for_voice){unsigned playing=0;if(!h->playing||h->playing(h->owner,&playing))return 2;return playing?0:(!h->remove||h->remove(h->owner)?2:0);}
 if(s->stopping){if(s->remaining>0){if(!h->delta)return 2;double d=h->delta(h->owner);if(!isfinite(d))return 2;s->remaining=(float)((double)s->remaining-d);return 0;}return !h->remove||h->remove(h->owner)?2:0;}
 if(s->remaining>0){if(!h->delta)return 2;double d=h->delta(h->owner);if(!isfinite(d))return 2;s->remaining=(float)((double)s->remaining-d);}
 if(s->remaining<=0){if(!h->play||h->play(h->owner,1))return 2;s->stopping=1;s->remaining=.5f;}
 return 0;
}
