/* Independent immutable range/sequence/handler, shared audited implementation.
 * Starts are renderer-thread-only; native writes and faults may be concurrent. */
#include "ram_write_watch.h"
static int uc_ram_watch_primary_overlaps(size_t o,size_t n){return uc_ram_watch_overlaps(o,n);}
#define UC_WATCH_SECONDARY 1
#define uc_ram_watch_start uc_ram_watch_second_start
#define uc_ram_watch_matches uc_ram_watch_second_matches
#define uc_ram_watch_contains uc_ram_watch_second_contains
#define uc_ram_watch_overlaps uc_ram_watch_second_overlaps
#define uc_ram_watch_arm uc_ram_watch_second_arm
#define uc_ram_watch_clean uc_ram_watch_second_clean
#define uc_ram_watch_native_begin uc_ram_watch_second_native_begin
#define uc_ram_watch_native_end uc_ram_watch_second_native_end
#define uc_ram_watch_disable uc_ram_watch_second_disable
#define uc_ram_watch_shutdown uc_ram_watch_second_shutdown
#define uc_ram_watch_failed uc_ram_watch_second_failed
#include "ram_write_watch.c"
