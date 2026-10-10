#ifndef UC_MEDIC_TEAM_H
#define UC_MEDIC_TEAM_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Typed counterpart of17B400's36-byte guest packet. Text and flags are
 * borrowed for the synchronous event; the root copies the quit flags. */
typedef struct UCMedicTeamJoin {
 int32_t count;const int32_t *flags;int32_t quit,conditions[3];
 const char *heartbeat,*joined,*left;
} UCMedicTeamJoin;
typedef struct UCMedicTeamServices {
 void *owner;
 int(*heartbeat)(void*,const char*,const char*,const char*); /*70CC0*/
 int(*client)(void*,int32_t*); /*empty descriptor,mode1 MedicOrderClient*/
 int(*copy_flags)(void*,int32_t,const int32_t*,int32_t quit);
 int(*condition)(void*,int32_t); /*39340,including -1*/
} UCMedicTeamServices;
/*169AA0: an existing order is a no-op. Earlier successful effects remain
 * published on service failure, as with the original owner calls. */
int uc_medic_team_join(int32_t *order,const char *const defaults[3],
 const UCMedicTeamJoin*,const UCMedicTeamServices*);
typedef struct UCMedicOrder {int32_t command,previous,object;} UCMedicOrder;
typedef struct UCMedicOrderServices {
 void *owner;
 int(*root_boolean)(void*,const char*,uint8_t*,uint8_t *found);
 int(*manager_integer)(void*,const char*,int32_t*,uint8_t *found);
 int(*player_health)(void*,float*,uint8_t *found);
 int(*tutorial)(void*,int32_t command); /*643E0+128/+129*/
 int(*complete)(void*,int32_t command); /*40DD0*/
} UCMedicOrderServices;
/*1682B0 and the scalar properties of168360, including168170/168210.
 * Vector properties remain synchronous passthroughs to the actual manager. */
int uc_medic_order_choose(UCMedicOrder*,int32_t,const UCMedicOrderServices*);
int uc_medic_order_integer(const UCMedicOrder*,const char*,int32_t*,uint8_t*,const UCMedicOrderServices*);
#ifdef __cplusplus
}
#endif
#endif
