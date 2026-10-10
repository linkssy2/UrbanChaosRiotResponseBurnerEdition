#ifndef UC_HYBRID_SOLVER_GUEST_H
#define UC_HYBRID_SOLVER_GUEST_H
#include <stdint.h>
/* Optional UCRR_NATIVE_SOLVER_CONTACT=1; default OFF. Exact3488F5..348A07
 * state for4..64 contiguous type2 rows. Original dispatch, row order, other
 * constraints, iteration and motion writeback remain. Declines before effects.
 * Supported title worker mode, distinct stable primary RAM source/output spans,
 * masked RN/gradual underflow, finite bounded inputs and finite final state. */
int uc_hybrid_solver_contact(uint32_t stack,uint32_t frame);
#endif
