#ifndef MIGRATION_INTERFACE_DEVICE_H
#define MIGRATION_INTERFACE_DEVICE_H
/* 0:21F750 publication, 1:208880 copy, 2:22D3E0 axis, 3:22D3F0 button,
 * 4:55860 logical-to-physical port. Preserve the public208880 trace hook. */
int migration_interface_device(unsigned operation);
#endif
