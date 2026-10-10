#ifndef UC_NPC_THROWN_SEQUENCE_H
#define UC_NPC_THROWN_SEQUENCE_H
#include "npc_thrown_weapon.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCNpcThrownSequenceServices {
    void *owner;
    int(*template_asset)(void *,void *source,const char *,const char **,unsigned *);
    int(*model_load)(void *,const char *); /*20BC40 then65360(variation) */
    int(*visible)(void *,unsigned); /*20A290 */
    int(*yield)(void *); /*real1FB930 suspension, state may change before return */
    int(*ignition)(void *);int(*flight)(void *);int(*explosion)(void *);
    int(*delta)(void *,double *);int(*flags)(void *,uint32_t);
    int(*pickup)(void *); /*nameWEAPON_PICKUP, then identity basis, Y+=9 */
    int(*land_sound)(void *);
    int(*sound_active)(void *,unsigned *);int(*remove)(void *);
} UCNpcThrownSequenceServices;
/* WholeDB4B0 plusDAA20 typed model boundary, exact lowercase properties. */
int uc_npc_thrown_model_start(UCNpcThrownWeapon *,UCBulletState *,const UCNpcThrownSequenceServices *);
/* WholeDD000. Native continuation supplies yield; no per-frame phase machine,
 * implicit effects, duplicate fields or timing approximation. Services must
 * complete the original named operation; failure preserves prior effects. */
int uc_npc_thrown_sequence(UCNpcThrownWeapon *,UCBulletState *,const UCNpcThrownSequenceServices *);
#ifdef __cplusplus
}
#endif
#endif
