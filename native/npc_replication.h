#ifndef UC_NPC_REPLICATION_H
#define UC_NPC_REPLICATION_H
#include "npc_thrown_weapon.h"
#ifdef __cplusplus
extern "C" {
#endif
/* DBE10/DBFC0/DC190 absent-replication branch. 28CC20 is the multiplayer
 * Projectile, not the thrown item's physical flight owner. Fresh47D60 options
 * and5B860/5B8F0 leave2918A0 NULL unless -serverstart/-clientjoin was supplied.
 * Explicitly reject imported/previously created replicas or network sessions;
 * successful calls preserve every field, including source-unwritten groups.
 * This does not supply DC330 flight/collision or the28CC20 network protocol. */
int uc_npc_replication_offline_admit(const UCNpcThrownWeapon*,const char*server_start,const char*client_join,const void*imported);
#ifdef __cplusplus
}
#endif
#endif
