#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Complete HazeRoom owner methods; zero leaves guest state unchanged. */
int uc_hybrid_haze_room(uint32_t entry);
#ifdef __cplusplus
}
#endif
