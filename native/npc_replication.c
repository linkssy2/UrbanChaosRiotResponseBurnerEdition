#include "npc_replication.h"
int uc_npc_replication_offline_admit(const UCNpcThrownWeapon*s,const char*server,const char*client,const void*imported){return !s||server||client||imported||s->physics250?1:0;}
