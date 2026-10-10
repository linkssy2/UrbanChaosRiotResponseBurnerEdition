#ifndef MIGRATION_INTERFACE_STORAGE_H
#define MIGRATION_INTERFACE_STORAGE_H
/* 0:80D27 read, 1:80E14 write, 2:80EEA truncate, 3:81157 size64,
 * 4:811E6 size32, 5:81221 close, 6:81265 open. Native imports remain the I/O boundary. */
int migration_interface_storage(unsigned operation);
#endif
