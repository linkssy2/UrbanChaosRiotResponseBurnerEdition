#ifndef UCRR_APU_TRACE_OPTIONS_H
#define UCRR_APU_TRACE_OPTIONS_H
#include <stdlib.h>
/* Called only on the owning audio thread or while holding the APU mutex.
 * Detailed synchronous traces are diagnostic-only; default runs keep errors
 * and coarse device queue counters but avoid voice/route formatting and writes. */
static int apu_verbose_trace(void){
#if UCRR_LEAN_RUNTIME
    return 0;
#else
    static int enabled=-1;
    if(enabled<0){const char *value=getenv("UCRR_AUDIO_VERBOSE_TRACE");enabled=value&&value[0]=='1';}
    return enabled;
#endif
}
#endif
