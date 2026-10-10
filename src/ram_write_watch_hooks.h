#ifndef UCRR_RAM_WRITE_WATCH_HOOKS_H
#define UCRR_RAM_WRITE_WATCH_HOOKS_H
#ifdef UCRR_RAM_WATCH_SHADOW
#include "ram_write_watch.h"
#include <stdlib.h>
#define UC_WATCH_NATIVE_BEGIN(buffer,bytes) uc_ram_watch_native_begin((buffer),(bytes))
#define UC_WATCH_NATIVE_END(token) uc_ram_watch_native_end(token)
#define UC_WATCH_DISABLE() do { if(!uc_ram_watch_disable()) abort(); } while(0)
#define UC_WATCH_SHUTDOWN() do { if(!uc_ram_watch_shutdown()) abort(); } while(0)
#else
#define UC_WATCH_NATIVE_BEGIN(buffer,bytes) 0
#define UC_WATCH_NATIVE_END(token) ((void)0)
#define UC_WATCH_DISABLE() ((void)0)
#define UC_WATCH_SHUTDOWN() ((void)0)
#endif
#endif
