#ifndef UCRR_RAM_WRITE_WATCH_H
#define UCRR_RAM_WRITE_WATCH_H
#include <stdint.h>
#include <stddef.h>
/* Experimental single-resource physical write tracking. Not enabled by default.
 * Start requires a complete stable layout inventory and page-aligned shared RAM.
 * Native output seams and permanent disable before guest protection operations
 * are mandatory before using this against game memory. */
int uc_ram_watch_start(size_t physical_offset,size_t bytes);
int uc_ram_watch_matches(size_t physical_offset,size_t bytes);
uint64_t uc_ram_watch_arm(void);
int uc_ram_watch_clean(uint64_t token);
int uc_ram_watch_native_begin(void*buffer,size_t bytes);
void uc_ram_watch_native_end(int entered);
int uc_ram_watch_disable(void);
int uc_ram_watch_shutdown(void);
int uc_ram_watch_failed(void);
int uc_ram_watch_overlaps(size_t offset,size_t bytes);
/* Full containment only, never an overlapping-but-unprotected tail. */
int uc_ram_watch_contains(size_t offset,size_t bytes);
/* Optional second, disjoint resource. Native/protection hooks cover both. */
int uc_ram_watch_second_start(size_t offset,size_t bytes);
int uc_ram_watch_second_matches(size_t offset,size_t bytes);
int uc_ram_watch_second_overlaps(size_t offset,size_t bytes);
int uc_ram_watch_second_contains(size_t offset,size_t bytes);
uint64_t uc_ram_watch_second_arm(void);
int uc_ram_watch_second_clean(uint64_t token);
int uc_ram_watch_second_native_begin(void*buffer,size_t bytes);
void uc_ram_watch_second_native_end(int entered);
int uc_ram_watch_second_disable(void);
int uc_ram_watch_second_shutdown(void);
int uc_ram_watch_second_failed(void);
#endif
