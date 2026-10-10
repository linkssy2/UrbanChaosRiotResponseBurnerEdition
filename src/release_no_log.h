#pragma once
/* Forced only into the isolated Windows RC experiment. Preserve ordinary file
 * I/O (shader caches/saves); remove console diagnostics before argument
 * evaluation and formatting. The kernel's separate file logger is gated too. */
#include <stdio.h>
#ifdef UCRR_RELEASE_NO_LOG
#define UCRR_CONSOLE_STREAM(s) ((s)==stdout || (s)==stderr)
#define printf(...) (0)
#define fprintf(s,...) (UCRR_CONSOLE_STREAM(s) ? 0 : fprintf((s),__VA_ARGS__))
#define fputc(c,s) (UCRR_CONSOLE_STREAM(s) ? 0 : fputc((c),(s)))
#define fputs(t,s) (UCRR_CONSOLE_STREAM(s) ? 0 : fputs((t),(s)))
#define fflush(s) (UCRR_CONSOLE_STREAM(s) ? 0 : fflush(s))
#endif
