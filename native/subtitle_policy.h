#ifndef UC_SUBTITLE_POLICY_H
#define UC_SUBTITLE_POLICY_H
#include <stdint.h>
/*7D000: inputs borrowed from the actual registered UI, names and actor owners.
 *Only NPC dialogue types1/2. Position is original actor40 (game units).*/
int uc_subtitle_admit(int ui,int named,int always,const float *player,const float *actor);
/*7CD80 NPC color; type2 is fallback text, character_type is the root property.*/
void uc_subtitle_color(int type,int character_type,int dispatch,float out[4]);
#endif
