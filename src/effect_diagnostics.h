#ifndef UC_EFFECT_DIAGNOSTICS_H
#define UC_EFFECT_DIAGNOSTICS_H

/* Play-only packages retain effect decisions, but do not collect diagnostics. */
#if UCRR_PLAY_ONLY
#define UC_EFFECT_DIAGNOSTIC(...) ((void)0)
#else
#define UC_EFFECT_DIAGNOSTIC(...) do { __VA_ARGS__; } while (0)
#endif

#endif
