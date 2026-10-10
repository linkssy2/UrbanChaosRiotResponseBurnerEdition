#pragma once
int uc_migration_physics_ragdoll_pool(unsigned entry);
int uc_migration_physics_ragdoll_pool_retained(unsigned entry,volatile unsigned *extra);

unsigned uc_ragdoll_retention_get(void);
void uc_ragdoll_retention_set(unsigned mode);
double uc_ragdoll_retention_seconds(void);
