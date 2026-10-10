#include "npc_thrown_trigger.h"
int uc_npc_thrown_trigger_emit(UCParticleTrigger*t,int32_t id,float delta,const UCNpcThrownTriggerServices*h,uint32_t*out){
 if(!t||!h||!h->resolve||!out)return 1;const UCNpcThrownWeapon*s=h->resolve(h->owner,id);
 if(!s||!s->flag219){*out=0;return 0;}return uc_particle_trigger_emit(t,delta,out);
}
