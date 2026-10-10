#ifndef UC_HYBRID_SMOKE_ROOM_GUEST_H
#define UC_HYBRID_SMOKE_ROOM_GUEST_H
#include <stdint.h>
/* Complete SmokeRoom setup/creation. Hazard registry and scheduling stay owned
 * by their original services. Zero declines before effects. */
int uc_hybrid_smoke_room(uint32_t entry);
#endif
