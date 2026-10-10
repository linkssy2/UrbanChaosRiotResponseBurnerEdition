#ifndef UC_HYBRID_SOLVER_PAIR_GUEST_H
#define UC_HYBRID_SOLVER_PAIR_GUEST_H
#include <stdint.h>
/* Optional UCRR_NATIVE_SOLVER_PAIR=1 for initial348320..348699 and
 * UCRR_NATIVE_SOLVER_REPEAT=1 for consecutive3486B0..348B45. Both defaultOFF.
 * Original advancement, iteration, callbacks and motion writeback remain.
 * Stable distinct primary-RAM spans, pinned zero worker configuration, masked
 * RN/gradual underflow, bounded finite inputs. Declines before guest effects. */
int uc_hybrid_solver_pair(uint32_t stack,uint32_t frame);
int uc_hybrid_solver_repeat(uint32_t stack,uint32_t frame);
#endif
