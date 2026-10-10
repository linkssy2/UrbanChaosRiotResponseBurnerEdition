#ifndef D3D8_DRAW_STATE_RECORD_H
#define D3D8_DRAW_STATE_RECORD_H
#include <stdint.h>
/* Internal value record. No PS combiner registers, pointers or host resources. */
typedef struct D3D8DrawStateRecord {
    uint32_t count, culled, unsupported_cull;
    uint32_t state[24], value[24];
} D3D8DrawStateRecord;
#endif
