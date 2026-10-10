#ifndef NV2A_LIGHT_AB_PHASE_H
#define NV2A_LIGHT_AB_PHASE_H
#include <stdint.h>
/* Diagnostic only: twelve 1200-boundary phases in ABBA order. Start on the
 * first eligible comparison, not startup. Phase12 restores the configured
 * wide path. Caller logs transitions before executing the new phase. */
typedef struct NvLightAbPhase {uint32_t start;unsigned phase,started;} NvLightAbPhase;
static int nv_light_ab_step(NvLightAbPhase*p,uint32_t frame){
 if(!p->started){p->started=1;p->start=frame;p->phase=0;return 1;}
 unsigned phase=(uint32_t)(frame-p->start)/1200u;if(phase>12)phase=12;
 if(phase==p->phase)return 0;p->phase=phase;return 1;
}
static int nv_light_ab_wide(const NvLightAbPhase*p){return p->phase>=12||(p->phase%4==1)||(p->phase%4==2);}
#endif
