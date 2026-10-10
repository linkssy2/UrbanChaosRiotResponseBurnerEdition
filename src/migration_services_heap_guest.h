#ifndef UC_MIGRATION_SERVICES_HEAP_GUEST_H
#define UC_MIGRATION_SERVICES_HEAP_GUEST_H
/* Returns zero without guest mutation when the complete path is unsupported. */
int uc_migration_services_heap(unsigned entry, unsigned frame);
#endif
