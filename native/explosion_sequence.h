#ifndef UC_EXPLOSION_SEQUENCE_H
#define UC_EXPLOSION_SEQUENCE_H
#include "explosion.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct UCExplosionBlast {
 float point[3],radius;int32_t owner;
 uint8_t affects_doors,ignore_collision,pad22[2];float damage;int32_t type;
 uint8_t blast,pad33[3];int32_t team;uint8_t flag40,pad41[3];int32_t parent;
} UCExplosionBlast;
typedef struct UCExplosionSequence {
 const float *position;const UCExplosionParameters *parameters;
 int32_t id,parent;float damage;
} UCExplosionSequence;
typedef struct UCExplosionSequenceServices {
 void *owner;
 /* All endpoints complete the actual owner. Field stores its own lifetime;
  * create receives render cm and the constructor-scaled power. */
 int (*field_create)(void*,const float*,float);
 int (*shake)(void*,const float*,float range,float strength,float seconds,uint32_t kind);
 int (*online)(void*); /*0/1, negative failure; read at the original boundaries.*/
 int (*broadcast)(void*,const UCExplosionBlast*,int32_t sender);
 int (*area_effect)(void*,const float bounds[12],uint32_t type);
 int (*sound)(void*,const char*);
 /* Original helper address selects the complete concrete effect owner. */
 int (*effect)(void*,uint32_t original_helper);
 int (*debris)(void*,uint32_t count,float scale,float power,const char*texture);
 int (*field_done)(void*); /*0/1, negative failure; actual retained scanner.*/
 int (*field_step)(void*,float);
 double (*delta)(void*);
 int (*sound_active)(void*); /*actual2160A0 owner predicate,0/1/negative*/
 int (*yield)(void*);
 int (*remove)(void*);
} UCExplosionSequenceServices;
/* Full EA790 ordering with required concrete physics/audio/effect boundaries.
 * Parameters/identity/position stay valid, finite and unchanged through calls;
 * source delta and asynchronous completion may change across real yields.
 * Positive-power, successful force/scanner allocation domain only: retail
 *263570 dereferences its scanner; zero power does not prove a safe NULL path.
 * Constructor already scales power by1.5. No synthetic frame state machine.
 * Return1 before effects for invalid input,2 on committed service failure. */
int uc_explosion_sequence(UCExplosionSequence*,const UCExplosionSequenceServices*);
#ifdef __cplusplus
}
#endif
#endif
